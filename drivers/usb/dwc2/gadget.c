// SPDX-License-Identifier: GPL-2.0-or-later
#include <dma.h>
#include <linux/usb/gadget.h>
#include "dwc2.h"

#define to_dwc2 gadget_to_dwc2
#define dwc2_set_bit(d, r, b)	dwc2_writel(d, (b) | dwc2_readl(d, r), r)
#define dwc2_clear_bit(d, r, b)	dwc2_writel(d, ~(b) & dwc2_readl(d, r), r)

#define spin_lock(lock)
#define spin_unlock(lock)
#define local_irq_save(flags)(void)(flags)
#define local_irq_restore(flags) (void)(flags)
#define spin_lock_irqsave(lock, flags) (void)(flags)
#define spin_unlock_irqrestore(lock, flags) (void)(flags)

static void kill_all_requests(struct dwc2 *, struct dwc2_ep *, int);

static inline struct dwc2_ep *index_to_ep(struct dwc2 *dwc2,
					  unsigned int ep, int in)
{
	if (ep >= DWC2_MAX_EPS_CHANNELS)
		return NULL;
	if (in)
		return dwc2->eps_in[ep];
	else
		return dwc2->eps_out[ep];
}

static inline int using_dma(struct dwc2 *dwc2)
{
	/* Only buffer dma is supported */
	return 1;
}

static void dwc2_dcfg_set_addr(struct dwc2 *dwc2, int addr)
{
	u32 dcfg = dwc2_readl(dwc2, DCFG);

	dcfg &= ~DCFG_DEVADDR_MASK;
	dcfg |= (addr << DCFG_DEVADDR_SHIFT) & DCFG_DEVADDR_MASK;
	dwc2_writel(dwc2, dcfg, DCFG);
}

/**
 * dwc2_hsotg_ctrl_epint - enable/disable an endpoint irq
 * @hsotg: The device state
 * @ep: The endpoint index
 * @dir_in: True if direction is in.
 * @en: The enable value, true to enable
 *
 * Set or clear the mask for an individual endpoint's interrupt
 * request.
 */
static void dwc2_hsotg_ctrl_epint(struct dwc2 *dwc2,
				  unsigned int ep, unsigned int dir_in,
				  unsigned int en)
{
	unsigned long flags;
	u32 bit = 1 << ep;
	u32 daint;

	if (!dir_in)
		bit <<= 16;

	local_irq_save(flags);
	daint = dwc2_readl(dwc2, DAINTMSK);
	if (en)
		daint |= bit;
	else
		daint &= ~bit;
	dwc2_writel(dwc2, daint, DAINTMSK);
	local_irq_restore(flags);

}

/**
 * get_ep_head - return the first request on the endpoint
 * @hs_ep: The controller endpoint to get
 *
 * Get the first request on the endpoint.
 */
static struct dwc2_request *get_ep_head(struct dwc2_ep *hs_ep)
{
	return list_first_entry_or_null(&hs_ep->queue, struct dwc2_request,
					queue);
}

/**
 * get_ep_limit - get the maximum data legnth for this endpoint
 * @hs_ep: The endpoint
 *
 * Return the maximum data that can be queued in one go on a given endpoint
 * so that transfers that are too long can be split.
 */
static unsigned int get_ep_limit(struct dwc2_ep *hs_ep)
{
	int index = hs_ep->epnum;
	unsigned int maxsize;
	unsigned int maxpkt;

	if (index != 0) {
		maxsize = DXEPTSIZ_XFERSIZE_LIMIT + 1;
		maxpkt = DXEPTSIZ_PKTCNT_LIMIT + 1;
	} else {
		maxsize = 64 + 64;
		if (hs_ep->dir_in)
			maxpkt = DIEPTSIZ0_PKTCNT_LIMIT + 1;
		else
			maxpkt = 2;
	}

	/* we made the constant loading easier above by using +1 */
	maxpkt--;
	maxsize--;

	/*
	 * constrain by packet count if maxpkts*pktsize is greater
	 * than the length register size.
	 */

	if ((maxpkt * hs_ep->ep.maxpacket) < maxsize)
		maxsize = maxpkt * hs_ep->ep.maxpacket;

	return maxsize;
}

static u32 dwc2_read_frameno(struct dwc2 *dwc2)
{
	u32 dsts;

	dsts = dwc2_readl(dwc2, DSTS);
	dsts &= DSTS_SOFFN_MASK;
	dsts >>= DSTS_SOFFN_SHIFT;

	return dsts;
}

/**
 * dwc2_gadget_incr_frame_num - Increments the targeted frame number.
 * @hs_ep: The endpoint
 *
 * This function will also check if the frame number overruns DSTS_SOFFN_LIMIT.
 * If an overrun occurs it will wrap the value and set the frame_overrun flag.
 */
static inline void dwc2_gadget_incr_frame_num(struct dwc2_ep *hs_ep)
{
	hs_ep->target_frame += hs_ep->interval;
	if (hs_ep->target_frame > DSTS_SOFFN_LIMIT) {
		hs_ep->frame_overrun = true;
		hs_ep->target_frame &= DSTS_SOFFN_LIMIT;
	} else {
		hs_ep->frame_overrun = false;
	}
}

/**
 * dwc2_gadget_target_frame_elapsed - Checks target frame
 * @hs_ep: The driver endpoint to check
 *
 * Returns 1 if targeted frame elapsed. If returned 1 then we need to drop
 * corresponding transfer.
 */
static bool dwc2_gadget_target_frame_elapsed(struct dwc2_ep *hs_ep)
{
	struct dwc2 *dwc2 = hs_ep->dwc2;
	u32 target_frame = hs_ep->target_frame;
	u32 current_frame = dwc2->frame_number;
	bool frame_overrun = hs_ep->frame_overrun;

	if (!frame_overrun && current_frame >= target_frame)
		return true;

	if (frame_overrun && current_frame >= target_frame &&
	    ((current_frame - target_frame) < DSTS_SOFFN_LIMIT / 2))
		return true;

	return false;
}


/**
 * dwc2_gadget_start_req - start a USB request from an endpoint's queue
 * @dwc2: The controller state.
 * @hs_ep: The endpoint to process a request for
 * @hs_req: The request to start.
 * @continuing: True if we are doing more for the current request.
 *
 * Start the given request running by setting the endpoint registers
 * appropriately, and writing any data to the FIFOs.
 */
static void dwc2_gadget_start_req(struct dwc2 *dwc2,
				  struct dwc2_ep *hs_ep,
				  struct dwc2_request *hs_req,
				  bool continuing)
{
	struct usb_request *ureq = &hs_req->req;
	int index = hs_ep->epnum;
	int dir_in = hs_ep->dir_in;
	u32 epctrl_reg;
	u32 epsize_reg;
	u32 epsize;
	u32 ctrl;
	unsigned int length;
	unsigned int packets;
	unsigned int maxreq;
	unsigned int dma_reg;

	if (index != 0) {
		if (hs_ep->req && !continuing) {
			dwc2_err(dwc2, "%s: active request\n", __func__);
			WARN_ON(1);
			return;
		} else if (hs_ep->req != hs_req && continuing) {
			dwc2_err(dwc2,
				 "%s: continue different req\n", __func__);
			WARN_ON(1);
			return;
		}
	}

	dma_reg = dir_in ? DIEPDMA(index) : DOEPDMA(index);
	epctrl_reg = dir_in ? DIEPCTL(index) : DOEPCTL(index);
	epsize_reg = dir_in ? DIEPTSIZ(index) : DOEPTSIZ(index);

	dwc2_dbg(dwc2, "%s: DxEPCTL=0x%08x, ep %d, dir %s\n",
		__func__, dwc2_readl(dwc2, epctrl_reg), index,
		hs_ep->dir_in ? "in" : "out");

	/* If endpoint is stalled, we will restart request later */
	ctrl = dwc2_readl(dwc2, epctrl_reg);

	if (index && ctrl & DXEPCTL_STALL) {
		dwc2_warn(dwc2, "%s: ep%d is stalled\n", __func__, index);
		return;
	}

	length = ureq->length - ureq->actual;
	dwc2_dbg(dwc2, "ureq->length:%d ureq->actual:%d\n",
		ureq->length, ureq->actual);

	maxreq = get_ep_limit(hs_ep);

	if (length > maxreq) {
		int round = maxreq % hs_ep->ep.maxpacket;

		dwc2_dbg(dwc2, "%s: length %d, max-req %d, r %d\n",
			__func__, length, maxreq, round);

		/* round down to multiple of packets */
		if (round)
			maxreq -= round;

		length = maxreq;
	}

	if (length)
		packets = DIV_ROUND_UP(length, hs_ep->ep.maxpacket);
	else
		packets = 1;	/* send one packet if length is zero. */

	if (dir_in && index != 0)
		if (hs_ep->isochronous)
			epsize = DXEPTSIZ_MC(packets);
		else
			epsize = DXEPTSIZ_MC(1);
	else
		epsize = 0;

	/*
	 * zero length packet should be programmed on its own and should not
	 * be counted in DIEPTSIZ.PktCnt with other packets.
	 */
	if (dir_in && ureq->zero && !continuing) {
		/* Test if zlp is actually required. */
		if ((ureq->length >= hs_ep->ep.maxpacket) &&
		    !(ureq->length % hs_ep->ep.maxpacket))
			hs_ep->send_zlp = 1;
	}

	epsize |= DXEPTSIZ_PKTCNT(packets);
	epsize |= DXEPTSIZ_XFERSIZE(length);

	dwc2_dbg(dwc2, "%s: %d@%d/%d, 0x%08x => 0x%08x\n",
		__func__, packets, length, ureq->length, epsize, epsize_reg);

	/* store the request as the current one we're doing */
	hs_ep->req = hs_req;

	/* write size / packets */
	dwc2_writel(dwc2, epsize, epsize_reg);

	if (using_dma(dwc2) && !continuing && (length != 0)) {
		/*
		 * write DMA address to control register, buffer
		 * already synced by dwc2_ep_queue().
		 */

		dwc2_writel(dwc2, ureq->dma, dma_reg);

		dwc2_dbg(dwc2, "%s: 0x%pad => 0x%08x\n",
			 __func__, &ureq->dma, dma_reg);
	}

	if (hs_ep->isochronous && hs_ep->interval == 1) {
		hs_ep->target_frame = dwc2_read_frameno(dwc2);
		dwc2_gadget_incr_frame_num(hs_ep);

		if (hs_ep->target_frame & 0x1)
			ctrl |= DXEPCTL_SETODDFR;
		else
			ctrl |= DXEPCTL_SETEVENFR;
	}

	ctrl |= DXEPCTL_EPENA;	/* ensure ep enabled */

	dwc2_dbg(dwc2, "%s: ep0 state:%d\n", __func__, dwc2->ep0_state);

	/* For Setup request do not clear NAK */
	if (!(index == 0 && dwc2->ep0_state == DWC2_EP0_SETUP))
		ctrl |= DXEPCTL_CNAK;	/* clear NAK set by core */

	dwc2_dbg(dwc2, "%s: DxEPCTL=0x%08x\n", __func__, ctrl);
	dwc2_writel(dwc2, ctrl, epctrl_reg);

	/*
	 * set these, it seems that DMA support increments past the end
	 * of the packet buffer so we need to calculate the length from
	 * this information.
	 */
	hs_ep->size_loaded = length;
	hs_ep->last_load = ureq->actual;

	/*
	 * Note, trying to clear the NAK here causes problems with transmit
	 * on the S3C6400 ending up with the TXFIFO becoming full.
	 */

	/* check ep is enabled */
	if (!(dwc2_readl(dwc2, epctrl_reg) & DXEPCTL_EPENA))
		dwc2_dbg(dwc2,
			"ep%d: failed to become enabled (DXEPCTL=0x%08x)?\n",
			 index, dwc2_readl(dwc2, epctrl_reg));

	dwc2_dbg(dwc2, "%s: DXEPCTL=0x%08x\n",
		__func__, dwc2_readl(dwc2, epctrl_reg));

	/* enable ep interrupts */
	dwc2_hsotg_ctrl_epint(dwc2, hs_ep->epnum, hs_ep->dir_in, 1);
}

/* conversion functions */
static inline struct dwc2_request *our_req(struct usb_request *req)
{
	return container_of(req, struct dwc2_request, req);
}

static inline struct dwc2_ep *our_ep(struct usb_ep *ep)
{
	return container_of(ep, struct dwc2_ep, ep);
}


/**
 * dwc2_ep0_mps - turn max packet size into register setting
 * @mps: The maximum packet size in bytes.
 */
