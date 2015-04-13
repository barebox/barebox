/*
 * xHCI HCD driver
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Some code borrowed from the Linux xHCI driver
 *   Author: Sarah Sharp
 *   Copyright (C) 2008 Intel Corp.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
//#define DEBUG
#include <clock.h>
#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>
#include <usb/usb.h>
#include <usb/xhci.h>

#include "xhci.h"

/*
 * xHCI ring handling
 */

static int xhci_ring_is_last_trb(struct xhci_ring *ring, union xhci_trb *trb)
{
        if (ring->type == TYPE_EVENT)
                return trb == &ring->trbs[NUM_EVENT_TRBS];
        else
                return TRB_TYPE_LINK(le32_to_cpu(trb->link.control));
}

static int xhci_ring_increment(struct xhci_ring *ring, bool enqueue)
{
	union xhci_trb **queue = (enqueue) ? &ring->enqueue : &ring->dequeue;

	(*queue)++;

        if (!xhci_ring_is_last_trb(ring, *queue))
		return 0;

	if (ring->type == TYPE_EVENT) {
		*queue = &ring->trbs[0];
		ring->cycle_state ^= 1;
	} else {
		u32 ctrl = le32_to_cpu((*queue)->link.control);
		void *p = (void *)(dma_addr_t)
			le64_to_cpu((*queue)->link.segment_ptr);

		ctrl = (ctrl & ~TRB_CYCLE) | ring->cycle_state;
		(*queue)->link.control = cpu_to_le32(ctrl);

		if (enqueue)
			ring->enqueue = p;
		else
			ring->dequeue = p;

		if (ctrl & LINK_TOGGLE)
			ring->cycle_state ^= 1;
	}

	return 0;
}

static int xhci_ring_issue_trb(struct xhci_ring *ring, union xhci_trb *trb)
{
	union xhci_trb *enq = ring->enqueue;
	int i;

	/* Pass TRB to hardware */
	trb->generic.field[3] &= ~TRB_CYCLE;
	trb->generic.field[3] |= ring->cycle_state;
	for (i = 0; i < 4; i++)
		enq->generic.field[i] = cpu_to_le32(trb->generic.field[i]);

	xhci_ring_increment(ring, 1);

	return 0;
}

static void xhci_ring_init(struct xhci_ring *ring, int num_trbs,
			   enum xhci_ring_type type)
{
	ring->type = type;
	ring->cycle_state = 1;
	ring->num_trbs = num_trbs;
	ring->enqueue = ring->dequeue = &ring->trbs[0];

	/* Event ring is not linked */
	if (type == TYPE_EVENT)
		return;

	ring->trbs[num_trbs-1].link.segment_ptr =
		cpu_to_le64((dma_addr_t)&ring->trbs[0]);
	ring->trbs[num_trbs-1].link.control =
		cpu_to_le32(TRB_TYPE(TRB_LINK) | LINK_TOGGLE);
}

static struct xhci_ring *xhci_get_endpoint_ring(struct xhci_hcd *xhci)
{
	struct xhci_ring *ring;

	if (list_empty(&xhci->rings_list)) {
		dev_err(xhci->dev, "no more endpoint rings available\n");
		return NULL;
	}

	ring = list_last_entry(&xhci->rings_list, struct xhci_ring, list);
	list_del_init(&ring->list);

	return ring;
}

static void xhci_put_endpoint_ring(struct xhci_hcd *xhci, struct xhci_ring *ring)
{
	if (!ring)
		return;

	memset(ring->trbs, 0, ring->num_trbs * sizeof(union xhci_trb));
	list_add_tail(&ring->list, &xhci->rings_list);
}

/*
 * xhci_get_endpoint_index - Used for passing endpoint bitmasks between the
 * core and HCDs.  Find the index for an endpoint given its descriptor.
 * Use the return value to right shift 1 for the bitmask.
 *
 * Index  = (epnum * 2) + direction - 1,
 * where direction = 0 for OUT, 1 for IN.
 * For control endpoints, the IN index is used (OUT index is unused), so
 * index = (epnum * 2) + direction - 1 = (epnum * 2) + 1 - 1 = (epnum * 2)
 */
static unsigned int xhci_get_endpoint_index(u8 epaddress, u8 epattributes)
{
	u8 epnum = epaddress & USB_ENDPOINT_NUMBER_MASK;
	u8 xfer = epattributes & USB_ENDPOINT_XFERTYPE_MASK;
        unsigned int index;

        if (xfer == USB_ENDPOINT_XFER_CONTROL)
                index = (unsigned int)(epnum * 2);
        else
                index = (unsigned int)(epnum * 2) +
                        ((epaddress & USB_DIR_IN) ? 1 : 0) - 1;

        return index;
}

static u8 xhci_get_endpoint_type(u8 epaddress, u8 epattributes)
{
	int in = epaddress & USB_ENDPOINT_DIR_MASK;
	u8 xfer = epattributes & USB_ENDPOINT_XFERTYPE_MASK;
	u8 type;

	switch (xfer) {
	case USB_ENDPOINT_XFER_CONTROL:
		type = CTRL_EP;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		type = (in) ? ISOC_IN_EP : ISOC_OUT_EP;
		break;
	case USB_ENDPOINT_XFER_BULK:
		type = (in) ? BULK_IN_EP : BULK_OUT_EP;
		break;
	case USB_ENDPOINT_XFER_INT:
		type = (in) ? INT_IN_EP : INT_OUT_EP;
		break;
	}

	return type;
}

/*
 * Convert interval expressed as 2^(bInterval - 1) == interval into
 * straight exponent value 2^n == interval.
 *
 */
static u32 xhci_parse_exponent_interval(struct usb_device *udev,
					struct usb_endpoint_descriptor *ep)
{
	u32 interval;

	interval = clamp_val(ep->bInterval, 1, 16) - 1;
	/*
	 * Full speed isoc endpoints specify interval in frames,
	 * not microframes. We are using microframes everywhere,
	 * so adjust accordingly.
	 */
	if (udev->speed == USB_SPEED_FULL)
		interval += 3;	/* 1 frame = 2^3 uframes */

	return interval;
}

/*
 * Convert bInterval expressed in microframes (in 1-255 range) to exponent of
 * microframes, rounded down to nearest power of 2.
 */
static u32 xhci_microframes_to_exponent(struct usb_device *udev,
			struct usb_endpoint_descriptor *ep, u32 desc_interval,
			u32 min_exponent, u32 max_exponent)
{
	u32 interval;

	interval = fls(desc_interval) - 1;
	return clamp_val(interval, min_exponent, max_exponent);
}

static inline u32 xhci_parse_microframe_interval(struct usb_device *udev,
					  struct usb_endpoint_descriptor *ep)
{
	if (ep->bInterval == 0)
		return 0;
	return xhci_microframes_to_exponent(udev, ep, ep->bInterval, 0, 15);
}


static inline u32 xhci_parse_frame_interval(struct usb_device *udev,
				     struct usb_endpoint_descriptor *ep)
{
	return xhci_microframes_to_exponent(udev, ep, ep->bInterval * 8, 3, 10);
}

static u32 xhci_get_endpoint_interval(struct usb_device *udev,
				      struct usb_endpoint_descriptor *ep)
{
	u8 type = ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	u32 interval = 0;