static u32 dwc2_ep0_mps(unsigned int mps)
{
	switch (mps) {
	case 64:
		return D0EPCTL_MPS_64;
	case 32:
		return D0EPCTL_MPS_32;
	case 16:
		return D0EPCTL_MPS_16;
	case 8:
		return D0EPCTL_MPS_8;
	}

	/* bad max packet size, warn and return invalid result */
	WARN_ON(1);
	return (u32)-1;
}

/**
 * dwc2_set_ep_maxpacket - set endpoint's max-packet field
 * @dwc2: The driver state.
 * @ep: The index number of the endpoint
 * @mps: The maximum packet size in bytes
 * @mc: The multicount value
 * @dir_in: True if direction is in.
 *
 * Configure the maximum packet size for the given endpoint, updating
 * the hardware control registers to reflect this.
 */
static void dwc2_set_ep_maxpacket(struct dwc2 *dwc2,
				  unsigned int ep, unsigned int mps,
				  unsigned int mc, unsigned int dir_in)
{
	struct dwc2_ep *hs_ep;
	u32 reg;

	hs_ep = index_to_ep(dwc2, ep, dir_in);
	if (!hs_ep)
		return;

	if (ep == 0) {
		u32 mps_bytes = mps;

		/* EP0 is a special case */
		mps = dwc2_ep0_mps(mps_bytes);
		if (mps > 3)
			goto bad_mps;
		hs_ep->ep.maxpacket = mps_bytes;
		hs_ep->mc = 1;
	} else {
		if (mps > 1024)
			goto bad_mps;
		hs_ep->mc = mc;
		if (mc > 3)
			goto bad_mps;
		hs_ep->ep.maxpacket = mps;
	}

	if (dir_in) {
		reg = dwc2_readl(dwc2, DIEPCTL(ep));
		reg &= ~DXEPCTL_MPS_MASK;
		reg |= mps;
		dwc2_writel(dwc2, reg, DIEPCTL(ep));
	} else {
		reg = dwc2_readl(dwc2, DOEPCTL(ep));
		reg &= ~DXEPCTL_MPS_MASK;
		reg |= mps;
		dwc2_writel(dwc2, reg, DOEPCTL(ep));
	}

	return;

bad_mps:
	dwc2_err(dwc2, "ep%d: bad mps of %d\n", ep, mps);
}

static int dwc2_ep_enable(struct usb_ep *ep,
			  const struct usb_endpoint_descriptor *desc)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	unsigned long flags;
	unsigned int index = hs_ep->epnum;
	u32 epctrl_reg;
	u32 epctrl;
	u32 mps;
	u32 mc;
	u32 mask;
	unsigned int dir_in;
	unsigned int i, val, size;
	int ret = 0;
	unsigned char ep_type;

	dwc2_dbg(dwc2,
		"%s: ep %s: a 0x%02x, attr 0x%02x, mps 0x%04x, intr %d\n",
		__func__, ep->name, desc->bEndpointAddress, desc->bmAttributes,
		desc->wMaxPacketSize, desc->bInterval);

	/* not to be called for EP0 */
	if (index == 0) {
		dwc2_err(dwc2, "%s: called for EP 0\n", __func__);
		return -EINVAL;
	}

	dir_in = (desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ? 1 : 0;
	if (dir_in != hs_ep->dir_in) {
		dwc2_err(dwc2, "%s: direction mismatch!\n", __func__);
		return -EINVAL;
	}

	ep_type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	mps = usb_endpoint_maxp(desc) & USB_ENDPOINT_MAXP_MASK;
	mc = usb_endpoint_maxp_mult(desc);

	/* note, we handle this here instead of dwc2_set_ep_maxpacket */
	epctrl_reg = dir_in ? DIEPCTL(index) : DOEPCTL(index);
	epctrl = dwc2_readl(dwc2, epctrl_reg);

	dwc2_dbg(dwc2, "%s: read DxEPCTL=0x%08x from 0x%08x\n",
		__func__, epctrl, epctrl_reg);

	spin_lock_irqsave(&dwc2->lock, flags);

	epctrl &= ~(DXEPCTL_EPTYPE_MASK | DXEPCTL_MPS_MASK);
	epctrl |= DXEPCTL_MPS(mps);

	/*
	 * mark the endpoint as active, otherwise the core may ignore
	 * transactions entirely for this endpoint
	 */
	epctrl |= DXEPCTL_USBACTEP;

	/* update the endpoint state */
	dwc2_set_ep_maxpacket(dwc2, index, mps, mc, dir_in);

	/* default, set to non-periodic */
	hs_ep->isochronous = 0;
	hs_ep->periodic = 0;
	hs_ep->halted = 0;
	hs_ep->interval = desc->bInterval;

	switch (ep_type) {
	case USB_ENDPOINT_XFER_ISOC:
		epctrl |= DXEPCTL_EPTYPE_ISO;
		epctrl |= DXEPCTL_SETEVENFR;
		hs_ep->isochronous = 1;
		hs_ep->interval = 1 << (desc->bInterval - 1);
		hs_ep->target_frame = TARGET_FRAME_INITIAL;
		if (dir_in) {
			hs_ep->periodic = 1;
			mask = dwc2_readl(dwc2, DIEPMSK);
			mask |= DIEPMSK_NAKMSK;
			dwc2_writel(dwc2, mask, DIEPMSK);
		} else {
			mask = dwc2_readl(dwc2, DOEPMSK);
			mask |= DOEPMSK_OUTTKNEPDISMSK;
			dwc2_writel(dwc2, mask, DOEPMSK);
		}
		break;

	case USB_ENDPOINT_XFER_BULK:
		epctrl |= DXEPCTL_EPTYPE_BULK;
		break;

	case USB_ENDPOINT_XFER_INT:
		if (dir_in)
			hs_ep->periodic = 1;

		if (dwc2->gadget.speed == USB_SPEED_HIGH)
			hs_ep->interval = 1 << (desc->bInterval - 1);

		epctrl |= DXEPCTL_EPTYPE_INTERRUPT;
		break;

	case USB_ENDPOINT_XFER_CONTROL:
		epctrl |= DXEPCTL_EPTYPE_CONTROL;
		break;
	}

	/*
	 * if the hardware has dedicated fifos, we must give each IN EP
	 * a unique tx-fifo even if it is non-periodic.
	 */
	if (dir_in && dwc2->dedicated_fifos) {
		unsigned int fifo_count = dwc2_tx_fifo_count(dwc2);
		u32 fifo_index = 0;
		u32 fifo_size = UINT_MAX;

		size = hs_ep->ep.maxpacket * hs_ep->mc;
		for (i = 1; i <= fifo_count; i++) {
			if (dwc2->fifo_map & (1 << i))
				continue;

			val = dwc2_readl(dwc2, DPTXFSIZN(i));
			val = (val >> FIFOSIZE_DEPTH_SHIFT) * 4;
			if (val < size)
				continue;
			/* Search for smallest acceptable fifo */
			if (val < fifo_size) {
				fifo_size = val;
				fifo_index = i;
			}
		}
		if (!fifo_index) {
			dwc2_err(dwc2,
				"%s: No suitable fifo found\n", __func__);
			ret = -ENOMEM;
			goto error;
		}
		epctrl &= ~(DXEPCTL_TXFNUM_LIMIT << DXEPCTL_TXFNUM_SHIFT);
		epctrl |= DXEPCTL_TXFNUM(fifo_index);
		dwc2->fifo_map |= 1 << fifo_index;
		hs_ep->fifo_index = fifo_index;
		hs_ep->fifo_size = fifo_size;
	}

	/* for non control endpoints, set PID to D0 */
	if (index && !hs_ep->isochronous)
		epctrl |= DXEPCTL_SETD0PID;

	dwc2_dbg(dwc2, "%s: write DxEPCTL=0x%08x\n",
		__func__, epctrl);

	dwc2_writel(dwc2, epctrl, epctrl_reg);
	dwc2_dbg(dwc2, "%s: read DxEPCTL=0x%08x\n",
		__func__, dwc2_readl(dwc2, epctrl_reg));

	/* enable the endpoint interrupt */
	dwc2_hsotg_ctrl_epint(dwc2, index, dir_in, 1);

error:
	spin_unlock_irqrestore(&dwc2->lock, flags);

	return ret;
}

static void dwc2_ep_stop_xfr(struct dwc2 *dwc2, struct dwc2_ep *hs_ep)
{
	int in = hs_ep->dir_in;
	int epnum = hs_ep->epnum;
	u32 epctl_reg = in ? DIEPCTL(epnum) : DOEPCTL(epnum);
	u32 epint_reg = in ? DIEPINT(epnum) : DOEPINT(epnum);

	dwc2_dbg(dwc2, "%s: stopping transfer on %s\n", __func__,
		hs_ep->name);

	if (in) {
		if (dwc2->dedicated_fifos || hs_ep->periodic) {
			dwc2_set_bit(dwc2, epctl_reg, DXEPCTL_SNAK);
			/* Wait for Nak effect */
			if (dwc2_wait_bit_set(dwc2, epint_reg,
						    DXEPINT_INEPNAKEFF, 100))
				dwc2_warn(dwc2,
					 "%s: timeout DIEPINT.NAKEFF\n",
					 __func__);
		} else {
			dwc2_set_bit(dwc2, DCTL, DCTL_SGNPINNAK);
			/* Wait for Nak effect */
			if (dwc2_wait_bit_set(dwc2, GINTSTS,
						    GINTSTS_GINNAKEFF, 100))
				dwc2_warn(dwc2,
					 "%s: timeout GINTSTS.GINNAKEFF\n",
					 __func__);
		}
	} else {
		if (!(dwc2_readl(dwc2, GINTSTS) & GINTSTS_GOUTNAKEFF))
			dwc2_set_bit(dwc2, DCTL, DCTL_SGOUTNAK);

		/* Wait for global nak to take effect */
		if (dwc2_wait_bit_set(dwc2, GINTSTS,
					    GINTSTS_GOUTNAKEFF, 100))
			dwc2_warn(dwc2, "%s: timeout GINTSTS.GOUTNAKEFF\n",
				 __func__);
	}

	/* Disable ep */
	dwc2_set_bit(dwc2, epctl_reg, DXEPCTL_EPDIS | DXEPCTL_SNAK);

	/* Wait for ep to be disabled */
	if (dwc2_wait_bit_set(dwc2, epint_reg, DXEPINT_EPDISBLD, 100))
		dwc2_warn(dwc2, "%s: timeout DOEPCTL.EPDisable\n", __func__);

	/* Clear EPDISBLD interrupt */
	dwc2_set_bit(dwc2, epint_reg, DXEPINT_EPDISBLD);

	if (in) {
		unsigned short fifo_index;

		if (dwc2->dedicated_fifos || hs_ep->periodic)
			fifo_index = hs_ep->fifo_index;
		else
			fifo_index = 0;

		dwc2_flush_tx_fifo(dwc2, fifo_index);

		/* Clear Global In NP NAK in Shared FIFO for non periodic ep */
		if (!dwc2->dedicated_fifos && !hs_ep->periodic)
			dwc2_set_bit(dwc2, DCTL, DCTL_CGNPINNAK);

	} else {
		/* Remove global NAKs */
		dwc2_set_bit(dwc2, DCTL, DCTL_CGOUTNAK);
	}
}

static int dwc2_ep_disable(struct usb_ep *ep)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	int dir_in = hs_ep->dir_in;
	int index = hs_ep->epnum;
	u32 epctrl_reg = dir_in ? DIEPCTL(index) : DOEPCTL(index);
	u32 ctrl;

	dwc2_dbg(dwc2, "%s(ep %p)\n", __func__, ep);

	if (index == 0) {
		dwc2_err(dwc2, "%s: called for ep0\n", __func__);
		return -EINVAL;
	}

	ctrl = dwc2_readl(dwc2, epctrl_reg);
	if (ctrl & DXEPCTL_EPENA)
		dwc2_ep_stop_xfr(dwc2, hs_ep);

	ctrl &= ~DXEPCTL_EPENA;
	ctrl &= ~DXEPCTL_USBACTEP;
	ctrl |= DXEPCTL_SNAK;

	dwc2_dbg(dwc2, "%s: DxEPCTL=0x%08x\n", __func__, ctrl);
	dwc2_writel(dwc2, ctrl, epctrl_reg);

	/* disable endpoint interrupts */
	dwc2_hsotg_ctrl_epint(dwc2, index, hs_ep->dir_in, 0);

	/* terminate all requests with shutdown */
	kill_all_requests(dwc2, hs_ep, -ESHUTDOWN);

	dwc2->fifo_map &= ~(1 << hs_ep->fifo_index);
	hs_ep->fifo_index = 0;
	hs_ep->fifo_size = 0;

	return 0;
}