	switch (udev->speed) {
	case USB_SPEED_HIGH:
		/* Max NAK rate */
		if (type == USB_ENDPOINT_XFER_CONTROL ||
		    type == USB_ENDPOINT_XFER_BULK) {
			interval = xhci_parse_microframe_interval(udev, ep);
			break;
		}
		/* Fall through - SS and HS isoc/int have same decoding */
	case USB_SPEED_SUPER:
		if (type == USB_ENDPOINT_XFER_ISOC ||
		    type == USB_ENDPOINT_XFER_INT)
			interval = xhci_parse_exponent_interval(udev, ep);
		break;
	case USB_SPEED_FULL:
		if (type == USB_ENDPOINT_XFER_ISOC) {
			interval = xhci_parse_exponent_interval(udev, ep);
			break;
		}
		/*
		 * Fall through for interrupt endpoint interval decoding
		 * since it uses the same rules as low speed interrupt
		 * endpoints.
		 */
	case USB_SPEED_LOW:
		if (type == USB_ENDPOINT_XFER_ISOC ||
		    type == USB_ENDPOINT_XFER_INT)
			interval = xhci_parse_frame_interval(udev, ep);
		break;
	}

	return interval;
}

/* The "Mult" field in the endpoint context is only set for SuperSpeed isoc eps.
 * High speed endpoint descriptors can define "the number of additional
 * transaction opportunities per microframe", but that goes in the Max Burst
 * endpoint context field.
 */
static u32 xhci_get_endpoint_mult(struct usb_device *udev,
				  struct usb_endpoint_descriptor *ep)
{
	u8 type = ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;

	if (udev->speed != USB_SPEED_SUPER || type != USB_ENDPOINT_XFER_ISOC)
		return 0;
	/* FIXME: return ss_ep_comp_descriptor.bmAttributes */
	return 0;
}

/* Return the maximum endpoint service interval time (ESIT) payload.
 * Basically, this is the maxpacket size, multiplied by the burst size
 * and mult size.
 */
static u32 xhci_get_max_esit_payload(struct usb_device *udev,
				     struct usb_endpoint_descriptor *ep)
{
	u8 type = ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	int max_burst;
	int max_packet;
	u16 mps;

	/* Only applies for interrupt or isochronous endpoints */
	if (type != USB_ENDPOINT_XFER_INT && type != USB_ENDPOINT_XFER_ISOC)
		return 0;

	/* FIXME: return ss_ep_comp_descriptor.wBytesPerInterval */
	if (udev->speed == USB_SPEED_SUPER)
		return 0;

	mps = le16_to_cpu(ep->wMaxPacketSize);
	max_packet = GET_MAX_PACKET(mps);
	max_burst = (mps & 0x1800) >> 11;
	/* A 0 in max burst means 1 transfer per ESIT */
	return max_packet * (max_burst + 1);
}

int xhci_handshake(void __iomem *p, u32 mask, u32 done, int usec)
{
	u32 result;
	u64 start;

	start = get_time_ns();

	while (1) {
		result = readl(p) & mask;
		if (result == done)
			return 0;
		if (is_timeout(start, usec * USECOND))
			return -ETIMEDOUT;
	}
}

int xhci_issue_command(struct xhci_hcd *xhci, union xhci_trb *trb)
{
	int ret;

	ret = xhci_ring_issue_trb(&xhci->cmd_ring, trb);
	if (ret)
		return ret;

	/* Ring the bell */
	writel(DB_VALUE_HOST, &xhci->dba->doorbell[0]);
	readl(&xhci->dba->doorbell[0]);

	return 0;
}

static void xhci_set_event_dequeue(struct xhci_hcd *xhci)
{
        u64 reg64;

        reg64 = xhci_read_64(&xhci->ir_set->erst_dequeue);
        reg64 &= ERST_PTR_MASK;
        /*
	 * Don't clear the EHB bit (which is RW1C) because
         * there might be more events to service.
         */
        reg64 &= ~ERST_EHB;
	reg64 |= (dma_addr_t)xhci->event_ring.dequeue &
		~(dma_addr_t)ERST_PTR_MASK;

	/* Update HC event ring dequeue pointer */
	xhci_write_64(reg64, &xhci->ir_set->erst_dequeue);
}

int xhci_wait_for_event(struct xhci_hcd *xhci, u8 type, union xhci_trb *trb)
{
	while (true) {
		union xhci_trb *deq = xhci->event_ring.dequeue;
		u8 event_type;
		int i, ret;

		ret = xhci_handshake(&deq->event_cmd.flags,
				     cpu_to_le32(TRB_CYCLE),
				     cpu_to_le32(xhci->event_ring.cycle_state),
				     XHCI_CMD_DEFAULT_TIMEOUT / USECOND);
		if (ret) {
			dev_err(xhci->dev, "Timeout while waiting for event\n");
			return ret;
		}

		for (i = 0; i < 4; i++)
			trb->generic.field[i] =
				le32_to_cpu(deq->generic.field[i]);

		xhci_set_event_dequeue(xhci);
		xhci_ring_increment(&xhci->event_ring, 0);

		event_type = TRB_FIELD_TO_TYPE(trb->event_cmd.flags);

		switch (event_type) {
		case TRB_PORT_STATUS:
			dev_dbg(xhci->dev, "Event PortStatusChange %u\n",
				GET_PORT_ID(trb->generic.field[0]));
			break;
		case TRB_TRANSFER:
			dev_dbg(xhci->dev, "Event Transfer %u\n",
				GET_COMP_CODE(trb->event_cmd.status));
			ret = -GET_COMP_CODE(trb->event_cmd.status);
			if (ret == -COMP_SUCCESS)
				ret = 0;
			break;
		case TRB_COMPLETION:
			dev_dbg(xhci->dev, "Event CommandCompletion %u\n",
				GET_COMP_CODE(trb->event_cmd.status));
			ret = -GET_COMP_CODE(trb->event_cmd.status);
			if (ret == -COMP_SUCCESS)
				ret = 0;
			break;
		default:
			dev_err(xhci->dev, "unhandled event %u (%02x) [%08x %08x %08x %08x]\n",
				event_type, event_type,
				trb->generic.field[0], trb->generic.field[1],
				trb->generic.field[2], trb->generic.field[3]);
		}

		if (event_type == type)
			return ret;
	}
	return -ENOSYS;
}

static struct xhci_virtual_device *xhci_find_virtdev(struct xhci_hcd *xhci,
						     struct usb_device *udev)
{
	struct xhci_virtual_device *vdev;

	list_for_each_entry(vdev, &xhci->vdev_list, list)
		if (vdev->udev == udev)
			return vdev;

	return NULL;
}

static struct xhci_virtual_device *xhci_alloc_virtdev(struct xhci_hcd *xhci,
						      struct usb_device *udev)
{
	struct xhci_virtual_device *vdev;
	size_t sz_ctx, sz_ictx, sz_dctx;
	void *p;

	vdev = xzalloc(sizeof(*vdev));
	vdev->udev = udev;
	list_add_tail(&vdev->list, &xhci->vdev_list);

	sz_ctx = HCC_64BYTE_CONTEXT(xhci->hcc_params) ? 2048 : 1024;
	/* Device Context: 64B aligned */
	sz_dctx = ALIGN(sz_ctx, 64);
	/* Input Control Context: 64B aligned */
	sz_ictx = ALIGN(sz_ctx + HCC_CTX_SIZE(xhci->hcc_params), 64);

	vdev->dma_size = sz_ictx + sz_dctx;
	p = vdev->dma = dma_alloc_coherent(vdev->dma_size, DMA_ADDRESS_BROKEN);
	memset(vdev->dma, 0, vdev->dma_size);

	vdev->out_ctx = p; p += sz_dctx;
	vdev->in_ctx = p; p += sz_ictx;

	return vdev;
}

static void xhci_free_virtdev(struct xhci_virtual_device *vdev)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	int i;

	for (i = 0; i < USB_MAXENDPOINTS; i++)
		if (vdev->ep[i])
			xhci_put_endpoint_ring(xhci, vdev->ep[i]);

	list_del(&vdev->list);
	dma_free_coherent(vdev->dma, 0, vdev->dma_size);
	free(vdev);
}

static int xhci_virtdev_issue_transfer(struct xhci_virtual_device *vdev,
				 u8 ep, union xhci_trb *trb, bool ringbell)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	struct xhci_ring *ring = vdev->ep[ep];
	int ret;

	ret = xhci_ring_issue_trb(ring, trb);
	if (ret || !ringbell)
		return ret;

	/* Ring the bell */
	writel(DB_VALUE(ep, 0), &xhci->dba->doorbell[vdev->slot_id]);
	readl(&xhci->dba->doorbell[vdev->slot_id]);

	return 0;
}

static void xhci_virtdev_zero_in_ctx(struct xhci_virtual_device *vdev)
{
	int i;

        /* When a device's add flag and drop flag are zero, any subsequent
         * configure endpoint command will leave that endpoint's state
         * untouched.  Make sure we don't leave any old state in the input
         * endpoint contexts.
         */
        vdev->in_ctx->icc.drop_flags = 0;
        vdev->in_ctx->icc.add_flags = 0;
        vdev->in_ctx->slot.dev_info &= cpu_to_le32(~LAST_CTX_MASK);
        /* Endpoint 0 is always valid */
        vdev->in_ctx->slot.dev_info |= cpu_to_le32(LAST_CTX(1));
	for (i = 1; i < 31; i++) {
		vdev->in_ctx->ep[i].ep_info = 0;
		vdev->in_ctx->ep[i].ep_info2 = 0;
		vdev->in_ctx->ep[i].deq = 0;
		vdev->in_ctx->ep[i].tx_info = 0;
	}
}

static int xhci_virtdev_disable_slot(struct xhci_virtual_device *vdev)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	union xhci_trb trb;
	int ret;

	/* Issue Disable Slot Command */
	memset(&trb, 0, sizeof(union xhci_trb));
	trb.event_cmd.flags = TRB_TYPE(TRB_DISABLE_SLOT) |
		SLOT_ID_FOR_TRB(vdev->slot_id);
	xhci_print_trb(xhci, &trb, "Request  DisableSlot");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response DisableSlot");

        /* Clear Device Context Base Address Array */
	xhci->dcbaa[vdev->slot_id] = 0;

	return ret;
}

static int xhci_virtdev_enable_slot(struct xhci_virtual_device *vdev)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	union xhci_trb trb;
	int slot_id;
	int ret;

	/* Issue Enable Slot Command */
	memset(&trb, 0, sizeof(union xhci_trb));
	trb.event_cmd.flags = TRB_TYPE(TRB_ENABLE_SLOT);
	xhci_print_trb(xhci, &trb, "Request  EnableSlot");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response EnableSlot");
	if (ret)
		return ret;

	slot_id = TRB_TO_SLOT_ID(trb.event_cmd.flags);
	if (slot_id == 0) {
		dev_err(xhci->dev, "EnableSlot returned reserved slot ID 0\n");
		return -EINVAL;
	}

	vdev->slot_id = slot_id;

	return 0;
}

int xhci_virtdev_reset(struct xhci_virtual_device *vdev)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	union xhci_trb trb;
	int ret;

	/* If device is not setup, there is no point in resetting it */
	if (GET_SLOT_STATE(le32_to_cpu(vdev->out_ctx->slot.dev_state)) ==
	    SLOT_STATE_DISABLED)
                return 0;

	memset(&trb, 0, sizeof(union xhci_trb));
	trb.event_cmd.flags = TRB_TYPE(TRB_RESET_DEV) |
		SLOT_ID_FOR_TRB(vdev->slot_id);
	xhci_print_trb(xhci, &trb, "Request  Reset");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response Reset");

        /*
	 * The Reset Device command can't fail, according to the 0.95/0.96 spec,
         * unless we tried to reset a slot ID that wasn't enabled,
         * or the device wasn't in the addressed or configured state.
         */
        switch (GET_COMP_CODE(trb.event_cmd.status)) {
        case COMP_CMD_ABORT:
        case COMP_CMD_STOP:
                dev_warn(xhci->dev, "Timeout waiting for reset device command\n");
                ret = -ETIMEDOUT;
		break;
        case COMP_EBADSLT: /* 0.95 completion code for bad slot ID */
        case COMP_CTX_STATE: /* 0.96 completion code for same thing */
                /* Don't treat this as an error.  May change my mind later. */
                ret = 0;
        case COMP_SUCCESS:
                break;
        default:
                ret = -EINVAL;
        }

	return ret;
}

/*
 * Once a hub descriptor is fetched for a device, we need to update the xHC's
 * internal data structures for the device.
 */
static int xhci_virtdev_update_hub_device(struct xhci_virtual_device *vdev,
					  void *buffer, int length)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	struct usb_hub_descriptor *desc = buffer;
	union xhci_trb trb;
	u32 dev_info, dev_info2, tt_info;
	u8 maxchild;
	u16 hubchar;
	int ret;

	/* Need at least first byte of wHubCharacteristics */
	if (length < 4)
		return 0;
	/* Skip already configured hub device */
	if (vdev->out_ctx->slot.dev_info & DEV_HUB)
		return 0;

	maxchild = desc->bNbrPorts;
	hubchar = le16_to_cpu(desc->wHubCharacteristics);

	/* update slot context */
	memcpy(&vdev->in_ctx->slot, &vdev->out_ctx->slot,
	       sizeof(struct xhci_slot_ctx));
	vdev->in_ctx->icc.add_flags |= cpu_to_le32(SLOT_FLAG);
	vdev->in_ctx->icc.drop_flags = 0;
	vdev->in_ctx->slot.dev_state = 0;
	dev_info = le32_to_cpu(vdev->in_ctx->slot.dev_info);
	dev_info2 = le32_to_cpu(vdev->in_ctx->slot.dev_info2);
	tt_info = le32_to_cpu(vdev->in_ctx->slot.tt_info);

	dev_info |= DEV_HUB;
	/* HS Multi-TT in bDeviceProtocol */
	if (vdev->udev->speed == USB_SPEED_HIGH &&
	    vdev->udev->descriptor->bDeviceProtocol == USB_HUB_PR_HS_MULTI_TT)
		dev_info |= DEV_MTT;
	if (xhci->hci_version > 0x95) {
		dev_info2 |= XHCI_MAX_PORTS(maxchild);
		/* Set TT think time - convert from ns to FS bit times.
		 * 0 = 8 FS bit times, 1 = 16 FS bit times,
		 * 2 = 24 FS bit times, 3 = 32 FS bit times.
		 *
		 * xHCI 1.0: this field shall be 0 if the device is not a
		 * High-speed hub.
		 */
		if (xhci->hci_version < 0x100 ||
		    vdev->udev->speed == USB_SPEED_HIGH) {
			u32 think_time = (hubchar & HUB_CHAR_TTTT) >> 5;
                        tt_info |= TT_THINK_TIME(think_time);
		}
        }
	vdev->in_ctx->slot.dev_info = cpu_to_le32(dev_info);
	vdev->in_ctx->slot.dev_info2 = cpu_to_le32(dev_info2);
	vdev->in_ctx->slot.tt_info = cpu_to_le32(tt_info);

        /* Issue Configure Endpoint or Evaluate Context Command */
	memset(&trb, 0, sizeof(union xhci_trb));
	xhci_write_64((dma_addr_t)vdev->in_ctx, &trb.event_cmd.cmd_trb);
	trb.event_cmd.flags = SLOT_ID_FOR_TRB(vdev->slot_id);
	if (xhci->hci_version > 0x95)
		trb.event_cmd.flags |= TRB_TYPE(TRB_CONFIG_EP);
	else
		trb.event_cmd.flags |= TRB_TYPE(TRB_EVAL_CONTEXT);
	xhci_print_trb(xhci, &trb, "Request  ConfigureEndpoint");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response ConfigureEndpoint");
	xhci_virtdev_zero_in_ctx(vdev);

	return ret;
}