static struct usb_request *dwc2_ep_alloc_req(struct usb_ep *ep)
{
	struct dwc2_request *req;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return NULL;

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void dwc2_ep_free_req(struct usb_ep *ep, struct usb_request *req)
{
	struct dwc2_request *hs_req = our_req(req);

	kfree(hs_req);
}

/**
 * dwc2_start_next_request - Starts next request from ep queue
 * @hs_ep: Endpoint structure
 *
 * If queue is empty and EP is ISOC-OUT - unmasks OUTTKNEPDIS which is masked
 * in its handler. Hence we need to unmask it here to be able to do
 * resynchronization.
 */
static void dwc2_start_next_request(struct dwc2_ep *hs_ep)
{
	u32 mask;
	struct dwc2 *dwc2 = hs_ep->dwc2;
	int dir_in = hs_ep->dir_in;
	struct dwc2_request *hs_req;
	u32 epmsk_reg = dir_in ? DIEPMSK : DOEPMSK;

	dwc2_dbg(dwc2, "%s: next req\n", __func__);

	if (!list_empty(&hs_ep->queue)) {
		hs_req = get_ep_head(hs_ep);
		dwc2_gadget_start_req(dwc2, hs_ep, hs_req, false);
		return;
	}
	if (!hs_ep->isochronous)
		return;

	if (dir_in) {
		dwc2_dbg(dwc2, "%s: No more ISOC-IN requests\n", __func__);
	} else {
		dwc2_dbg(dwc2, "%s: No more ISOC-OUT requests\n", __func__);
		mask = dwc2_readl(dwc2, epmsk_reg);
		mask |= DOEPMSK_OUTTKNEPDISMSK;
		dwc2_writel(dwc2, mask, epmsk_reg);
	}
}

static int dwc2_ep_queue(struct usb_ep *ep, struct usb_request *req)
{

	struct dwc2_request *hs_req = our_req(req);
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	bool first;

	dwc2_dbg(dwc2, "%s: req %p: %d@%p, noi=%d, zero=%d, snok=%d\n",
		ep->name, req, req->length, req->buf, req->no_interrupt,
		req->zero, req->short_not_ok);

	/* initialise status of the request */
	INIT_LIST_HEAD(&hs_req->queue);
	req->actual = 0;
	req->status = -EINPROGRESS;

	/* Don't queue ISOC request if length greater than mps*mc */
	if (hs_ep->isochronous &&
	    req->length > (hs_ep->mc * hs_ep->ep.maxpacket)) {
		dwc2_err(dwc2, "req length > maxpacket*mc\n");
		return -EINVAL;
	}

	if ((long)req->buf & 3)
		dwc2_err(dwc2, "dma buffer not aligned\n");

	req->dma = dma_map_single(dwc2->dev, req->buf, req->length,
			hs_ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	if (dma_mapping_error(dwc2->dev, req->dma)) {
		dwc2_err(dwc2, "failed to map buffer\n");
		return -EFAULT;
	}

	first = list_empty(&hs_ep->queue);
	list_add_tail(&hs_req->queue, &hs_ep->queue);

	/* Change EP direction if status phase request is after data out */
	if (hs_ep->epnum == 0 && !req->length && !hs_ep->dir_in &&
		dwc2->ep0_state == DWC2_EP0_DATA_OUT)
		hs_ep->dir_in = 1;

	if (first) {
		if (!hs_ep->isochronous) {
			dwc2_gadget_start_req(dwc2, hs_ep, hs_req, false);
			return 0;
		}

		/* Update current frame number value. */
		dwc2->frame_number = dwc2_read_frameno(dwc2);
		while (dwc2_gadget_target_frame_elapsed(hs_ep)) {
			dwc2_gadget_incr_frame_num(hs_ep);
			/* Update current frame number value once more as it
			 * changes here.
			 */
			dwc2->frame_number = dwc2_read_frameno(dwc2);
		}

		if (hs_ep->target_frame != TARGET_FRAME_INITIAL)
			dwc2_gadget_start_req(dwc2, hs_ep, hs_req, false);
	}
	return 0;
}

static int dwc2_ep_dequeue(struct usb_ep *ep, struct usb_request *req)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	dwc2_warn(dwc2, "%s\n", __func__);
	return -EOPNOTSUPP;
}

static int dwc2_ep_set_halt(struct usb_ep *ep, int value)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	dwc2_warn(dwc2, "%s\n", __func__);
	return -EOPNOTSUPP;
}

static int dwc2_ep_set_wedge(struct usb_ep *ep)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	dwc2_warn(dwc2, "%s\n", __func__);
	return -EOPNOTSUPP;
}

static int dwc2_ep_fifo_status(struct usb_ep *ep)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	dwc2_warn(dwc2, "%s\n", __func__);
	return -EOPNOTSUPP;
}

static void dwc2_ep_fifo_flush(struct usb_ep *ep)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;
	dwc2_warn(dwc2, "%s\n", __func__);
}

static const struct usb_ep_ops dwc2_ep_ops = {
	.enable		= dwc2_ep_enable,
	.disable	= dwc2_ep_disable,
	.alloc_request	= dwc2_ep_alloc_req,
	.free_request	= dwc2_ep_free_req,
	.queue		= dwc2_ep_queue,
	.dequeue	= dwc2_ep_dequeue,
	.set_halt	= dwc2_ep_set_halt,
	.set_wedge	= dwc2_ep_set_wedge,
	.fifo_status	= dwc2_ep_fifo_status,
	.fifo_flush	= dwc2_ep_fifo_flush,
};

static void dwc2_program_zlp(struct dwc2 *dwc2, struct dwc2_ep *hs_ep)
{
	u32 ctrl;
	u8 index = hs_ep->epnum;
	u32 epctl_reg = hs_ep->dir_in ? DIEPCTL(index) : DOEPCTL(index);
	u32 epsiz_reg = hs_ep->dir_in ? DIEPTSIZ(index) : DOEPTSIZ(index);

	if (hs_ep->dir_in)
		dwc2_dbg(dwc2, "Sending zero-length packet on ep%d\n", index);
	else
		dwc2_dbg(dwc2, "Receiving zero-length packet on ep%d\n", index);

	dwc2_writel(dwc2, DXEPTSIZ_MC(1) | DXEPTSIZ_PKTCNT(1) |
		    DXEPTSIZ_XFERSIZE(0),
		    epsiz_reg);

	ctrl = dwc2_readl(dwc2, epctl_reg);
	ctrl |= DXEPCTL_CNAK;  /* clear NAK set by core */
	ctrl |= DXEPCTL_EPENA; /* ensure ep enabled */
	ctrl |= DXEPCTL_USBACTEP;
	dwc2_writel(dwc2, ctrl, epctl_reg);
}

/**
 * windex_to_ep - convert control wIndex value to endpoint
 * @dwc2: The driver state.
 * @windex: The control request wIndex field (in host order).
 *
 * Convert the given wIndex into a pointer to an driver endpoint
 * structure, or return NULL if it is not a valid endpoint.
 */
static struct dwc2_ep *windex_to_ep(struct dwc2 *dwc2, u16 windex)
{
	struct dwc2_ep *ep;
	int dir = (windex & USB_DIR_IN) ? 1 : 0;
	int idx = windex & 0x7F;

	if (windex >= 0x100)
		return NULL;

	if (idx >= dwc2->num_eps)
		return NULL;

	ep = index_to_ep(dwc2, idx, dir);

	if (idx && ep->dir_in != dir)
		return NULL;

	return ep;
}

/**
 * dwc2_hsotg_send_reply - send reply to control request
 * @dwc2: The device state
 * @buff: Buffer for request
 * @length: Length of reply.
 *
 * Create a request and queue it on the given endpoint. This is useful as
 * an internal method of sending replies to certain control requests, etc.
 */
static int dwc2_hsotg_send_reply(struct dwc2 *dwc2, void *buff, int length)
{
	struct dwc2_ep *ep0 = dwc2->eps_out[0];
	struct usb_request *req;
	int ret;

	if (length == 0 && (dwc2->ep0_state == DWC2_EP0_STATUS_IN ||
			    dwc2->ep0_state == DWC2_EP0_STATUS_OUT)) {
		dwc2_program_zlp(dwc2, ep0);
		return 0;
	}

	dwc2_dbg(dwc2, "%s: buff %p, len %d\n", __func__, buff, length);

	req = dwc2_ep_alloc_req(&ep0->ep);
	if (!req) {
		dwc2_warn(dwc2, "%s: cannot alloc req\n", __func__);
		return -ENOMEM;
	}

	req->buf = dwc2->ep0_buff;
	req->length = length;
	/*
	 * zero flag is for sending zlp in DATA IN stage. It has no impact on
	 * STATUS stage.
	 */
	req->zero = 0;
	req->complete = dwc2_ep_free_req;

	if (length)
		memcpy(req->buf, buff, length);

	ret = dwc2_ep_queue(&ep0->ep, req);
	if (ret) {
		dwc2_warn(dwc2, "%s: cannot queue req\n", __func__);
		return ret;
	}

	return 0;
}

/**
 * dwc2_process_req_status - process request GET_STATUS
 * @dwc2: The device state
 * @ctrl: USB control request
 */
static int dwc2_process_req_status(struct dwc2 *dwc2,
				   struct usb_ctrlrequest *ctrl)
{
	struct dwc2_ep *ep0 = dwc2->eps_out[0];
	struct dwc2_ep *ep;
	__le16 reply;
	u16 status;
	int ret;

	dwc2_dbg(dwc2, "%s: USB_REQ_GET_STATUS\n", __func__);

	if (!ep0->dir_in) {
		dwc2_warn(dwc2, "%s: direction out?\n", __func__);
		return -EINVAL;
	}

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		status = dwc2->is_selfpowered << USB_DEVICE_SELF_POWERED;
		status |= 0 << USB_DEVICE_REMOTE_WAKEUP;

		reply = cpu_to_le16(status);
		break;

	case USB_RECIP_INTERFACE:
		/* currently, the data result should be zero */
		reply = cpu_to_le16(0);
		break;

	case USB_RECIP_ENDPOINT:
		ep = windex_to_ep(dwc2, le16_to_cpu(ctrl->wIndex));
		if (!ep)
			return -ENOENT;

		reply = cpu_to_le16(ep->halted ? 1 : 0);
		break;

	default:
		return 0;
	}

	if (le16_to_cpu(ctrl->wLength) != 2)
		return -EINVAL;

	ret = dwc2_hsotg_send_reply(dwc2, &reply, 2);
	if (ret) {
		dwc2_err(dwc2, "%s: failed to send reply\n", __func__);
		return ret;
	}

	return 1;
}

/**
 * dwc2_process_req_feature - process request {SET,CLEAR}_FEATURE
 * @dwc2: The device state
 * @ctrl: USB control request
 */
static int dwc2_process_req_feature(struct dwc2 *dwc2,
				    struct usb_ctrlrequest *ctrl)
{
	struct dwc2_request *hs_req;
	bool set = (ctrl->bRequest == USB_REQ_SET_FEATURE);
	struct dwc2_ep *ep;
	int ret;
	bool halted;
	u32 recip;
	u16 wValue;
	u16 wIndex;

	dwc2_dbg(dwc2, "%s: %s_FEATURE\n", __func__, set ? "SET" : "CLEAR");

	wValue = le16_to_cpu(ctrl->wValue);
	wIndex = le16_to_cpu(ctrl->wIndex);
	recip = ctrl->bRequestType & USB_RECIP_MASK;

	switch (recip) {
	case USB_RECIP_DEVICE:
		switch (wValue) {
		case USB_DEVICE_REMOTE_WAKEUP:
		case USB_DEVICE_TEST_MODE:
			/* Not supported */
		default:
			return -ENOENT;
		}
		break;

	case USB_RECIP_ENDPOINT:
		ep = windex_to_ep(dwc2, wIndex);
		if (!ep)
			return -ENOENT;

		switch (wValue) {
		case USB_ENDPOINT_HALT:
			halted = ep->halted;

			dwc2_ep_set_halt(&ep->ep, set);

			ret = dwc2_hsotg_send_reply(dwc2, NULL, 0);
			if (ret) {
				dwc2_err(dwc2,
					"%s: failed to send reply\n", __func__);
				return ret;
			}

			/*
			 * we have to complete all requests for ep if it was
			 * halted, and the halt was cleared by CLEAR_FEATURE
			 */

			if (!set && halted) {
				/*
				 * If we have request in progress,
				 * then complete it
				 */
				if (ep->req) {
					hs_req = ep->req;
					ep->req = NULL;
					list_del_init(&hs_req->queue);
					if (hs_req->req.complete) {
						spin_unlock(&dwc2->lock);
						hs_req->req.complete(
							&ep->ep, &hs_req->req);
						spin_lock(&dwc2->lock);
					}
				}

				/* If we have pending request, then start it */
				if (!ep->req)
					dwc2_start_next_request(ep);
			}

			break;

		default:
			return -ENOENT;
		}
		break;
	default:
		return -ENOENT;
	}
	return 1;
}

static int dwc2_gadget_get_frame(struct usb_gadget *gadget)
{
	return dwc2_read_frameno(to_dwc2(gadget));
}

static void dwc2_core_disconnect(struct dwc2 *dwc2)
{
	u32 dctl;

	dctl = dwc2_readl(dwc2, DCTL);
	/* set the soft-disconnect bit */
	dctl |= DCTL_SFTDISCON;
	dwc2_writel(dwc2, dctl, DCTL);
}

static void dwc2_core_connect(struct dwc2 *dwc2)
{
	u32 dctl;

	dctl = dwc2_readl(dwc2, DCTL);
	/* clear the soft-disconnect bit */
	dctl &= ~DCTL_SFTDISCON;
	dwc2_writel(dwc2, dctl, DCTL);
}

static void dwc2_enqueue_setup(struct dwc2 *dwc2);

/**
 * dwc2_stall_ep0 - stall ep0
 * @dwc2: The device state
 *
 * Set stall for ep0 as response for setup request.
 */
static void dwc2_stall_ep0(struct dwc2 *dwc2)
{
	struct dwc2_ep *ep0 = dwc2->eps_out[0];
	u32 reg;
	u32 ctrl;

	dwc2_dbg(dwc2, "ep0 stall (dir=%d)\n", ep0->dir_in);
	reg = (ep0->dir_in) ? DIEPCTL0 : DOEPCTL0;

	/*
	 * DxEPCTL_Stall will be cleared by EP once it has
	 * taken effect, so no need to clear later.
	 */

	ctrl = dwc2_readl(dwc2, reg);
	ctrl |= DXEPCTL_STALL;
	ctrl |= DXEPCTL_CNAK;
	dwc2_writel(dwc2, ctrl, reg);

	dwc2_dbg(dwc2,
		"written DXEPCTL=0x%08x to %08x (DXEPCTL=0x%08x)\n",
		ctrl, reg, dwc2_readl(dwc2, reg));

	 /*
	  * complete won't be called, so we enqueue
	  * setup request here
	  */
	 dwc2_enqueue_setup(dwc2);
}

/**
 * dwc2_process_control - process a control request
 * @dwc2: The device state
 * @ctrl: The control request received
 *
 * The controller has received the SETUP phase of a control request, and
 * needs to work out what to do next (and whether to pass it on to the
 * gadget driver).
 */
static void dwc2_process_control(struct dwc2 *dwc2,
				 struct usb_ctrlrequest *ctrl)
{
	struct dwc2_ep *ep0 = dwc2->eps_out[0];
	int handled = false;
	int ret = 0;

	dwc2_dbg(dwc2,
		"ctrl Type=%02x, Req=%02x, V=%04x, I=%04x, L=%04x\n",
		ctrl->bRequestType, ctrl->bRequest, ctrl->wValue,
		ctrl->wIndex, ctrl->wLength);

	if (ctrl->wLength == 0) {
		ep0->dir_in = 1;
		dwc2->ep0_state = DWC2_EP0_STATUS_IN;
	} else if (ctrl->bRequestType & USB_DIR_IN) {
		ep0->dir_in = 1;
		dwc2->ep0_state = DWC2_EP0_DATA_IN;
	} else {
		ep0->dir_in = 0;
		dwc2->ep0_state = DWC2_EP0_DATA_OUT;
	}

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl->bRequest) {
		case USB_REQ_SET_ADDRESS:
			dwc2->connected = 1;
			dwc2_dcfg_set_addr(dwc2, le16_to_cpu(ctrl->wValue));
			ret = dwc2_hsotg_send_reply(dwc2, NULL, 0);
			handled = true;
			dwc2_info(dwc2, "new address %d\n", ctrl->wValue);
			break;

		case USB_REQ_GET_STATUS:
			ret = dwc2_process_req_status(dwc2, ctrl);
			handled = true;
			break;

		case USB_REQ_CLEAR_FEATURE:
		case USB_REQ_SET_FEATURE:
			ret = dwc2_process_req_feature(dwc2, ctrl);
			handled = true;
			break;
		}
	}

	/* as a fallback, try delivering it to the driver to deal with */
	if (!handled && dwc2->driver) {
		spin_unlock(&dwc2->lock);
		ret = dwc2->driver->setup(&dwc2->gadget, ctrl);
		spin_lock(&dwc2->lock);
		if (ret < 0)
			dwc2_dbg(dwc2, "driver->setup() ret %d\n", ret);
	}

	/*
	 * the request is either unhandlable, or is not formatted correctly
	 * so respond with a STALL for the status stage to indicate failure.
	 */

	if (ret < 0) {
		dwc2_dbg(dwc2, "unhandled ctrl request ");
		dwc2_stall_ep0(dwc2);
	}
}

/**
 * dwc2_complete_setup - completion of a setup transfer
 * @ep: The endpoint the request was on.
 * @req: The request completed.
 *
 * Called on completion of any requests the driver itself submitted for
 * EP0 setup packets
 */
static void dwc2_complete_setup(struct usb_ep *ep, struct usb_request *req)
{
	struct dwc2_ep *hs_ep = our_ep(ep);
	struct dwc2 *dwc2 = hs_ep->dwc2;

	if (req->status < 0) {
		dwc2_dbg(dwc2, "%s: failed %d\n", __func__, req->status);
		return;
	}

	spin_lock(&dwc2->lock);
	if (req->actual == 0)
		dwc2_enqueue_setup(dwc2);
	else
		dwc2_process_control(dwc2, req->buf);
	spin_unlock(&dwc2->lock);
}

static void dwc2_enqueue_setup(struct dwc2 *dwc2)
{
	struct usb_request *req = dwc2->ctrl_req;
	struct dwc2_request *hs_req = our_req(req);
	int ret;

	dwc2_dbg(dwc2, "%s: queueing setup request\n", __func__);

	req->zero = 0;
	req->length = 8;
	req->buf = dwc2->ctrl_buff;
	req->complete = dwc2_complete_setup;

	if (!list_empty(&hs_req->queue)) {
		dwc2_dbg(dwc2, "%s already queued???\n", __func__);
		return;
	}

	dwc2->eps_out[0]->dir_in = 0;
	dwc2->eps_out[0]->send_zlp = 0;
	dwc2->ep0_state = DWC2_EP0_SETUP;

	ret = dwc2_ep_queue(&dwc2->eps_out[0]->ep, req);
	if (ret < 0) {
		dwc2_err(dwc2, "%s: failed queue (%d)\n", __func__, ret);
		/*
		 * Don't think there's much we can do other than watch the
		 * driver fail.
		 */
	}
}

/**
 * dwc2_complete_request - complete a request given to us
 * @dwc2: The device state.
 * @hs_ep: The endpoint the request was on.
 * @hs_req: The request to complete.
 * @result: The result code (0 => Ok, otherwise errno)
 *
 * The given request has finished, so call the necessary completion
 * if it has one and then look to see if we can start a new request
 * on the endpoint.
 *
 * Note, expects the ep to already be locked as appropriate.
 */
static void dwc2_complete_request(struct dwc2 *dwc2, struct dwc2_ep *hs_ep,
				  struct dwc2_request *hs_req, int status)
{
	if (!hs_req) {
		dwc2_dbg(dwc2, "%s: nothing to complete?\n", __func__);
		return;
	}

	dwc2_dbg(dwc2, "complete: ep %p %s, req %p, %d => %p\n",
		hs_ep, hs_ep->ep.name, hs_req, status, hs_req->req.complete);

	/*
	 * only replace the status if we've not already set an error
	 * from a previous transaction
	 */

	if (hs_req->req.status == -EINPROGRESS)
		hs_req->req.status = status;