static int xhci_virtdev_update_hub_status(struct xhci_virtual_device *vhub,
					  int port)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vhub->udev->host);
	struct usb_device *udev = vhub->udev->children[port - 1];
	struct xhci_virtual_device *vdev;

	if (!udev)
		return 0;

	/* Check if we have a virtual device for it */
	vdev = xhci_find_virtdev(xhci, udev);
	if (vdev)
		xhci_virtdev_detach(vdev);

	return 0;
}

static int xhci_virtdev_configure(struct xhci_virtual_device *vdev, int config)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	struct usb_device *udev = vdev->udev;
	union xhci_trb trb;
	u32 add_flags = 0, last_ctx;
	int i, j;
	int ret;

	for (i = 0; i < udev->config.no_of_if; i++) {
		struct usb_interface *intf = &udev->config.interface[i];

		for (j = 0; j < intf->no_of_ep; j++) {
			struct usb_endpoint_descriptor *ep = &intf->ep_desc[j];
			u8 type = ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
			u8 eptype = xhci_get_endpoint_type(ep->bEndpointAddress,
							   ep->bmAttributes);
			u8 epi = xhci_get_endpoint_index(ep->bEndpointAddress,
							 ep->bmAttributes);
			struct xhci_ep_ctx *ctx = &vdev->in_ctx->ep[epi];
			u32 mps, interval, mult, esit, max_packet, max_burst;
			u32 ep_info, ep_info2, tx_info;

			vdev->ep[epi] = xhci_get_endpoint_ring(xhci);
			if (!vdev->ep[epi])
				return -ENOMEM;
			/* FIXME: set correct ring type */
			xhci_ring_init(vdev->ep[epi], NUM_TRANSFER_TRBS,
				       TYPE_BULK);
			add_flags |= BIT(epi+1);

			mps = le16_to_cpu(ep->wMaxPacketSize);
			interval = xhci_get_endpoint_interval(vdev->udev, ep);
			mult = xhci_get_endpoint_mult(vdev->udev, ep);
			esit = xhci_get_max_esit_payload(vdev->udev, ep);
			max_packet = GET_MAX_PACKET(mps);
			max_burst = 0;

			ep_info = EP_INTERVAL(interval) | EP_MULT(mult);
			ep_info2 = EP_TYPE(eptype);
			if (type == USB_ENDPOINT_XFER_ISOC)
				ep_info2 |= ERROR_COUNT(0);
			else
				ep_info2 |= ERROR_COUNT(3);

			switch (udev->speed) {
			case USB_SPEED_SUPER:
				/* FIXME: max_burst = ss_ep_comp.bMaxBurst */
				max_burst = 0;
				break;
			case USB_SPEED_HIGH:
				/* Some devices get this wrong */
				if (type == USB_ENDPOINT_XFER_BULK)
					max_packet = 512;
				if (type == USB_ENDPOINT_XFER_ISOC ||
				    type == USB_ENDPOINT_XFER_INT)
					max_burst = (mps & 0x1800) >> 11;
				break;
			case USB_SPEED_FULL:
			case USB_SPEED_LOW:
				break;
			}
			ep_info2 |= MAX_PACKET(max_packet) | MAX_BURST(max_burst);

			tx_info = MAX_ESIT_PAYLOAD_FOR_EP(esit);
			switch (type) {
			case USB_ENDPOINT_XFER_CONTROL:
				tx_info |= AVG_TRB_LENGTH_FOR_EP(8);
				break;
			case USB_ENDPOINT_XFER_ISOC:
			case USB_ENDPOINT_XFER_BULK:
				tx_info |= AVG_TRB_LENGTH_FOR_EP(3 * 1024);
				break;
			case USB_ENDPOINT_XFER_INT:
				tx_info |= AVG_TRB_LENGTH_FOR_EP(1 * 1024);
				break;
			}

			ctx->ep_info = cpu_to_le32(ep_info);
			ctx->ep_info2 = cpu_to_le32(ep_info2);
			ctx->tx_info = cpu_to_le32(tx_info);
			ctx->deq =
				cpu_to_le64((dma_addr_t)vdev->ep[epi]->enqueue |
					    vdev->ep[epi]->cycle_state);
		}
	}

	last_ctx = fls(add_flags) - 1;

        /* See section 4.6.6 - A0 = 1; A1 = D0 = D1 = 0 */
	vdev->in_ctx->icc.add_flags = cpu_to_le32(add_flags);
        vdev->in_ctx->icc.add_flags |= cpu_to_le32(SLOT_FLAG);
        vdev->in_ctx->icc.add_flags &= cpu_to_le32(~EP0_FLAG);
        vdev->in_ctx->icc.drop_flags &= cpu_to_le32(~(SLOT_FLAG | EP0_FLAG));

	/* Don't issue the command if there's no endpoints to update. */
        if (vdev->in_ctx->icc.add_flags == cpu_to_le32(SLOT_FLAG) &&
            vdev->in_ctx->icc.drop_flags == 0)
		return 0;

	vdev->in_ctx->slot.dev_info &= cpu_to_le32(~LAST_CTX_MASK);
	vdev->in_ctx->slot.dev_info |= cpu_to_le32(LAST_CTX(last_ctx));

        /* Issue Configure Endpoint Command */
	memset(&trb, 0, sizeof(union xhci_trb));
	xhci_write_64((dma_addr_t)vdev->in_ctx, &trb.event_cmd.cmd_trb);
	trb.event_cmd.flags = TRB_TYPE(TRB_CONFIG_EP) |
		SLOT_ID_FOR_TRB(vdev->slot_id);
	xhci_print_trb(xhci, &trb, "Request  ConfigureEndpoint");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response ConfigureEndpoint");
	xhci_virtdev_zero_in_ctx(vdev);

	return ret;
}

static int xhci_virtdev_deconfigure(struct xhci_virtual_device *vdev)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	union xhci_trb trb;
	int ret;

        /* Issue Deconfigure Endpoint Command */
	memset(&trb, 0, sizeof(union xhci_trb));
	xhci_write_64((dma_addr_t)vdev->in_ctx, &trb.event_cmd.cmd_trb);
	trb.event_cmd.flags = TRB_TYPE(TRB_CONFIG_EP) | TRB_DC |
		SLOT_ID_FOR_TRB(vdev->slot_id);
	xhci_print_trb(xhci, &trb, "Request  DeconfigureEndpoint");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response DeconfigureEndpoint");
	xhci_virtdev_zero_in_ctx(vdev);

	return ret;
}