	dma_unmap_single(dwc2->dev, hs_req->req.dma, hs_req->req.length,
			hs_ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	hs_ep->req = NULL;
	list_del_init(&hs_req->queue);

	/*
	 * call the complete request with the locks off, just in case the
	 * request tries to queue more work for this endpoint.
	 */

	if (hs_req->req.complete) {
		spin_unlock(&dwc2->lock);
		hs_req->req.complete(&hs_ep->ep, &hs_req->req);
		spin_lock(&dwc2->lock);
	}

	/*
	 * Look to see if there is anything else to do. Note, the completion
	 * of the previous request may have caused a new request to be started
	 * so be careful when doing this.
	 */
	dwc2_dbg(dwc2, "%s: req %p, status %d\n", __func__, hs_ep->req, status);
	if (!hs_ep->req && status >= 0)
		dwc2_start_next_request(hs_ep);
}

/**
 * dwc2_handle_outdone - handle receiving OutDone/SetupDone from RXFIFO
 * @dwc2: The device instance
 * @epnum: The endpoint received from
 *
 * The RXFIFO has delivered an OutDone event, which means that the data
 * transfer for an OUT endpoint has been completed, either by a short
 * packet or by the finish of a transfer.
 */
static void dwc2_handle_outdone(struct dwc2 *dwc2, int epnum)
{
	u32 epsize = dwc2_readl(dwc2, DOEPTSIZ(epnum));
	struct dwc2_ep *hs_ep = dwc2->eps_out[epnum];
	struct dwc2_request *hs_req = hs_ep->req;
	struct usb_request *req = &hs_req->req;
	unsigned int size_left = DXEPTSIZ_XFERSIZE_GET(epsize);
	int result = 0;

	if (!hs_req) {
		dwc2_dbg(dwc2, "%s: no request active\n", __func__);
		return;
	}

	if (epnum == 0 && dwc2->ep0_state == DWC2_EP0_STATUS_OUT) {
		dwc2_dbg(dwc2, "zlp packet received\n");
		dwc2_complete_request(dwc2, hs_ep, hs_req, 0);
		dwc2_enqueue_setup(dwc2);
		return;
	}

	if (using_dma(dwc2)) {
		unsigned int size_done;

		/*
		 * Calculate the size of the transfer by checking how much
		 * is left in the endpoint size register and then working it
		 * out from the amount we loaded for the transfer.
		 *
		 * We need to do this as DMA pointers are always 32bit aligned
		 * so may overshoot/undershoot the transfer.
		 */
		size_done = hs_ep->size_loaded - size_left;
		size_done += hs_ep->last_load;

		req->actual = size_done;
	}

	if (req->actual < req->length && size_left == 0) {
		dwc2_gadget_start_req(dwc2, hs_ep, hs_req, true);
		return;
	}

	if (req->actual < req->length && req->short_not_ok) {
		dwc2_dbg(dwc2, "%s: got %d/%d (short not ok) => error\n",
			__func__, req->actual, req->length);

		/*
		 * todo - what should we return here? there's no one else
		 * even bothering to check the status.
		 */
	}

	if (epnum == 0 && dwc2->ep0_state == DWC2_EP0_DATA_OUT) {
		/* Move to STATUS IN */
		dwc2->eps_out[0]->dir_in = 1;
		dwc2->ep0_state = DWC2_EP0_STATUS_IN;
		dwc2_program_zlp(dwc2, dwc2->eps_out[0]);
	}

	/*
	 * Slave mode OUT transfers do not go through XferComplete so
	 * adjust the ISOC parity here.
	 */
#if 0
	if (!using_dma(dwc2)) {
		if (hs_ep->isochronous && hs_ep->interval == 1)
			dwc2_hsotg_change_ep_iso_parity(dwc2, DOEPCTL(epnum));
		else if (hs_ep->isochronous && hs_ep->interval > 1)
			dwc2_gadget_incr_frame_num(hs_ep);
	}

	/* Set actual frame number for completed transfers */
	if (hs_ep->isochronous)
		req->frame_number = dwc2->frame_number;
#endif

	dwc2_complete_request(dwc2, hs_ep, hs_req, result);
}

/**
 * kill_all_requests - remove all requests from the endpoint's queue
 * @dwc2: The device state.
 * @ep: The endpoint the requests may be on.
 * @result: The result code to use.
 *
 * Go through the requests on the given endpoint and mark them
 * completed with the given result code.
 */
static void kill_all_requests(struct dwc2 *dwc2, struct dwc2_ep *ep, int result)
{
	struct dwc2_request *req, *treq;
	unsigned int size;

	ep->req = NULL;

	list_for_each_entry_safe(req, treq, &ep->queue, queue)
		dwc2_complete_request(dwc2, ep, req, result);

	if (!dwc2->dedicated_fifos)
		return;
	size = (dwc2_readl(dwc2, DTXFSTS(ep->fifo_index)) & 0xffff) * 4;
	if (size < ep->fifo_size)
		dwc2_flush_tx_fifo(dwc2, ep->fifo_index);
}

/**
 * dwc2_gadget_disconnect - disconnect service
 * @dwc2: The device state.
 *
 * The device has been disconnected. Remove all current
 * transactions and signal the gadget driver that this
 * has happened.
 */
static void dwc2_gadget_disconnect(struct dwc2 *dwc2)
{

	unsigned int ep;

	if (!dwc2->connected)
		return;

	dwc2->connected = 0;

	/* all endpoints should be shutdown */
	for (ep = 0; ep < dwc2->num_eps; ep++) {
		if (dwc2->eps_in[ep])
			kill_all_requests(dwc2, dwc2->eps_in[ep], -ESHUTDOWN);
		if (dwc2->eps_out[ep])
			kill_all_requests(dwc2, dwc2->eps_out[ep], -ESHUTDOWN);
	}

	spin_unlock(&dwc2->lock);
	dwc2->driver->disconnect(&dwc2->gadget);
	spin_lock(&dwc2->lock);

	usb_gadget_set_state(&dwc2->gadget, USB_STATE_NOTATTACHED);
}

static void dwc2_gadget_setup_fifo(struct dwc2 *dwc2)
{
	unsigned int ep;
	unsigned int addr;
	u32 np_tx_fifo_size = dwc2->params.g_np_tx_fifo_size;
	u32 rx_fifo_size = dwc2->params.g_rx_fifo_size;
	u32 fifo_size = dwc2->hw_params.total_fifo_size;
	u32 *txfsz = dwc2->params.g_tx_fifo_size;
	u32 size, val;

	/* Reset fifo map if not correctly cleared during previous session */
	WARN_ON(dwc2->fifo_map);
	dwc2->fifo_map = 0;

	/* set RX/NPTX FIFO sizes */
	dwc2_writel(dwc2, rx_fifo_size, GRXFSIZ);
	size = rx_fifo_size << FIFOSIZE_STARTADDR_SHIFT;
	size |= np_tx_fifo_size << FIFOSIZE_DEPTH_SHIFT;
	dwc2_writel(dwc2, size, GNPTXFSIZ);

	/*
	 * arange all the rest of the TX FIFOs, as some versions of this
	 * block have overlapping default addresses. This also ensures
	 * that if the settings have been changed, then they are set to
	 * known values.
	 */

	/* start at the end of the GNPTXFSIZ, rounded up */
	addr = rx_fifo_size + np_tx_fifo_size;

	/*
	 * Configure fifos sizes from provided configuration and assign
	 * them to endpoints dynamically according to maxpacket size value of
	 * given endpoint.
	 */
	for (ep = 1; ep < DWC2_MAX_EPS_CHANNELS; ep++) {
		if (!txfsz[ep])
			continue;
		val = addr;
		val |= txfsz[ep] << FIFOSIZE_DEPTH_SHIFT;
		WARN_ONCE(addr + txfsz[ep] > fifo_size,
			  "insufficient fifo memory");
		addr += txfsz[ep];

		dwc2_writel(dwc2, val, DPTXFSIZN(ep));
		val = dwc2_readl(dwc2, DPTXFSIZN(ep));
	}

	dwc2_writel(dwc2, dwc2->hw_params.total_fifo_size |
		    addr << GDFIFOCFG_EPINFOBASE_SHIFT,
		    GDFIFOCFG);

	dwc2_flush_all_fifo(dwc2);
}

static void dwc2_gadget_interrupt_init(struct dwc2 *dwc2)
{
	/* unmask subset of endpoint interrupts */
	dwc2_writel(dwc2, DIEPMSK_TIMEOUTMSK | DIEPMSK_AHBERRMSK |
		    DIEPMSK_EPDISBLDMSK | DIEPMSK_XFERCOMPLMSK,
		    DIEPMSK);

	dwc2_writel(dwc2, DOEPMSK_SETUPMSK | DOEPMSK_AHBERRMSK |
		    DOEPMSK_EPDISBLDMSK | DOEPMSK_XFERCOMPLMSK,
		    DOEPMSK);

	dwc2_writel(dwc2, 0, DAINTMSK);
}

/**
 * dwc2_complete_in - complete IN transfer
 * @dwc2: The device state.
 * @hs_ep: The endpoint that has just completed.
 *
 * An IN transfer has been completed, update the transfer's state and then
 * call the relevant completion routines.
 */
static void dwc2_complete_in(struct dwc2 *dwc2, struct dwc2_ep *hs_ep)
{
	struct dwc2_request *hs_req = hs_ep->req;
	u32 epsize = dwc2_readl(dwc2, DIEPTSIZ(hs_ep->epnum));
	int size_left, size_done;

	if (!hs_req) {
		dwc2_dbg(dwc2, "XferCompl but no req\n");
		return;
	}

	/* Finish ZLP handling for IN EP0 transactions */
	if (hs_ep->epnum == 0 && dwc2->ep0_state == DWC2_EP0_STATUS_IN) {
		dwc2_dbg(dwc2, "zlp packet sent\n");
		/*
		 * While send zlp for DWC2_EP0_STATUS_IN EP direction was
		 * changed to IN. Change back to complete OUT transfer request
		 */
		hs_ep->dir_in = 0;
		dwc2_complete_request(dwc2, hs_ep, hs_req, 0);
		dwc2_enqueue_setup(dwc2);
		return;
	}

	/*
	 * Calculate the size of the transfer by checking how much is left
	 * in the endpoint size register and then working it out from
	 * the amount we loaded for the transfer.
	 *
	 * We do this even for DMA, as the transfer may have incremented
	 * past the end of the buffer (DMA transfers are always 32bit
	 * aligned).
	 */
	size_left = DXEPTSIZ_XFERSIZE_GET(epsize);

	size_done = hs_ep->size_loaded - size_left;
	size_done += hs_ep->last_load;

	if (hs_req->req.actual != size_done)
		dwc2_dbg(dwc2, "%s: adjusting size done %d => %d\n",
			__func__, hs_req->req.actual, size_done);

	hs_req->req.actual = size_done;
	dwc2_dbg(dwc2, "req->length:%d req->actual:%d req->zero:%d\n",
		hs_req->req.length, hs_req->req.actual, hs_req->req.zero);

	if (!size_left && hs_req->req.actual < hs_req->req.length) {
		dwc2_dbg(dwc2, "%s trying more for req...\n", __func__);
		dwc2_gadget_start_req(dwc2, hs_ep, hs_req, true);
		return;
	}

	/* Zlp for all endpoints, for ep0 only in DATA IN stage */
	if (hs_ep->send_zlp) {
		dwc2_program_zlp(dwc2, hs_ep);
		hs_ep->send_zlp = 0;
		/* transfer will be completed on next complete interrupt */
		return;
	}

	if (hs_ep->epnum == 0 && dwc2->ep0_state == DWC2_EP0_DATA_IN) {
		/* Move to STATUS OUT */
		dwc2->eps_out[0]->dir_in = 0;
		dwc2->ep0_state = DWC2_EP0_STATUS_OUT;
		dwc2_program_zlp(dwc2, dwc2->eps_out[0]);
		return;
	}

	dwc2_complete_request(dwc2, hs_ep, hs_req, 0);
}

/**
 * dwc2_core_gadget_init - issue softreset to the core
 * @dwc2: The device state
 * @usb_reset: Usb resetting flag
 *
 * Issue a soft reset to the core, and await the core finishing it.
 */
static void dwc2_core_gadget_init(struct dwc2 *dwc2, bool usb_reset)
{
	u32 intmsk;
	u32 dctl;
	u32 usbcfg;
	u32 dcfg;
	int ep;

	/* Kill any ep0 requests as controller will be reinitialized */
	kill_all_requests(dwc2, dwc2->eps_out[0], -ECONNRESET);

	if (!usb_reset) {
		if (dwc2_core_reset(dwc2))
			return;
	} else {
		/* all endpoints should be shutdown */
		for (ep = 1; ep < dwc2->num_eps; ep++) {
			if (dwc2->eps_in[ep])
				dwc2_ep_disable(&dwc2->eps_in[ep]->ep);
			if (dwc2->eps_out[ep])
				dwc2_ep_disable(&dwc2->eps_out[ep]->ep);
		}
	}

	/*
	 * we must now enable ep0 ready for host detection and then
	 * set configuration.
	 */

	/* keep other bits untouched (so e.g. forced modes are not lost) */
	usbcfg = dwc2_readl(dwc2, GUSBCFG);
	usbcfg &= ~GUSBCFG_TOUTCAL_MASK;
	usbcfg |= GUSBCFG_TOUTCAL(7);
	dwc2_writel(dwc2, usbcfg, GUSBCFG);

	dwc2_phy_init(dwc2, true);

	dwc2_gadget_setup_fifo(dwc2);

	if (!usb_reset)
		dwc2_core_disconnect(dwc2);

	dcfg = DCFG_EPMISCNT(1);

	if (dwc2->params.speed == DWC2_SPEED_PARAM_LOW) {
		dcfg |= DCFG_DEVSPD_LS;
	} else if (dwc2->params.speed == DWC2_SPEED_PARAM_FULL &&
		   dwc2->params.phy_type == DWC2_PHY_TYPE_PARAM_FS) {
		dcfg |= DCFG_DEVSPD_FS48;
	} else if (dwc2->params.speed == DWC2_SPEED_PARAM_FULL &&
		   dwc2->params.phy_type != DWC2_PHY_TYPE_PARAM_FS) {
		dcfg |= DCFG_DEVSPD_FS;
	}

	if (dwc2->params.ipg_isoc_en)
		dcfg |= DCFG_IPG_ISOC_SUPPORDED;

	dwc2_writel(dwc2, dcfg,  DCFG);

	/* Clear any pending OTG interrupts */
	dwc2_writel(dwc2, 0xffffffff, GOTGINT);

	/* Clear any pending interrupts */
	dwc2_writel(dwc2, 0xffffffff, GINTSTS);
	intmsk = GINTSTS_ERLYSUSP |
		GINTSTS_GOUTNAKEFF | GINTSTS_GINNAKEFF |
		GINTSTS_USBRST | GINTSTS_RESETDET |
		GINTSTS_ENUMDONE |
		GINTSTS_USBSUSP | GINTSTS_WKUPINT |
		GINTSTS_LPMTRANRCVD;

	intmsk |= GINTSTS_INCOMPL_SOIN | GINTSTS_INCOMPL_SOOUT;

	/* enable in and out endpoint interrupts */
	intmsk |= GINTSTS_OEPINT | GINTSTS_IEPINT;

	/*
	 * Enable the RXFIFO when in slave mode, as this is how we collect
	 * the data. In DMA mode, we get events from the FIFO but also
	 * things we cannot process, so do not use it.
	 */
	if (!using_dma(dwc2))
		intmsk |= GINTSTS_RXFLVL;

	if (!dwc2->params.external_id_pin_ctl)
		intmsk |= GINTSTS_CONIDSTSCHNG;

	dwc2_writel(dwc2, intmsk, GINTMSK);

	dwc2_gahbcfg_init(dwc2);

	/*
	 * If INTknTXFEmpMsk is enabled, it's important to disable ep interrupts
	 * when we have no data to transfer. Otherwise we get being flooded by
	 * interrupts.
	 */
	intmsk = DIEPMSK_EPDISBLDMSK | DIEPMSK_XFERCOMPLMSK |
		DIEPMSK_TIMEOUTMSK | DIEPMSK_AHBERRMSK;

	if (dwc2->dedicated_fifos && !using_dma(dwc2))
		intmsk |= DIEPMSK_TXFIFOEMPTY | DIEPMSK_INTKNTXFEMPMSK;

	dwc2_writel(dwc2, intmsk, DIEPMSK);

	/*
	 * don't need XferCompl, we get that from RXFIFO in slave mode. In
	 * DMA mode we may need this and StsPhseRcvd.
	 */
	intmsk = DOEPMSK_EPDISBLDMSK | DOEPMSK_AHBERRMSK | DOEPMSK_SETUPMSK;
	if (using_dma(dwc2))
		intmsk |= DIEPMSK_XFERCOMPLMSK |  DOEPMSK_STSPHSERCVDMSK;
	dwc2_writel(dwc2, intmsk, DOEPMSK);

	dwc2_writel(dwc2, 0, DAINTMSK);

	dwc2_dbg(dwc2, "EP0: DIEPCTL0=0x%08x, DOEPCTL0=0x%08x\n",
		dwc2_readl(dwc2, DIEPCTL0),
		dwc2_readl(dwc2, DOEPCTL0));

	/* Enable interrupts for EP0 in and out */
	dwc2_hsotg_ctrl_epint(dwc2, 0, 0, 1);
	dwc2_hsotg_ctrl_epint(dwc2, 0, 1, 1);
	dwc2_dbg(dwc2, "DAINTMSK=0x%08x\n", dwc2_readl(dwc2, DAINTMSK));

	if (!usb_reset) {
		dctl = dwc2_readl(dwc2, DCTL);
		dwc2_writel(dwc2, dctl | DCTL_PWRONPRGDONE, DCTL);

		udelay(10);  /* see openiboot */

		dctl = dwc2_readl(dwc2, DCTL);
		dwc2_writel(dwc2, dctl & ~DCTL_PWRONPRGDONE, DCTL);
	}

	dwc2_dbg(dwc2, "DCTL=0x%08x\n", dwc2_readl(dwc2, DCTL));

	/*
	 * DxEPCTL_USBActEp says RO in manual, but seems to be set by
	 * writing to the EPCTL register..
	 */
	dwc2_writel(dwc2, dwc2_ep0_mps(dwc2->eps_out[0]->ep.maxpacket) |
		    DXEPCTL_CNAK | DXEPCTL_EPENA |
		    DXEPCTL_USBACTEP, DOEPCTL0);

	/* enable, but don't activate EP0in */
	dwc2_writel(dwc2, dwc2_ep0_mps(dwc2->eps_out[0]->ep.maxpacket) |
		    DXEPCTL_USBACTEP, DIEPCTL0);

	/* clear global NAKs */
	dctl = dwc2_readl(dwc2, DCTL);
	dctl |= DCTL_CGOUTNAK | DCTL_CGNPINNAK;

	if (!usb_reset)
		dctl |= DCTL_SFTDISCON;
	dwc2_writel(dwc2, dctl, DCTL);

	/* must be at-least 3ms to allow bus to see disconnect */
	mdelay(3);
}

/**
 * dwc2_set_selfpowered - set if device is self/bus powered
 * @gadget: The usb gadget state
 * @is_selfpowered: Whether the device is self-powered
 *
 * Set if the device is self or bus powered.
 */
static int dwc2_set_selfpowered(struct usb_gadget *gadget, int is_selfpowered)
{
	struct dwc2 *dwc2 = to_dwc2(gadget);

	dwc2->is_selfpowered = !!is_selfpowered;

	return 0;
}

/**
 * dwc2_gadget_pullup - connect/disconnect the USB PHY
 * @gadget: The usb gadget state
 * @is_on: Current state of the USB PHY
 *
 * Connect/Disconnect the USB PHY pullup
 */
static int dwc2_gadget_pullup(struct usb_gadget *gadget, int is_on)
{
	struct dwc2 *dwc2 = to_dwc2(gadget);
	unsigned long flags = 0;

	dwc2_dbg(dwc2, "%s: is_on: %d\n", __func__, is_on);

	dwc2->enabled = is_on;
	/* Don't modify pullup state while in host mode */
	if (dwc2_is_host_mode(dwc2)) {
		WARN_ON(dwc2_is_host_mode(dwc2));
		return -EINVAL;
	}

	spin_lock_irqsave(&dwc2->lock, flags);
	if (is_on) {
		dwc2_core_gadget_init(dwc2, false);
		/* Enable ACG feature in device mode,if supported */
		dwc2_core_connect(dwc2);
	} else {
		dwc2_core_disconnect(dwc2);
		dwc2_gadget_disconnect(dwc2);
	}

	dwc2->gadget.speed = USB_SPEED_UNKNOWN;
	spin_unlock_irqrestore(&dwc2->lock, flags);

	return 0;
}

static int dwc2_gadget_vbus_session(struct usb_gadget *gadget, int is_active)
{
	struct dwc2 *dwc2 = to_dwc2(gadget);
	unsigned long flags;

	dwc2_dbg(dwc2, "%s: is_active: %d\n", __func__, is_active);
	spin_lock_irqsave(&dwc2->lock, flags);

	if (is_active) {
		dwc2_core_gadget_init(dwc2, false);
		if (dwc2->enabled)
			dwc2_core_connect(dwc2);
	} else {
		dwc2_core_disconnect(dwc2);
		dwc2_gadget_disconnect(dwc2);
	}

	spin_unlock_irqrestore(&dwc2->lock, flags);
	return 0;
}


/**
 * dwc2_handle_ep_disabled - handle DXEPINT_EPDISBLD
 * @hs_ep: The endpoint on which interrupt is asserted.
 *
 * This interrupt indicates that the endpoint has been disabled per the
 * application's request.
 *
 * For IN endpoints flushes txfifo, in case of BULK clears DCTL_CGNPINNAK,
 * in case of ISOC completes current request.
 *
 * For ISOC-OUT endpoints completes expired requests. If there is remaining
 * request starts it.
 */
static void dwc2_handle_ep_disabled(struct dwc2_ep *hs_ep)
{
	struct dwc2 *dwc2 = hs_ep->dwc2;
	struct dwc2_request *hs_req;
	unsigned char idx = hs_ep->epnum;
	int dir_in = hs_ep->dir_in;
	u32 epctl_reg = dir_in ? DIEPCTL(idx) : DOEPCTL(idx);
	int dctl = dwc2_readl(dwc2, DCTL);

	dwc2_dbg(dwc2, "%s: EPDisbld\n", __func__);

	if (dir_in) {
		int epctl = dwc2_readl(dwc2, epctl_reg);

		dwc2_flush_tx_fifo(dwc2, hs_ep->fifo_index);

		if (hs_ep->isochronous) {
			dwc2_complete_in(dwc2, hs_ep);
			return;
		}

		if ((epctl & DXEPCTL_STALL) && (epctl & DXEPCTL_EPTYPE_BULK)) {
			int dctl = dwc2_readl(dwc2, DCTL);

			dctl |= DCTL_CGNPINNAK;
			dwc2_writel(dwc2, dctl, DCTL);
		}
		return;
	}

	if (dctl & DCTL_GOUTNAKSTS) {
		dctl |= DCTL_CGOUTNAK;
		dwc2_writel(dwc2, dctl, DCTL);
	}

	if (!hs_ep->isochronous)
		return;

	if (list_empty(&hs_ep->queue)) {
		dwc2_dbg(dwc2, "%s: complete_ep 0x%p, ep->queue empty!\n",
			__func__, hs_ep);
		return;
	}

	do {
		hs_req = get_ep_head(hs_ep);
		if (hs_req)
			dwc2_complete_request(dwc2, hs_ep, hs_req, -ENODATA);
		dwc2_gadget_incr_frame_num(hs_ep);
		/* Update current frame number value. */
		dwc2->frame_number = dwc2_read_frameno(dwc2);
	} while (dwc2_gadget_target_frame_elapsed(hs_ep));

	dwc2_start_next_request(hs_ep);
}

/**
 * dwc2_handle_out_token_ep_disabled - handle DXEPINT_OUTTKNEPDIS
 * @ep: The endpoint on which interrupt is asserted.
 *
 * This is starting point for ISOC-OUT transfer, synchronization done with
 * first out token received from host while corresponding EP is disabled.
 *
 * Device does not know initial frame in which out token will come. For this
 * HW generates OUTTKNEPDIS - out token is received while EP is disabled. Upon
 * getting this interrupt SW starts calculation for next transfer frame.
 */
static void dwc2_handle_out_token_ep_disabled(struct dwc2_ep *ep)
{
	struct dwc2 *dwc2 = ep->dwc2;
	int dir_in = ep->dir_in;
	u32 doepmsk;
	u32 ctrl;

	dwc2_dbg(dwc2, "%s\n", __func__);

	if (dir_in || !ep->isochronous)
		return;

	if (ep->interval > 1 && ep->target_frame == TARGET_FRAME_INITIAL) {
		ep->target_frame = dwc2->frame_number;

		dwc2_gadget_incr_frame_num(ep);

		ctrl = dwc2_readl(dwc2, DOEPCTL(ep->epnum));

		if (ep->target_frame & 0x1)
			ctrl |= DXEPCTL_SETODDFR;
		else
			ctrl |= DXEPCTL_SETEVENFR;

		dwc2_writel(dwc2, ctrl, DOEPCTL(ep->epnum));
	}

	dwc2_start_next_request(ep);

	doepmsk = dwc2_readl(dwc2, DOEPMSK);
	doepmsk &= ~DOEPMSK_OUTTKNEPDISMSK;
	dwc2_writel(dwc2, doepmsk, DOEPMSK);
}

/**
 * dwc2_handle_nak - handle NAK interrupt
 * @hs_ep: The endpoint on which interrupt is asserted.
 *
 * This is starting point for ISOC-IN transfer, synchronization done with
 * first IN token received from host while corresponding EP is disabled.
 *
 * Device does not know when first one token will arrive from host. On first
 * token arrival HW generates 2 interrupts: 'in token received while FIFO empty'
 * and 'NAK'. NAK interrupt for ISOC-IN means that token has arrived and ZLP was
 * sent in response to that as there was no data in FIFO. SW is basing on this
 * interrupt to obtain frame in which token has come and then based on the
 * interval calculates next frame for transfer.
 */
static void dwc2_handle_nak(struct dwc2_ep *hs_ep)
{
	struct dwc2 *dwc2 = hs_ep->dwc2;
	int dir_in = hs_ep->dir_in;
	u32 ctrl;

	if (!dir_in || !hs_ep->isochronous)
		return;

	if (hs_ep->target_frame == TARGET_FRAME_INITIAL) {
		hs_ep->target_frame = dwc2->frame_number;
		if (hs_ep->interval > 1) {
			ctrl = dwc2_readl(dwc2, DIEPCTL(hs_ep->epnum));

			if (hs_ep->target_frame & 0x1)
				ctrl |= DXEPCTL_SETODDFR;
			else
				ctrl |= DXEPCTL_SETEVENFR;

			dwc2_writel(dwc2, ctrl, DIEPCTL(hs_ep->epnum));
		}

		dwc2_complete_request(dwc2, hs_ep, get_ep_head(hs_ep), 0);
	}

	dwc2_gadget_incr_frame_num(hs_ep);
}

/**
 * dwc2_handle_epint - handle an in/out endpoint interrupt
 * @dwc2: The driver state
 * @idx: The index for the endpoint (0..15)
 * @dir_in: Set if this is an IN endpoint
 *
 * Process and clear any interrupt pending for an individual endpoint
 */
static void dwc2_handle_epint(struct dwc2 *dwc2, unsigned int idx,
			      int dir_in)
{
	struct dwc2_ep *hs_ep = index_to_ep(dwc2, idx, dir_in);
	u32 epmsk_reg = dir_in ? DIEPMSK : DOEPMSK;
	u32 epint_reg = dir_in ? DIEPINT(idx) : DOEPINT(idx);
	u32 epctl_reg = dir_in ? DIEPCTL(idx) : DOEPCTL(idx);
	u32 epsiz_reg = dir_in ? DIEPTSIZ(idx) : DOEPTSIZ(idx);
	u32 ints;
	u32 mask;
	u32 ctrl;

	mask = dwc2_readl(dwc2, epmsk_reg);
	if ((dwc2_readl(dwc2, DIEPEMPMSK) >> idx) & 1)
		mask |= DIEPMSK_TXFIFOEMPTY;
	mask |= DXEPINT_SETUP_RCVD;

	ints = dwc2_readl(dwc2, epint_reg);

	if (ints & DXEPINT_AHBERR)
		dwc2_err(dwc2, "%s: AHBErr\n", __func__);

	ints &= mask;

	ctrl = dwc2_readl(dwc2, epctl_reg);

	/* Clear endpoint interrupts */
	dwc2_writel(dwc2, ints, epint_reg);

	if (!hs_ep) {
		dwc2_err(dwc2, "%s:Interrupt for unconfigured ep%d(%s)\n",
			__func__, idx, dir_in ? "in" : "out");
		return;
	}

	dwc2_dbg(dwc2, "%s: ep%d(%s) DxEPINT=0x%08x\n",
		__func__, idx, dir_in ? "in" : "out", ints);

	/* Don't process XferCompl interrupt if it is a setup packet */
	if (idx == 0 && (ints & (DXEPINT_SETUP | DXEPINT_SETUP_RCVD)))
		ints &= ~DXEPINT_XFERCOMPL;

	if (ints & DXEPINT_XFERCOMPL) {
		dwc2_dbg(dwc2,
			"%s: XferCompl: DxEPCTL=0x%08x, DXEPTSIZ=%08x\n",
			__func__, dwc2_readl(dwc2, epctl_reg),
			dwc2_readl(dwc2, epsiz_reg));
		if (dir_in) {
			/*
			 * We get OutDone from the FIFO, so we only
			 * need to look at completing IN requests here
			 * if operating slave mode
			 */
			if (hs_ep->isochronous && hs_ep->interval > 1)
				dwc2_gadget_incr_frame_num(hs_ep);

			dwc2_complete_in(dwc2, hs_ep);
			if (ints & DXEPINT_NAKINTRPT)
				ints &= ~DXEPINT_NAKINTRPT;

			if (idx == 0 && !hs_ep->req)
				dwc2_enqueue_setup(dwc2);
		} else if (using_dma(dwc2)) {
			/*
			 * We're using DMA, we need to fire an OutDone here
			 * as we ignore the RXFIFO.
			 */
			if (hs_ep->isochronous && hs_ep->interval > 1)
				dwc2_gadget_incr_frame_num(hs_ep);

			dwc2_handle_outdone(dwc2, idx);
		}
	}

	if (ints & DXEPINT_EPDISBLD)
		dwc2_handle_ep_disabled(hs_ep);

	if (ints & DXEPINT_OUTTKNEPDIS)
		dwc2_handle_out_token_ep_disabled(hs_ep);

	if (ints & DXEPINT_NAKINTRPT)
		dwc2_handle_nak(hs_ep);

	if (ints & DXEPINT_SETUP) {  /* Setup or Timeout */
		dwc2_dbg(dwc2, "%s: Setup/Timeout\n",  __func__);

		if (using_dma(dwc2) && idx == 0) {
			/*
			 * this is the notification we've received a
			 * setup packet. In non-DMA mode we'd get this
			 * from the RXFIFO, instead we need to process
			 * the setup here.
			 */

			if (!dir_in && dwc2->ep0_state == DWC2_EP0_SETUP)
				dwc2_handle_outdone(dwc2, 0);
		}
	}

	if (ints & DXEPINT_STSPHSERCVD)
		dwc2_dbg(dwc2, "%s: StsPhseRcvd\n", __func__);

	if (ints & DXEPINT_BACK2BACKSETUP)
		dwc2_dbg(dwc2, "%s: B2BSetup/INEPNakEff\n", __func__);

	if (ints & DXEPINT_BNAINTR) {
		dwc2_dbg(dwc2, "%s: BNA interrupt\n", __func__);
#if 0
		if (hs_ep->isochronous)
			dwc2_handle_isoc_bna(hs_ep);
#endif
	}

	if (dir_in && !hs_ep->isochronous) {
		/* not sure if this is important, but we'll clear it anyway */
		if (ints & DXEPINT_INTKNTXFEMP) {
			dwc2_dbg(dwc2, "%s: ep%d: INTknTXFEmpMsk\n",
				__func__, idx);
		}

		/* this probably means something bad is happening */
		if (ints & DXEPINT_INTKNEPMIS) {
			dwc2_warn(dwc2, "%s: ep%d: INTknEP\n",
				 __func__, idx);
		}

		/* FIFO has space or is empty (see GAHBCFG) */
		if (dwc2->dedicated_fifos && ints & DXEPINT_TXFEMP) {
			dwc2_dbg(dwc2, "%s: ep%d: TxFIFOEmpty\n",
				__func__, idx);
		}
	}
}

/**
 * dwc2_handle_enumdone - Handle EnumDone interrupt (enumeration done)
 * @dwc2: The device state.
 *
 * Handle updating the device settings after the enumeration phase has
 * been completed.
 */
static void dwc2_handle_enumdone(struct dwc2 *dwc2)
{
	u32 dsts = dwc2_readl(dwc2, DSTS);
	int ep0_mps = 0, ep_mps = 8;
	int i;

	/*
	 * This should signal the finish of the enumeration phase
	 * of the USB handshaking, so we should now know what rate
	 * we connected at.
	 */
	dwc2_dbg(dwc2, "EnumDone (DSTS=0x%08x)\n", dsts);

	/*
	 * note, since we're limited by the size of transfer on EP0, and
	 * it seems IN transfers must be a even number of packets we do
	 * not advertise a 64byte MPS on EP0.
	 */

	/* catch both EnumSpd_FS and EnumSpd_FS48 */
	switch ((dsts & DSTS_ENUMSPD_MASK) >> DSTS_ENUMSPD_SHIFT) {
	case DSTS_ENUMSPD_FS:
	case DSTS_ENUMSPD_FS48:
		dwc2->gadget.speed = USB_SPEED_FULL;
		ep0_mps = 64;
		ep_mps = 1023;
		break;
	case DSTS_ENUMSPD_HS:
		dwc2->gadget.speed = USB_SPEED_HIGH;
		ep0_mps = 64;
		ep_mps = 1024;
		break;
	case DSTS_ENUMSPD_LS:
		dwc2->gadget.speed = USB_SPEED_LOW;
		ep0_mps = 8;
		ep_mps = 8;
		/*
		 * note, we don't actually support LS in this driver at the
		 * moment, and the documentation seems to imply that it isn't
		 * supported by the PHYs on some of the devices.
		 */
		break;
	}
	dwc2_dbg(dwc2, "new %s device\n", usb_speed_string(dwc2->gadget.speed));

	/*
	 * we should now know the maximum packet size for an
	 * endpoint, so set the endpoints to a default value.
	 */
	if (ep0_mps) {
		/* Initialize ep0 for both in and out directions */
		dwc2_set_ep_maxpacket(dwc2, 0, ep0_mps, 0, 1);
		dwc2_set_ep_maxpacket(dwc2, 0, ep0_mps, 0, 0);
		for (i = 1; i < dwc2->num_eps; i++) {
			if (dwc2->eps_in[i])
				dwc2_set_ep_maxpacket(dwc2, i, ep_mps, 0, 1);
			if (dwc2->eps_out[i])
				dwc2_set_ep_maxpacket(dwc2, i, ep_mps, 0, 0);
		}
	}

	/* ensure after enumeration our EP0 is active */
	dwc2_enqueue_setup(dwc2);

	dwc2_dbg(dwc2, "EP0: DIEPCTL0=0x%08x, DOEPCTL0=0x%08x\n",
		dwc2_readl(dwc2, DIEPCTL0),
		dwc2_readl(dwc2, DOEPCTL0));
}

/* IRQ flags which will trigger a retry around the IRQ loop */
#define IRQ_RETRY_MASK (GINTSTS_NPTXFEMP | \
			GINTSTS_PTXFEMP |  \
			GINTSTS_RXFLVL)

static void dwc2_gadget_udc_poll(struct usb_gadget *gadget)
{
	struct dwc2 *dwc2 = to_dwc2(gadget);
	int retry_count = 4;
	u32 gintsts;
	u32 gintmsk;

	if (!dwc2_is_device_mode(dwc2))
		return;

	spin_lock(&dwc2->lock);

irq_retry:
	gintsts = readl(dwc2->regs + GINTSTS);
	gintmsk = readl(dwc2->regs + GINTMSK);
	gintsts &= gintmsk;

	if (!gintsts)
		return;

	dwc2_dbg(dwc2, "%s: %08x (%08x) retry %d\n",
		__func__, gintsts, gintmsk, 4 - retry_count);

	if (gintsts & GINTSTS_RESETDET) {
		dwc2_dbg(dwc2, "%s: USBRstDet\n", __func__);
		dwc2_writel(dwc2, GINTSTS_RESETDET, GINTSTS);
	}

	if (gintsts & (GINTSTS_USBRST | GINTSTS_RESETDET)) {
		u32 usb_status = dwc2_readl(dwc2, GOTGCTL);
		u32 connected = dwc2->connected;

		dwc2_dbg(dwc2, "%s: USBRst\n", __func__);
		dwc2_dbg(dwc2, "GNPTXSTS=%08x\n", dwc2_readl(dwc2, GNPTXSTS));

		dwc2_writel(dwc2, GINTSTS_USBRST, GINTSTS);

		/* Report disconnection if it is not already done. */
		dwc2_gadget_disconnect(dwc2);

		/* Reset device address to zero */
		dwc2_dcfg_set_addr(dwc2, 0);

		if (usb_status & GOTGCTL_BSESVLD && connected)
			dwc2_core_gadget_init(dwc2, true);
	}

	if (gintsts & GINTSTS_ENUMDONE) {
		dwc2_writel(dwc2, GINTSTS_ENUMDONE, GINTSTS);

		dwc2_handle_enumdone(dwc2);
	}

	if (gintsts & (GINTSTS_OEPINT | GINTSTS_IEPINT)) {
		u32 daint = dwc2_readl(dwc2, DAINT);
		u32 daintmsk = dwc2_readl(dwc2, DAINTMSK);
		u32 daint_out, daint_in;
		int ep;

		daint &= daintmsk;
		daint_out = daint >> DAINT_OUTEP_SHIFT;
		daint_in = daint & ~(daint_out << DAINT_OUTEP_SHIFT);

		dwc2_dbg(dwc2, "%s: daint=%08x\n", __func__, daint);

		for (ep = 0; ep < dwc2->num_eps && daint_out;
						ep++, daint_out >>= 1) {
			if (daint_out & 1)
				dwc2_handle_epint(dwc2, ep, 0);
		}

		for (ep = 0; ep < dwc2->num_eps && daint_in;
						ep++, daint_in >>= 1) {
			if (daint_in & 1)
				dwc2_handle_epint(dwc2, ep, 1);
		}
	}

	/* check both FIFOs */
	if (gintsts & GINTSTS_NPTXFEMP) {
		dwc2_dbg(dwc2, "NPTxFEmp\n");

		/*
		 * Disable the interrupt to stop it happening again
		 * unless one of these endpoint routines decides that
		 * it needs re-enabling
		 */
#if 0
		dwc2_hsotg_disable_gsint(dwc2, GINTSTS_NPTXFEMP);
		dwc2_hsotg_irq_fifoempty(dwc2, false);
#endif
	}

	if (gintsts & GINTSTS_PTXFEMP) {
		dwc2_dbg(dwc2, "PTxFEmp\n");

		/* See note in GINTSTS_NPTxFEmp */
#if 0
		dwc2_hsotg_disable_gsint(dwc2, GINTSTS_PTXFEMP);
		dwc2_hsotg_irq_fifoempty(dwc2, true);
#endif
	}

	if (gintsts & GINTSTS_RXFLVL) {
		/*
		 * note, since GINTSTS_RxFLvl doubles as FIFO-not-empty,
		 * we need to retry dwc2_hsotg_handle_rx if this is still
		 * set.
		 */
		dwc2_err(dwc2, "RXFLVL\n");
#if 0
		dwc2_handle_rx(dwc2);
#endif
	}

	if (gintsts & GINTSTS_ERLYSUSP) {
		dwc2_dbg(dwc2, "GINTSTS_ErlySusp\n");
		dwc2_writel(dwc2, GINTSTS_ERLYSUSP, GINTSTS);
	}
	if (gintsts & GINTSTS_USBSUSP) {
		dwc2_dbg(dwc2, "USBSusp\n");
		dwc2_writel(dwc2, GINTSTS_USBSUSP, GINTSTS);
	}

	/*
	 * these next two seem to crop-up occasionally causing the core
	 * to shutdown the USB transfer, so try clearing them and logging
	 * the occurrence.
	 */

	if (gintsts & GINTSTS_GOUTNAKEFF) {
		u8 idx;
		u32 epctrl;
		u32 gintmsk;
		u32 daintmsk;
		struct dwc2_ep *hs_ep;

		daintmsk = dwc2_readl(dwc2, DAINTMSK);
		daintmsk >>= DAINT_OUTEP_SHIFT;
		/* Mask this interrupt */
		gintmsk = dwc2_readl(dwc2, GINTMSK);
		gintmsk &= ~GINTSTS_GOUTNAKEFF;
		dwc2_writel(dwc2, gintmsk, GINTMSK);

		dwc2_dbg(dwc2, "GOUTNakEff triggered\n");
		for (idx = 1; idx < dwc2->num_eps; idx++) {
			hs_ep = dwc2->eps_out[idx];
			/* Proceed only unmasked ISOC EPs */
			if ((BIT(idx) & ~daintmsk) || !hs_ep->isochronous)
				continue;

			epctrl = dwc2_readl(dwc2, DOEPCTL(idx));

			if (epctrl & DXEPCTL_EPENA) {
				epctrl |= DXEPCTL_SNAK;
				epctrl |= DXEPCTL_EPDIS;
				dwc2_writel(dwc2, epctrl, DOEPCTL(idx));
			}
		}

		/* This interrupt bit is cleared in DXEPINT_EPDISBLD handler */
	}

	if (gintsts & GINTSTS_GINNAKEFF) {
		dwc2_info(dwc2, "GINNakEff triggered\n");
		dwc2_set_bit(dwc2, DCTL, DCTL_CGNPINNAK);
	}
#if 0
	if (gintsts & GINTSTS_INCOMPL_SOIN)
		dwc2_handle_incomplete_isoc_in(dwc2);

	if (gintsts & GINTSTS_INCOMPL_SOOUT)
		dwc2_handle_incomplete_isoc_out(dwc2);
#endif

	/*
	 * if we've had fifo events, we should try and go around the
	 * loop again to see if there's any point in returning yet.
	 */
	if (gintsts & IRQ_RETRY_MASK && --retry_count > 0)
		goto irq_retry;

	spin_unlock(&dwc2->lock);
}