static int xhci_virtdev_init(struct xhci_virtual_device *vdev)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	struct usb_device *top_dev;
	int max_packets;
	u32 route = 0, dev_info, dev_info2, tt_info, ep_info2, tx_info;
	bool on_hs_hub = false;
	int hs_slot_id = 0;

	/*
	 * Find the root hub port this device is under, also determine SlotID
	 * of possible external HS hub a LS/FS device could be connected to.
	 */
	for (top_dev = vdev->udev; top_dev->parent && top_dev->parent->parent;
	     top_dev = top_dev->parent) {
		if (top_dev->parent->descriptor->bDeviceClass == USB_CLASS_HUB)
			route = (route << 4) | (top_dev->portnr & 0xf);
		if (top_dev->parent->descriptor->bDeviceClass == USB_CLASS_HUB &&
		    top_dev->parent->speed != USB_SPEED_LOW &&
		    top_dev->parent->speed != USB_SPEED_FULL) {
			on_hs_hub |= true;
			if (!hs_slot_id) {
				struct xhci_virtual_device *vhub =
					xhci_find_virtdev(xhci, top_dev->parent);
				hs_slot_id = vhub->slot_id;
			}
		}
	}

	/* 4.3.3 3) Initalize Input Slot Context */
	dev_info = LAST_CTX(1);
	switch (vdev->udev->speed) {
	case USB_SPEED_SUPER:
		dev_info |= SLOT_SPEED_SS;
		max_packets = 512;
		break;
	case USB_SPEED_HIGH:
		dev_info |= SLOT_SPEED_HS;
		max_packets = 64;
		break;
	case USB_SPEED_FULL:
		dev_info |= SLOT_SPEED_FS;
		max_packets = 64;
		break;
	case USB_SPEED_LOW:
		dev_info |= SLOT_SPEED_LS;
		max_packets = 8;
		break;
	default:
		max_packets = 0;
		break;
	}
	dev_info |= route;
	dev_info2 = ROOT_HUB_PORT(top_dev->portnr);
	tt_info = 0;

	/* Is this a LS/FS device under an external HS hub? */
	if (on_hs_hub && (vdev->udev->speed == USB_SPEED_FULL ||
			  vdev->udev->speed == USB_SPEED_LOW)) {
		dev_info |= DEV_MTT;
		tt_info |= (top_dev->portnr << 8) | hs_slot_id;
	}

	vdev->in_ctx->slot.dev_info = cpu_to_le32(dev_info);
	vdev->in_ctx->slot.dev_info2 = cpu_to_le32(dev_info2);
	vdev->in_ctx->slot.tt_info = cpu_to_le32(tt_info);

	/* 4.3.3 4) Initalize Transfer Ring */
	vdev->ep[0] = xhci_get_endpoint_ring(xhci);
	if (!vdev->ep[0])
		return -ENOMEM;
	xhci_ring_init(vdev->ep[0], NUM_TRANSFER_TRBS, TYPE_CTRL);

	/* 4.3.3 5) Initialize Input Control Endpoint 0 Context */
	ep_info2 = EP_TYPE(CTRL_EP) | MAX_BURST(0) | ERROR_COUNT(3);
	ep_info2 |= MAX_PACKET(max_packets);
	tx_info = AVG_TRB_LENGTH_FOR_EP(8);
	vdev->in_ctx->ep[0].ep_info2 = cpu_to_le32(ep_info2);
	vdev->in_ctx->ep[0].tx_info = cpu_to_le32(tx_info);
	vdev->in_ctx->ep[0].deq = cpu_to_le64((dma_addr_t)vdev->ep[0]->enqueue |
					      vdev->ep[0]->cycle_state);

        /* 4.3.3 6+7) Initalize and Set Device Context Base Address Array */
	xhci->dcbaa[vdev->slot_id] = cpu_to_le64((dma_addr_t)vdev->out_ctx);

	return 0;
}

static int xhci_virtdev_setup(struct xhci_virtual_device *vdev,
			      enum xhci_setup_dev setup)
{
	struct xhci_hcd *xhci = to_xhci_hcd(vdev->udev->host);
	union xhci_trb trb;
	int ret;

	/*
	 * If this is the first Set Address since device
	 * plug-in then initialize Slot Context
	 */
	if (!vdev->in_ctx->slot.dev_info)
		xhci_virtdev_init(vdev);
	else {
		/* Otherwise, update Control Ring Dequeue pointer */
		vdev->in_ctx->ep[0].deq =
			cpu_to_le64((dma_addr_t)vdev->ep[0]->enqueue |
				    vdev->ep[0]->cycle_state);
		/*
		 * FS devices have MaxPacketSize0 of 8 or 64, we start
		 * with 64. If assumtion was wrong, fix it up here.
		 */
		if (vdev->udev->speed == USB_SPEED_FULL &&
		    vdev->udev->maxpacketsize == PACKET_SIZE_8) {
			u32 info = le32_to_cpu(vdev->in_ctx->ep[0].ep_info2);
			info &= ~MAX_PACKET_MASK;
			info |= MAX_PACKET(8);
			vdev->in_ctx->ep[0].ep_info2 = cpu_to_le32(info);
		}
	}

	vdev->in_ctx->icc.add_flags = cpu_to_le32(SLOT_FLAG | EP0_FLAG);
	vdev->in_ctx->icc.drop_flags = 0;

        /* Issue Address Device Command */
	memset(&trb, 0, sizeof(union xhci_trb));
	xhci_write_64((dma_addr_t)vdev->in_ctx, &trb.event_cmd.cmd_trb);
	trb.event_cmd.flags = TRB_TYPE(TRB_ADDR_DEV) |
		SLOT_ID_FOR_TRB(vdev->slot_id);
	if (setup == SETUP_CONTEXT_ONLY)
		trb.event_cmd.flags |= TRB_BSR;
	xhci_print_trb(xhci, &trb, "Request  AddressDevice");
	xhci_issue_command(xhci, &trb);
	ret = xhci_wait_for_event(xhci, TRB_COMPLETION, &trb);
	xhci_print_trb(xhci, &trb, "Response AddressDevice");
	xhci_virtdev_zero_in_ctx(vdev);

	return ret;
}

static int xhci_virtdev_set_address(struct xhci_virtual_device *vdev)
{
        return xhci_virtdev_setup(vdev, SETUP_CONTEXT_ADDRESS);
}

static int xhci_virtdev_enable(struct xhci_virtual_device *vdev)
{
        return xhci_virtdev_setup(vdev, SETUP_CONTEXT_ONLY);
}

static int xhci_virtdev_attach(struct xhci_hcd *xhci, struct usb_device *udev)
{
	struct xhci_virtual_device *vdev;
	int ret;

	vdev = xhci_alloc_virtdev(xhci, udev);
	if (IS_ERR(vdev))
		return PTR_ERR(vdev);

	ret = xhci_virtdev_enable_slot(vdev);
	if (ret)
		return ret;

	return xhci_virtdev_enable(vdev);
}

int xhci_virtdev_detach(struct xhci_virtual_device *vdev)
{
	xhci_virtdev_deconfigure(vdev);
	xhci_virtdev_disable_slot(vdev);
	xhci_free_virtdev(vdev);

	return 0;
}

static int xhci_submit_normal(struct usb_device *udev, unsigned long pipe,
			      void *buffer, int length)
{
	struct usb_host *host = udev->host;
	struct xhci_hcd *xhci = to_xhci_hcd(host);
	struct xhci_virtual_device *vdev;
	union xhci_trb trb;
	u8 epaddr = (usb_pipein(pipe) ? USB_DIR_IN : USB_DIR_OUT) |
		usb_pipeendpoint(pipe);
	u8 epi = xhci_get_endpoint_index(epaddr, usb_pipetype(pipe));
	int ret;

	vdev = xhci_find_virtdev(xhci, udev);
	if (!vdev)
		return -ENODEV;

	dev_dbg(xhci->dev, "%s udev %p vdev %p slot %u state %u epi %u in_ctx %p out_ctx %p\n",
		__func__, udev, vdev, vdev->slot_id,
		GET_SLOT_STATE(le32_to_cpu(vdev->out_ctx->slot.dev_state)), epi,
		vdev->in_ctx, vdev->out_ctx);

	/* pass ownership of data buffer to device */
	dma_sync_single_for_device((unsigned long)buffer, length,
				   usb_pipein(pipe) ?
				   DMA_FROM_DEVICE : DMA_TO_DEVICE);

	/* Normal TRB */
	memset(&trb, 0, sizeof(union xhci_trb));
	trb.event_cmd.cmd_trb = cpu_to_le64((dma_addr_t)buffer);
	/* FIXME: TD remainder */
	trb.event_cmd.status = TRB_LEN(length) | TRB_INTR_TARGET(0);
	trb.event_cmd.flags = TRB_TYPE(TRB_NORMAL) | TRB_IOC;
	if (usb_pipein(pipe))
		trb.event_cmd.flags |= TRB_ISP;
	xhci_print_trb(xhci, &trb, "Request  Normal");
	xhci_virtdev_issue_transfer(vdev, epi, &trb, true);
	ret = xhci_wait_for_event(xhci, TRB_TRANSFER, &trb);
	xhci_print_trb(xhci, &trb, "Response Normal");

	/* Regain ownership of data buffer from device */
	dma_sync_single_for_cpu((unsigned long)buffer, length,
				usb_pipein(pipe) ?
				DMA_FROM_DEVICE : DMA_TO_DEVICE);

	switch (ret) {
	case -COMP_SHORT_TX:
		udev->status = 0;
		udev->act_len = length - EVENT_TRB_LEN(trb.event_cmd.status);
		return 0;
	case 0:
		udev->status = 0;
		udev->act_len = 0;
		return 0;
	case -ETIMEDOUT:
		udev->status = USB_ST_CRC_ERR;
		return -1;
	default:
		return -1;
	}
}

static int xhci_submit_control(struct usb_device *udev, unsigned long pipe,
			       void *buffer, int length, struct devrequest *req)
{
	struct usb_host *host = udev->host;
	struct xhci_hcd *xhci = to_xhci_hcd(host);
	struct xhci_virtual_device *vdev;
	union xhci_trb trb;
	u16 typeReq = (req->requesttype << 8) | req->request;
	int ret;

	dev_dbg(xhci->dev, "%s req %u (%#x), type %u (%#x), value %u (%#x), index %u (%#x), length %u (%#x)\n",
		__func__, req->request, req->request,
		req->requesttype, req->requesttype,
		le16_to_cpu(req->value), le16_to_cpu(req->value),
		le16_to_cpu(req->index), le16_to_cpu(req->index),
		le16_to_cpu(req->length), le16_to_cpu(req->length));

	vdev = xhci_find_virtdev(xhci, udev);
	if (!vdev) {
		ret = xhci_virtdev_attach(xhci, udev);
		if (ret)
			return ret;
		vdev = xhci_find_virtdev(xhci, udev);
	}
	if (!vdev)
		return -ENODEV;

	dev_dbg(xhci->dev, "%s udev %p vdev %p slot %u state %u epi %u in_ctx %p out_ctx %p\n",
		__func__, udev, vdev, vdev->slot_id,
		GET_SLOT_STATE(le32_to_cpu(vdev->out_ctx->slot.dev_state)), 0,
		vdev->in_ctx, vdev->out_ctx);

	if (req->request == USB_REQ_SET_ADDRESS)
		return xhci_virtdev_set_address(vdev);
	if (req->request == USB_REQ_SET_CONFIGURATION) {
		ret = xhci_virtdev_configure(vdev, le16_to_cpu(req->value));
		if (ret)
			return ret;
	}

	/* Pass ownership of data buffer to device */
	dma_sync_single_for_device((unsigned long)buffer, length,
				   (req->requesttype & USB_DIR_IN) ?
				   DMA_FROM_DEVICE : DMA_TO_DEVICE);

	/* Setup TRB */
	memset(&trb, 0, sizeof(union xhci_trb));
	trb.generic.field[0] = le16_to_cpu(req->value) << 16 |
		req->request << 8 | req->requesttype;
	trb.generic.field[1] = le16_to_cpu(req->length) << 16 |
		le16_to_cpu(req->index);
	trb.event_cmd.status = TRB_LEN(8) | TRB_INTR_TARGET(0);
	trb.event_cmd.flags = TRB_TYPE(TRB_SETUP) | TRB_IDT;
	if (xhci->hci_version == 0x100 && length > 0) {
		if (req->requesttype & USB_DIR_IN)
			trb.event_cmd.flags |= TRB_TX_TYPE(TRB_DATA_IN);
		else
			trb.event_cmd.flags |= TRB_TX_TYPE(TRB_DATA_OUT);
	}
	xhci_print_trb(xhci, &trb, "Request  Setup ");
	xhci_virtdev_issue_transfer(vdev, 0, &trb, false);

	/* Data TRB */
	if (length > 0) {
		memset(&trb, 0, sizeof(union xhci_trb));
		trb.event_cmd.cmd_trb = cpu_to_le64((dma_addr_t)buffer);
		/* FIXME: TD remainder */
		trb.event_cmd.status = TRB_LEN(length) | TRB_INTR_TARGET(0);
		trb.event_cmd.flags = TRB_TYPE(TRB_DATA) | TRB_IOC;
		if (req->requesttype & USB_DIR_IN)
			trb.event_cmd.flags |= TRB_ISP | TRB_DIR_IN;
		xhci_print_trb(xhci, &trb, "Request  Data  ");
		xhci_virtdev_issue_transfer(vdev, 0, &trb, false);
	}

	/* Status TRB */
	memset(&trb, 0, sizeof(union xhci_trb));
	trb.event_cmd.status = TRB_INTR_TARGET(0);
	if (length > 0 && req->requesttype & USB_DIR_IN)
		trb.event_cmd.flags = 0;
	else
		trb.event_cmd.flags = TRB_DIR_IN;
	trb.event_cmd.flags |= TRB_TYPE(TRB_STATUS) | TRB_IOC;
	xhci_print_trb(xhci, &trb, "Request  Status");
	xhci_virtdev_issue_transfer(vdev, 0, &trb, true);

	if (length > 0 && req->requesttype & USB_DIR_IN) {
		ret = xhci_wait_for_event(xhci, TRB_TRANSFER, &trb);
		xhci_print_trb(xhci, &trb, "Response Data  ");
		if (ret == -COMP_SHORT_TX)
			length -= EVENT_TRB_LEN(trb.event_cmd.status);
		else if (ret < 0)
			goto dma_regain;
	}

	ret = xhci_wait_for_event(xhci, TRB_TRANSFER, &trb);
	xhci_print_trb(xhci, &trb, "Response Status");

dma_regain:
	/* Regain ownership of data buffer from device */
	dma_sync_single_for_cpu((unsigned long)buffer, length,
				(req->requesttype & USB_DIR_IN) ?
				DMA_FROM_DEVICE : DMA_TO_DEVICE);
	if (ret < 0)
		return ret;

	/*
	 * usb core doesn't notify us about device events on
	 * external Hubs, track it ourselves.
	 */
	if (typeReq == GetHubDescriptor)
		xhci_virtdev_update_hub_device(vdev, buffer, length);
	if (typeReq == ClearPortFeature &&
	    cpu_to_le16(req->value) == USB_PORT_FEAT_C_CONNECTION)
		xhci_virtdev_update_hub_status(vdev, le16_to_cpu(req->index));

	return length;
}