/**
 * dwc2_dwc2_udc_start - prepare the udc for work
 * @gadget: The usb gadget state
 * @driver: The usb gadget driver
 *
 * Perform initialization to prepare udc device and driver
 * to work.
 */
static int dwc2_gadget_udc_start(struct usb_gadget *gadget,
				 struct usb_gadget_driver *driver)
{
	struct dwc2 *dwc2 = to_dwc2(gadget);

	if (!driver) {
		dwc2_err(dwc2, "%s: no driver\n", __func__);
		return -EINVAL;
	}

	if (driver->max_speed < USB_SPEED_FULL) {
		dwc2_err(dwc2, "%s: bad speed\n", __func__);
		return -EINVAL;
	}

	dwc2->driver = driver;
	dwc2->gadget.speed = USB_SPEED_UNKNOWN;

	dwc2_core_gadget_init(dwc2, false);
	dwc2_gadget_interrupt_init(dwc2);

	dwc2_info(dwc2, "bound driver %s\n", driver->driver.name);

	return 0;
}

static int dwc2_gadget_udc_stop(struct usb_gadget *gadget)
{
	struct dwc2 *dwc2 = to_dwc2(gadget);
	unsigned long flags = 0;

	dwc2_dbg(dwc2, "%s\n", __func__);

	/* all endpoints should be shutdown */
	spin_lock_irqsave(&dwc2->lock, flags);

	dwc2->driver = NULL;
	dwc2->gadget.speed = USB_SPEED_UNKNOWN;
	dwc2->enabled = 0;

	spin_unlock_irqrestore(&dwc2->lock, flags);

	return 0;
}