/*
 * xHCI host controller driver
 */

static void xhci_dma_alloc(struct xhci_hcd *xhci)
{
	size_t sz_sp, sz_spa, sz_dca, sz_cmd, sz_evt, sz_erst, sz_ep;
	u64 reg64;
	void *p;
	int i, num_ep;

	/* Scratchpad buffers: PAGE_SIZE aligned */
	sz_sp = ALIGN(xhci->num_sp * xhci->page_size, xhci->page_size);
	/* Device Context Array: 64B aligned */
	sz_dca = ALIGN(xhci->max_slots * sizeof(u64), 64);
	/* Command Ring: 64B aligned */
	sz_cmd = ALIGN(NUM_COMMAND_TRBS * sizeof(union xhci_trb), 64);
	/* Event Ring: 64B aligned */
	sz_evt = NUM_EVENT_SEGM *
		ALIGN(NUM_EVENT_TRBS * sizeof(union xhci_trb), 64);
	/* Event Ring Segment Table: 64B aligned */
	sz_erst = ALIGN(NUM_EVENT_SEGM * sizeof(struct xhci_erst_entry), 64);
	/* Scratchpad Buffer Array: 64B aligned */
	sz_spa = ALIGN(xhci->num_sp * sizeof(u64), 64);

	xhci->dma_size = sz_sp + sz_spa + sz_dca + sz_cmd + sz_evt + sz_erst;

	/*
	 * Endpoint Transfer Ring: 16B aligned
	 *
	 * We allocate up to MAX_EP_RINGS from the rest of the PAGE
	 * for virtual devices to pick-up (and return) for endpoint trbs.
	 */
	sz_ep = ALIGN(NUM_TRANSFER_TRBS * sizeof(union xhci_trb), 16);

	num_ep = PAGE_ALIGN(xhci->dma_size) -
		MIN_EP_RINGS * sz_ep - xhci->dma_size;
	num_ep /= sz_ep;
	num_ep = max(MAX_EP_RINGS, MIN_EP_RINGS + num_ep);
	xhci->dma_size += num_ep * sz_ep;

	p = xhci->dma = dma_alloc_coherent(xhci->dma_size, DMA_ADDRESS_BROKEN);
	memset(xhci->dma, 0, xhci->dma_size);

	xhci->sp = p; p += sz_sp;
	xhci->dcbaa = p; p += sz_dca;
	xhci->cmd_ring.trbs = p; p += sz_cmd;
	xhci->event_ring.trbs = p; p += sz_evt;
	xhci->event_erst = p; p += sz_erst;
	xhci->sp_array = p; p += sz_spa;

	xhci->rings = xzalloc(num_ep * sizeof(*xhci->rings));
	for (i = 0; i < num_ep; i++) {
		xhci->rings[i].trbs = p;
		p += sz_ep;
		xhci_put_endpoint_ring(xhci, &xhci->rings[i]);
	}

	/* Setup Scratchpad Buffer Array and Base Address in Device Context */
	reg64 = cpu_to_le64((dma_addr_t)xhci->sp);
	for (i = 0; i < xhci->num_sp; i++, reg64 += xhci->page_size)
		xhci->sp_array[i] = cpu_to_le64(reg64);
	if (xhci->num_sp)
		xhci->dcbaa[0] = cpu_to_le64((dma_addr_t)xhci->sp_array);

	/* Setup Event Ring Segment Table and Event Ring */
	reg64 = (dma_addr_t)&xhci->event_ring.trbs[0];
	xhci->event_erst[0].seg_addr = cpu_to_le64(reg64);
	xhci->event_erst[0].seg_size = cpu_to_le32(NUM_EVENT_TRBS);
	xhci_ring_init(&xhci->event_ring, NUM_EVENT_TRBS, TYPE_EVENT);

	/* Setup Command Ring */
	xhci_ring_init(&xhci->cmd_ring, NUM_COMMAND_TRBS, TYPE_COMMAND);
}

static int xhci_halt(struct xhci_hcd *xhci)
{
	u32 reg = readl(&xhci->op_regs->status);
	u32 mask = ~XHCI_IRQS;

	if (!(reg & STS_HALT))
		mask &= ~CMD_RUN;

	/* disable any IRQs and begin halting process */
	reg = readl(&xhci->op_regs->command);
	reg &= mask;
	writel(reg, &xhci->op_regs->command);

	return xhci_handshake(&xhci->op_regs->status,
			      STS_HALT, STS_HALT, XHCI_MAX_HALT_USEC);
}

static int xhci_reset(struct xhci_hcd *xhci)
{
	u32 reg;
	int ret;

	reg = readl(&xhci->op_regs->command);
	reg |= CMD_RESET;
	writel(reg, &xhci->op_regs->command);

	ret = xhci_handshake(&xhci->op_regs->command,
			     CMD_RESET, 0, 10 * SECOND / USECOND);
	if (ret) {
		dev_err(xhci->dev, "failed to reset\n");
		return ret;
	}

        return 0;
}

static int xhci_start(struct xhci_hcd *xhci)
{
	u32 reg;
	int ret, i;

	reg = readl(&xhci->op_regs->command);
	reg |= CMD_RUN;
	writel(reg, &xhci->op_regs->command);

	ret = xhci_handshake(&xhci->op_regs->status,
			     STS_HALT, 0, XHCI_MAX_HALT_USEC);
        if (ret) {
                dev_err(xhci->dev, "failed to start\n");
		return ret;
	}

	/* Ensure ports are powered-off */
	for (i = 0; i < xhci->num_usb_ports; i++)
		xhci_hub_port_power(xhci, i, false);

        return 0;
}