static const struct usb_gadget_ops dwc2_gadget_ops = {
	.get_frame		= dwc2_gadget_get_frame,
	.set_selfpowered	= dwc2_set_selfpowered,
	.vbus_session		= dwc2_gadget_vbus_session,
	.vbus_draw		= NULL,
	.pullup			= dwc2_gadget_pullup,
	.udc_start		= dwc2_gadget_udc_start,
	.udc_stop		= dwc2_gadget_udc_stop,
	.udc_poll		= dwc2_gadget_udc_poll,
};

/**
 * dwc2_gadget_ep_init - initialise a single endpoint
 * @dwc2: The device state.
 * @epnum: The endpoint number
 * @dir_in: True if direction is in.
 *
 * Initialise the given endpoint (as part of the probe and device state
 * creation) to give to the gadget driver. Setup the endpoint name, any
 * direction information and other state that may be required.
 */
static void dwc2_ep_init(struct dwc2 *dwc2, int epnum,  bool dir_in)
{
	struct dwc2_ep *ep;
	char *dir;

	if (dir_in)
		ep = dwc2->eps_in[epnum];
	else
		ep = dwc2->eps_out[epnum];

	if (epnum == 0)
		dir = "";
	else if (dir_in)
		dir = "in";
	else
		dir = "out";

	ep->dir_in = dir_in;
	ep->epnum = epnum;

	snprintf(ep->name, sizeof(ep->name), "ep%d%s", epnum, dir);

	INIT_LIST_HEAD(&ep->queue);
	INIT_LIST_HEAD(&ep->ep.ep_list);

	if (epnum)
		list_add_tail(&ep->ep.ep_list, &dwc2->gadget.ep_list);

	ep->dwc2 = dwc2;
	ep->ep.name = ep->name;

	if (dwc2->params.speed == DWC2_SPEED_PARAM_LOW)
		usb_ep_set_maxpacket_limit(&ep->ep, 8);
	else if (epnum == 0)
		usb_ep_set_maxpacket_limit(&ep->ep, D0EPCTL_MPS_LIMIT);
	else
		usb_ep_set_maxpacket_limit(&ep->ep, 1024);

	ep->ep.ops = &dwc2_ep_ops;
}

static int dwc2_eps_alloc(struct dwc2 *dwc2)
{
	struct dwc2_ep *ep;
	u32 cfg;
	u32 ep_type;
	u32 i;

	/* Number of Device Endpoints */
	dwc2->num_eps = 1 + dwc2->hw_params.num_dev_ep;

	ep = kzalloc(sizeof(*ep), GFP_KERNEL);
	if (!ep)
		return -ENOMEM;

	/* Same endpoint is used in both directions for ep0 */
	dwc2->eps_out[0] = dwc2->eps_in[0] = ep;

	cfg = dwc2->hw_params.dev_ep_dirs;
	for (i = 1, cfg >>= 2; i < dwc2->num_eps; i++, cfg >>= 2) {
		ep_type = cfg & 3;
		/* Direction in or both */
		if (!(ep_type & 2)) {
			ep = kzalloc(sizeof(*ep), GFP_KERNEL);
			if (!ep)
				return -ENOMEM;
			dwc2->eps_in[i] = ep;
		}
		/* Direction out or both */
		if (!(ep_type & 1)) {
			ep = kzalloc(sizeof(*ep), GFP_KERNEL);
			if (!ep)
				return -ENOMEM;
			dwc2->eps_out[i] = ep;
		}
	}

	dwc2->dedicated_fifos = dwc2->hw_params.en_multiple_tx_fifo;

	dwc2_info(dwc2, "EPs: %d, %s fifos, 0x%x entries in SPRAM\n",
		 dwc2->num_eps,
		 dwc2->dedicated_fifos ? "dedicated" : "shared",
		 dwc2->hw_params.total_fifo_size);
	return 0;
}

/**
 * dwc2_force_mode() - Force the mode of the controller.
 *
 * Forcing the mode is needed for two cases:
 *
 * 1) If the dr_mode is set to either HOST or PERIPHERAL we force the
 * controller to stay in a particular mode regardless of ID pin
 * changes. We do this once during probe.
 *
 * 2) During probe we want to read reset values of the hw
 * configuration registers that are only available in either host or
 * device mode. We may need to force the mode if the current mode does
 * not allow us to access the register in the mode that we want.
 *
 * In either case it only makes sense to force the mode if the
 * controller hardware is OTG capable.
 *
 * Checks are done in this function to determine whether doing a force
 * would be valid or not.
 *
 * If a force is done, it requires a IDDIG debounce filter delay if
 * the filter is configured and enabled. We poll the current mode of
 * the controller to account for this delay.
 *
 * @dwc2: Programming view of DWC_otg controller
 * @host: Host mode flag
 */
static void dwc2_force_mode(struct dwc2 *dwc2, bool host)
{
	u32 gusbcfg;
	u32 set;
	u32 clear;

	dev_dbg(dwc2->dev, "Forcing mode to %s\n", host ? "host" : "device");

	/*
	 * If dr_mode is either peripheral or host only, there is no
	 * need to ever force the mode to the opposite mode.
	 */
	if (WARN_ON(host && dwc2->dr_mode == USB_DR_MODE_PERIPHERAL))
		return;

	if (WARN_ON(!host && dwc2->dr_mode == USB_DR_MODE_HOST))
		return;

	gusbcfg = dwc2_readl(dwc2, GUSBCFG);

	set = host ? GUSBCFG_FORCEHOSTMODE : GUSBCFG_FORCEDEVMODE;
	clear = host ? GUSBCFG_FORCEDEVMODE : GUSBCFG_FORCEHOSTMODE;

	gusbcfg &= ~clear;
	gusbcfg |= set;
	dwc2_writel(dwc2, gusbcfg, GUSBCFG);

	dwc2_wait_for_mode(dwc2, host);

	return;
}

/**
 * dwc2_clear_force_mode() - Clears the force mode bits.
 *
 * After clearing the bits, wait up to 100 ms to account for any
 * potential IDDIG filter delay. We can't know if we expect this delay
 * or not because the value of the connector ID status is affected by
 * the force mode. We only need to call this once during probe if
 * dr_mode == OTG.
 *
 * @dwc2: Programming view of DWC_otg controller
 */
static void dwc2_clear_force_mode(struct dwc2 *dwc2)
{
	u32 gusbcfg;

	dev_dbg(dwc2->dev, "Clearing force mode bits\n");

	gusbcfg = dwc2_readl(dwc2, GUSBCFG);
	gusbcfg &= ~GUSBCFG_FORCEHOSTMODE;
	gusbcfg &= ~GUSBCFG_FORCEDEVMODE;
	dwc2_writel(dwc2, gusbcfg, GUSBCFG);

	if (dwc2_iddig_filter_enabled(dwc2))
		mdelay(100);
}

int dwc2_gadget_init(struct dwc2 *dwc2)
{
	u32 dctl;
	int epnum;
	int ret;

	if (!dwc2->params.dma) {
		dwc2_err(dwc2, "no DMA support, required by gadget driver");
		return -ENOTSUPP;
	}

	dwc2_core_init(dwc2);

	dwc2->gadget.speed = USB_SPEED_UNKNOWN;
	if (dwc2->params.speed == DWC2_SPEED_PARAM_HIGH)
		dwc2->gadget.max_speed = USB_SPEED_HIGH;
	else
		dwc2->gadget.max_speed = USB_SPEED_FULL;

	dwc2->gadget.ops = &dwc2_gadget_ops;
	dwc2->gadget.name = "DWC2 gadget";

	dwc2->gadget.is_otg = (dwc2->dr_mode == USB_DR_MODE_OTG) ? 1 : 0;

	if (dwc2->gadget.is_otg)
		dwc2_clear_force_mode(dwc2);
	else
		dwc2_force_mode(dwc2, false);

	ret = dwc2_eps_alloc(dwc2);
	if (ret) {
		dwc2_err(dwc2, "Endpoints allocation failed: %d\n", ret);
		return ret;
	}

	dwc2->ctrl_buff = dma_alloc(DWC2_CTRL_BUFF_SIZE);
	if (!dwc2->ctrl_buff)
		return -ENOMEM;

	dwc2->ep0_buff = dma_alloc(DWC2_CTRL_BUFF_SIZE);
	if (!dwc2->ep0_buff)
		return -ENOMEM;

	if (dwc2->num_eps == 0) {
		dwc2_err(dwc2, "wrong number of EPs (zero)\n");
		return -EINVAL;
	}

	/* setup endpoint information */
	INIT_LIST_HEAD(&dwc2->gadget.ep_list);
	dwc2->gadget.ep0 = &dwc2->eps_out[0]->ep;

	/* allocate EP0 request */
	dwc2->ctrl_req = dwc2_ep_alloc_req(&dwc2->eps_out[0]->ep);
	if (!dwc2->ctrl_req) {
		dwc2_err(dwc2, "failed to allocate ctrl req\n");
		return -ENOMEM;
	}

	/* initialise the endpoints now the core has been initialised */
	for (epnum = 0; epnum < dwc2->num_eps; epnum++) {
		if (dwc2->eps_in[epnum])
			dwc2_ep_init(dwc2, epnum, 1);
		if (dwc2->eps_out[epnum])
			dwc2_ep_init(dwc2, epnum, 0);
	}

	/* Be in disconnected state until gadget is registered */
	dctl = dwc2_readl(dwc2, DCTL);
	dwc2_writel(dwc2, dctl | DCTL_SFTDISCON, DCTL);

	ret = usb_add_gadget_udc(dwc2->dev, &dwc2->gadget);
	if (ret) {
		dwc2_ep_free_req(&dwc2->eps_out[0]->ep, dwc2->ctrl_req);
		return ret;
	}

	return 0;
}

void dwc2_gadget_uninit(struct dwc2 *dwc2)
{
	dwc2_core_disconnect(dwc2);
	dwc2_gadget_disconnect(dwc2);
}