static int xhci_init(struct usb_host *host)
{
	struct xhci_hcd *xhci = to_xhci_hcd(host);
	u32 reg;
	u64 reg64;
	int i, tmp, ret;

	ret = xhci_halt(xhci);
	if (ret)
		return ret;

	ret = xhci_reset(xhci);
	if (ret)
		return ret;

	tmp = readl(&xhci->op_regs->page_size);
	for (i = 0; i < 16; i++) {
		if ((0x1 & tmp) != 0)
			break;
		tmp >>= 1;
	}
	if (i < 16)
		tmp = (1 << (i+12));
	else
		dev_warn(xhci->dev, "unsupported page size %d\n", tmp);
	/* Use 4K pages, since that's common and the minimum the HC supports */
	xhci->page_shift = 12;
	xhci->page_size = 1 << xhci->page_shift;

	xhci->rootdev = 0;
	xhci->num_sp = HCS_MAX_SCRATCHPAD(xhci->hcs_params2);
	xhci->max_slots = HCS_MAX_SLOTS(xhci->hcs_params1);
	xhci_dma_alloc(xhci);

	ret = xhci_hub_setup_ports(xhci);
	if (ret)
		return ret;

	/*
	 * Program the Max Device Slots Enabled (MaxSlotsEn) field in the
	 * CONFIG register (5.4.7) with the max number of slots HC can handle.
	 */
	reg = readl(&xhci->op_regs->config_reg);
	reg |= (xhci->max_slots & HCS_SLOTS_MASK);
	writel(reg, &xhci->op_regs->config_reg);

	/*
	 * Program the Device Context Base Address Array Pointer (DCBAAP)
	 * register (5.4.6) with a 64-bit address pointing to where the
	 * Device Context Base Address Array is located.
	 */
	xhci_write_64((dma_addr_t)xhci->dcbaa, &xhci->op_regs->dcbaa_ptr);

	/*
	 * Define the Command Ring Dequeue Pointer by programming the
	 * Command Ring Control Register (5.4.5) with a 64-bit address
	 * pointing to the starting address of the first TRB of the Command
	 * Ring.
	 */
	reg64 = xhci_read_64(&xhci->op_regs->cmd_ring);
	reg64 = (reg64 & (u64)CMD_RING_RSVD_BITS) |
		((dma_addr_t)&xhci->cmd_ring.trbs[0] &
		 ~(dma_addr_t)CMD_RING_RSVD_BITS) |
		xhci->cmd_ring.cycle_state;
	xhci_write_64(reg64, &xhci->op_regs->cmd_ring);

	reg = readl(&xhci->cap_regs->db_off) & DBOFF_MASK;
	xhci->dba = (void __iomem *)xhci->cap_regs + reg;
	xhci->ir_set = &xhci->run_regs->ir_set[0];

	reg64 = (dma_addr_t)&xhci->event_ring.trbs[0] &
		~(dma_addr_t)CMD_RING_RSVD_BITS;
	xhci->event_erst[i].seg_addr = cpu_to_le64(reg64);
	xhci->event_erst[i].seg_size = cpu_to_le32(NUM_EVENT_TRBS);
	reg = readl(&xhci->ir_set->erst_size) & ~ERST_SIZE_MASK;
	writel(reg | NUM_EVENT_SEGM, &xhci->ir_set->erst_size);
	xhci_set_event_dequeue(xhci);

	reg64 = xhci_read_64(&xhci->ir_set->erst_base);
	reg64 &= ERST_PTR_MASK;
	reg64 |= (dma_addr_t)xhci->event_erst &
		~(dma_addr_t)CMD_RING_RSVD_BITS;
	xhci_write_64(reg64, &xhci->ir_set->erst_base);

	/*
	 * Write the USBCMD (5.4.1) to turn the host controller ON via
	 * setting the Run/Stop (R/S) bit to ‘1’. This operation allows the
	 * xHC to begin accepting doorbell references.
	 */

	return xhci_start(xhci);

	/*
	 * At this point, the host controller is up and running and the Root
	 * Hub ports (5.4.8) will begin reporting device connects, etc.,
	 * and system software may begin enumerating devices.
	 * System software may follow the procedures described in section 4.3,
	 * to enumerate attached devices.
	 *
	 * USB2 (LS/FS/HS) devices require the port reset process to advance
	 * the port to the Enabled state. Once USB2 ports are Enabled, the port
	 * is active with SOFs occurring on the port, but the Pipe Schedules
	 * have not yet been enabled.
	 *
	 * SS ports automatically advance to the Enabled state if a successful
	 * device attach is detected.
	 */
}

static int xhci_submit_bulk_msg(struct usb_device *dev, unsigned long pipe,
				void *buffer, int length, int timeout)
{
	return xhci_submit_normal(dev, pipe, buffer, length);
}

static int xhci_submit_control_msg(struct usb_device *dev, unsigned long pipe,
	   void *buffer, int length, struct devrequest *setup, int timeout)
{
	struct xhci_hcd *xhci = to_xhci_hcd(dev->host);

	/* Catch Root Hub requests */
	if (usb_pipedevice(pipe) == xhci->rootdev) {
		if (xhci->rootdev == 0)
			dev->speed = USB_SPEED_HIGH;
		return xhci_hub_control(dev, pipe, buffer, length, setup);
	}

	return xhci_submit_control(dev, pipe, buffer, length, setup);
}

static int xhci_submit_int_msg(struct usb_device *dev, unsigned long pipe,
			       void *buffer, int length, int interval)
{
	struct xhci_hcd *xhci = to_xhci_hcd(dev->host);

	dev_err(xhci->dev, "Interrupt messages not supported\n");

	return -ENOTSUPP;
}

static int xhci_detect(struct device_d *dev)
{
	struct xhci_hcd *xhci = dev->priv;

	return usb_host_detect(&xhci->host);
}

int xhci_register(struct device_d *dev, struct xhci_data *data)
{
	struct usb_host	*host;
	struct xhci_hcd *xhci;

	xhci = xzalloc(sizeof(*xhci));
	host = &xhci->host;
	INIT_LIST_HEAD(&xhci->vdev_list);
	xhci->dev = dev;
	xhci->cap_regs = data->regs;
	xhci->op_regs = (void __iomem *)xhci->cap_regs +
		HC_LENGTH(readl(&xhci->cap_regs->hc_capbase));
	xhci->run_regs = (void __iomem *)xhci->cap_regs +
		(readl(&xhci->cap_regs->run_regs_off) & RTSOFF_MASK);
	/* Cache read-only capability registers */
	xhci->hcs_params1 = readl(&xhci->cap_regs->hcs_params1);
	xhci->hcs_params2 = readl(&xhci->cap_regs->hcs_params2);
	xhci->hcs_params3 = readl(&xhci->cap_regs->hcs_params3);
	xhci->hcc_capbase = readl(&xhci->cap_regs->hc_capbase);
	xhci->hci_version = HC_VERSION(xhci->hcc_capbase);
	xhci->hcc_params = readl(&xhci->cap_regs->hcc_params);

	host->hw_dev = dev;
	host->init = xhci_init;
	host->submit_int_msg = xhci_submit_int_msg;
	host->submit_control_msg = xhci_submit_control_msg;
	host->submit_bulk_msg = xhci_submit_bulk_msg;

	dev->priv = xhci;
	dev->detect = xhci_detect;

	usb_register_host(host);

	dev_info(dev, "USB xHCI %x.%02x\n",
		 xhci->hci_version >> 8, xhci->hci_version & 0xff);

	return 0;
}

/*
 * xHCI platform driver
 */

static int xhci_probe(struct device_d *dev)
{
	struct xhci_data data = {};

	data.regs = dev_request_mem_region(dev, 0);

	return xhci_register(dev, &data);
}

static void xhci_remove(struct device_d *dev)
{
	struct xhci_hcd *xhci = dev->priv;
	xhci_halt(xhci);
}

static struct driver_d xhci_driver = {
	.name  = "xHCI",
	.probe = xhci_probe,
	.remove = xhci_remove,
};
device_platform_driver(xhci_driver);
