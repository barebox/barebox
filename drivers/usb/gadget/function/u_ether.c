// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/usb/gadget/function/u_ether.c?id=a36e5e800b9c93e3e1ffa42f34d38b36775dbcee
/*
 * u_ether.c -- Ethernet-over-USB link layer utilities for Gadget stack
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2003-2004 Robert Schwebel, Benedikt Spranger
 * Copyright (C) 2008 Nokia Corporation
 */

/* #define VERBOSE_DEBUG */

#include <common.h>
#include <linux/kernel.h>
#include <driver.h>
#include <linux/ctype.h>
#include <linux/atomic.h>
#include <linux/skbuff.h>
#include <net.h>
#include <poller.h>

#include "u_ether.h"

/*
 * This component encapsulates the Ethernet link glue needed to provide
 * one (!) network link through the USB gadget stack, normally "usb0".
 *
 * The control and data models are handled by the function driver which
 * connects to this code; such as CDC Ethernet (ECM or EEM),
 * "CDC Subset", or RNDIS.  That includes all descriptor and endpoint
 * management.
 *
 * Link level addressing is handled by this component using module
 * parameters; if no such parameters are provided, random link level
 * addresses are used.  Each end of the link uses one address.  The
 * host end address is exported in various ways, and is often recorded
 * in configuration databases.
 *
 * The driver which assembles each configuration using such a link is
 * responsible for ensuring that each configuration includes at most one
 * instance of is network link.  (The network layer provides ways for
 * this single "physical" link to be used by multiple virtual links.)
 */

#define netif_running(net)	((net)->active)

struct eth_dev {
	struct gether		*port_usb;

	struct eth_device	net;
	struct usb_gadget	*gadget;

	struct list_head	tx_reqs, rx_reqs;

	struct sk_buff_head	rx_frames;

	unsigned		qmult;

	unsigned		header_len;
	unsigned		tailroom;
	struct sk_buff		*(*wrap)(struct gether *, struct sk_buff *skb);
	int			(*unwrap)(struct gether *,
						struct sk_buff *skb,
						struct sk_buff_head *list);

	bool			zlp;
	u8			host_mac[ETH_ALEN];
	u8			dev_mac[ETH_ALEN];
};

/*-------------------------------------------------------------------------*/

#define RX_EXTRA	20	/* bytes guarding against rx overflows */

#define DEFAULT_QLEN	2	/* double buffering by default */

/* for dual-speed hardware, use deeper queues at high/super speed */
static inline int qlen(struct usb_gadget *gadget, unsigned qmult)
{
	if (gadget_is_dualspeed(gadget) && (gadget->speed == USB_SPEED_HIGH ||
					    gadget->speed >= USB_SPEED_SUPER))
		return qmult * DEFAULT_QLEN;
	else
		return DEFAULT_QLEN;
}

/*-------------------------------------------------------------------------*/

/* REVISIT there must be a better way than having two sets
 * of debug calls ...
 */

#undef DBG
#undef VDBG
#undef ERROR
#undef INFO

#define xprintk(d, level, fmt, args...) \
	printk(level "%s: " fmt , (d)->net.devname , ## args)

#ifdef DEBUG
#undef DEBUG
#define DBG(dev, fmt, args...) \
	xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DBG(dev, fmt, args...) \
	do { } while (0)
#endif /* DEBUG */

#ifdef VERBOSE_DEBUG
#define VDBG	DBG
#else
#define VDBG(dev, fmt, args...) \
	do { } while (0)
#endif /* DEBUG */

#define ERROR(dev, fmt, args...) \
	xprintk(dev , KERN_ERR , fmt , ## args)
#define INFO(dev, fmt, args...) \
	xprintk(dev , KERN_INFO , fmt , ## args)

/*-------------------------------------------------------------------------*/

static void rx_complete(struct usb_ep *ep, struct usb_request *req);

static int
rx_submit(struct eth_dev *dev, struct usb_request *req)
{
	struct sk_buff	*skb;
	int		retval = -ENOMEM;
	size_t		size = 0;
	struct usb_ep	*out;

	if (dev->port_usb)
		out = dev->port_usb->out_ep;
	else
		out = NULL;

	if (!out)
		return -ENOTCONN;

	/* Padding up to RX_EXTRA handles minor disagreements with host.
	 * Normally we use the USB "terminate on short read" convention;
	 * so allow up to (N*maxpacket), since that memory is normally
	 * already allocated.  Some hardware doesn't deal well with short
	 * reads (e.g. DMA must be N*maxpacket), so for now don't trim a
	 * byte off the end (to force hardware errors on overflow).
	 *
	 * RNDIS uses internal framing, and explicitly allows senders to
	 * pad to end-of-packet.  That's potentially nice for speed, but
	 * means receivers can't recover lost synch on their own (because
	 * new packets don't only start after a short RX).
	 */
	size += ETH_HLEN + PKTSIZE + RX_EXTRA;
	size += dev->port_usb->header_len;

	/*
	 * Only a short packet ends an OUT transfer early, so the controller
	 * may fill the buffer up to the next multiple of maxpacket.
	 */
	size = usb_ep_align(out, size);

	if (dev->port_usb->is_fixed)
		size = max_t(size_t, size, dev->port_usb->fixed_out_len);

	skb = dev_alloc_skb(size + NET_IP_ALIGN);
	if (skb == NULL) {
		DBG(dev, "no rx skb\n");
		goto enomem;
	}

	skb_reserve(skb, NET_IP_ALIGN);

	req->buf = skb->data;
	req->length = size;
	req->complete = rx_complete;
	req->context = skb;

	retval = usb_ep_queue(out, req);
enomem:
	if (retval) {
		DBG(dev, "rx submit --> %d\n", retval);
		if (skb)
			dev_kfree_skb_any(skb);
		list_add(&req->list, &dev->rx_reqs);
	}
	return retval;
}

static void rx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct sk_buff	*skb = req->context;
	struct eth_dev	*dev = ep->driver_data;
	int		status = req->status;

	req->context = NULL;
	/* defer requeue until rx_frames are processed */
	list_add(&req->list, &dev->rx_reqs);

	switch (status) {

	/* normal completion */
	case 0:
		if (req->actual > skb_tailroom(skb)) {
			DBG(dev, "rx overflow %u\n", req->actual);
			break;
		}

		skb_put(skb, req->actual);

		if (dev->unwrap) {
			if (dev->port_usb)
				dev->unwrap(dev->port_usb, skb, &dev->rx_frames);
			else
				dev_kfree_skb_any(skb);
		} else {
			skb_queue_tail(&dev->rx_frames, skb);
		}
		return;

	/* software-driven interface shutdown */
	case -ECONNRESET:		/* unlink */
	case -ESHUTDOWN:		/* disconnect etc */
		VDBG(dev, "rx shutdown, code %d\n", status);
		break;

	/* for hardware automagic (such as pxa) */
	case -ECONNABORTED:		/* endpoint reset */
		DBG(dev, "rx %s reset\n", ep->name);
		break;

	/* data overrun */
	case -EOVERFLOW:
		fallthrough;

	default:
		DBG(dev, "rx status %d\n", status);
		break;
	}

	dev_kfree_skb_any(skb);
}

static int prealloc(struct list_head *list, struct usb_ep *ep, unsigned n)
{
	unsigned		i;
	struct usb_request	*req;

	if (!n)
		return -ENOMEM;

	/* queue/recycle up to N requests */
	i = n;
	list_for_each_entry(req, list, list) {
		if (i-- == 0)
			goto extra;
	}
	while (i--) {
		req = usb_ep_alloc_request(ep);
		if (!req)
			return list_empty(list) ? -ENOMEM : 0;
		req->context = NULL;
		list_add(&req->list, list);
	}
	return 0;

extra:
	/* free extras */
	for (;;) {
		struct list_head	*next;

		next = req->list.next;
		list_del(&req->list);
		usb_ep_free_request(ep, req);

		if (next == list)
			break;

		req = container_of(next, struct usb_request, list);
	}
	return 0;
}

static int alloc_requests(struct eth_dev *dev, struct gether *link, unsigned n)
{
	int	status;

	status = prealloc(&dev->tx_reqs, link->in_ep, n);
	if (status < 0)
		goto fail;
	status = prealloc(&dev->rx_reqs, link->out_ep, n);
	if (status < 0)
		goto fail;
	goto done;
fail:
	DBG(dev, "can't alloc requests\n");
done:
	return status;
}

static void rx_fill(struct eth_dev *dev)
{
	struct usb_request	*req;

	/* fill unused rxq slots with some skb */
	while (!list_empty(&dev->rx_reqs)) {
		req = list_first_entry(&dev->rx_reqs, struct usb_request, list);
		list_del_init(&req->list);

		if (rx_submit(dev, req) < 0)
			return;
	}
}

static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct eth_dev	*dev = ep->driver_data;

	dev_kfree_skb_any(req->context);
	req->context = NULL;
	list_add(&req->list, &dev->tx_reqs);
}

static inline int is_promisc(u16 cdc_filter)
{
	return cdc_filter & USB_CDC_PACKET_TYPE_PROMISCUOUS;
}

static int gether_start_xmit(struct eth_device *net, void *packet, int length)
{
	struct sk_buff		*skb;
	struct eth_dev		*dev = net->priv;
	int			retval;
	struct usb_request	*req;
	struct usb_ep		*in;
	u16			cdc_filter;

	if (dev->port_usb) {
		in = dev->port_usb->in_ep;
		cdc_filter = dev->port_usb->cdc_filter;
	} else {
		in = NULL;
		cdc_filter = 0;
	}

	if (!in)
		return 0;

	/* apply outgoing CDC or RNDIS filters */
	if (!is_promisc(cdc_filter)) {
		u8		*dest = packet;

		if (is_multicast_ether_addr(dest)) {
			u16	type;

			/* ignores USB_CDC_PACKET_TYPE_MULTICAST and host
			 * SET_ETHERNET_MULTICAST_FILTERS requests
			 */
			if (is_broadcast_ether_addr(dest))
				type = USB_CDC_PACKET_TYPE_BROADCAST;
			else
				type = USB_CDC_PACKET_TYPE_ALL_MULTICAST;
			if (!(cdc_filter & type))
				return 0;
		}
		/* ignores USB_CDC_PACKET_TYPE_DIRECTED */
	}

	if (list_empty(&dev->tx_reqs)) {
		u64 start = get_time_ns();

		/*
		 * Requests complete in the UDC poller. That doesn't run while
		 * we are called from a poller ourselves, e.g. to answer an ARP
		 * request, so drop the frame as a full TX ring would.
		 */
		if (poller_active())
			return -EBUSY;

		while (list_empty(&dev->tx_reqs)) {
			if (is_timeout(start, 100 * MSECOND))
				return -ETIMEDOUT;
		}
	}

	/*
	 * Leave room for the function's framing, so wrap() needn't copy
	 * the frame a second time, and for the zlp substitute byte below.
	 */
	skb = dev_alloc_skb(dev->header_len + length + dev->tailroom + 1);
	if (!skb)
		return -ENOMEM;

	skb_reserve(skb, dev->header_len);
	memcpy(skb_put(skb, length), packet, length);

	/* no buffer copies needed, unless the network stack did it
	 * or the hardware can't use skb buffers.
	 * or there's not enough space for extra headers we need
	 */
	if (dev->wrap) {
		if (dev->port_usb)
			skb = dev->wrap(dev->port_usb, skb);
		if (!skb) {
			/* Multi frame CDC protocols may store the frame for
			 * later, we don't maintain statistics, so we can just
			 * treat it as dropped frame
			 */
			return 0;
		}
	}

	length = skb->len;
	req = list_first_entry(&dev->tx_reqs, struct usb_request, list);
	list_del(&req->list);
	req->buf = skb->data;
	req->context = skb;
	req->complete = tx_complete;

	/* NCM requires no zlp if transfer is dwNtbInMaxSize */
	if (dev->port_usb &&
	    dev->port_usb->is_fixed &&
	    length == dev->port_usb->fixed_in_len &&
	    (length % in->maxpacket) == 0)
		req->zero = 0;
	else
		req->zero = 1;

	/* use zlp framing on tx for strict CDC-Ether conformance,
	 * though any robust network rx path ignores extra padding.
	 * and some hardware doesn't like to write zlps.
	 */
	if (req->zero && !dev->zlp && (length % in->maxpacket) == 0)
		length++;

	req->length = length;

	retval = usb_ep_queue(in, req);
	if (retval) {
		DBG(dev, "tx queue err %d\n", retval);
		dev_kfree_skb_any(skb);
		req->context = NULL;
		list_add(&req->list, &dev->tx_reqs);
	}

	return retval;
}

static void gether_recv(struct eth_device *net)
{
	struct eth_dev *dev = net->priv;
	struct sk_buff *skb;

	while ((skb = list_first_entry_or_null(&dev->rx_frames,
					       struct sk_buff, list))) {
		list_del(&skb->list);

		if (ETH_HLEN <= skb->len && skb->len <= PKTSIZE)
			net_receive(&dev->net, skb->data, skb->len);
		else
			DBG(dev, "rx length %d\n", skb->len);

		dev_kfree_skb_any(skb);
	}

	rx_fill(dev);
}

/*-------------------------------------------------------------------------*/

static void eth_start(struct eth_dev *dev)
{
	DBG(dev, "%s\n", __func__);

	/* fill the rx queue */
	rx_fill(dev);
}

static int gether_open(struct eth_device *net)
{
	struct eth_dev	*dev = net->priv;
	struct gether	*link;

	DBG(dev, "%s\n", __func__);
	eth_start(dev);

	link = dev->port_usb;
	if (link && link->open)
		link->open(link);

	return 0;
}

static void gether_stop(struct eth_device *net)
{
	struct eth_dev	*dev = net->priv;

	VDBG(dev, "%s\n", __func__);

	/*
	 * Don't disable the USB endpoints here.  Endpoints are managed
	 * by the USB gadget framework via gether_connect/gether_disconnect
	 * and must stay enabled to keep the host-side link up.  Disabling
	 * them here would cause the host to reset its interface and lose
	 * its IP address every time barebox closes the network device.
	 */
	if (dev->port_usb) {
		struct gether	*link = dev->port_usb;

		if (link->close)
			link->close(link);
	}
}

/*-------------------------------------------------------------------------*/

static int get_ether_addr_str(u8 dev_addr[ETH_ALEN], char *str, int len)
{
	if (len < 18)
		return -EINVAL;

	snprintf(str, len, "%pM", dev_addr);
	return 18;
}

static int gether_set_ethaddr(struct eth_device *net, const unsigned char *mac)
{
	struct eth_dev *dev = net->priv;

	memcpy(dev->dev_mac, mac, ETH_ALEN);
	return 0;
}

static int gether_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct eth_dev *dev = edev->priv;

	if (is_valid_ether_addr(dev->dev_mac)) {
		memcpy(adr, dev->dev_mac, ETH_ALEN);
		return 0;
	}

	/* Prefer an address that stays the same across boots */
	return generate_ether_addr(adr, edev->dev.id);
}

struct eth_device *gether_setup_default(void)
{
	struct eth_dev *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	INIT_LIST_HEAD(&dev->tx_reqs);
	INIT_LIST_HEAD(&dev->rx_reqs);
	INIT_LIST_HEAD(&dev->rx_frames);

	/* network device setup */
	dev->qmult = QMULT_DEFAULT;

	/* The device address is assigned by the net core at registration */
	random_ether_addr(dev->host_mac);

	dev->net.priv = dev;

	dev->net.open		= gether_open;
	dev->net.halt		= gether_stop;
	dev->net.send		= gether_start_xmit;
	dev->net.recv		= gether_recv;
	dev->net.set_ethaddr	= gether_set_ethaddr;
	dev->net.get_ethaddr	= gether_get_ethaddr;

	return &dev->net;
}
EXPORT_SYMBOL_GPL(gether_setup_default);

int gether_register_netdev(struct eth_device *net)
{
	struct eth_dev *dev = net->priv;
	struct usb_gadget *g;
	int status;

	if (!net->parent)
		return -EINVAL;
	g = dev->gadget;
	status = eth_register(net);
	if (status < 0) {
		dev_dbg(&g->dev, "register_netdev failed, %d\n", status);
		return status;
	}

	INFO(dev, "HOST MAC %pM, DEV MAC %pM\n", dev->host_mac, dev->dev_mac);

	return status;
}
EXPORT_SYMBOL_GPL(gether_register_netdev);

void gether_set_gadget(struct eth_device *net, struct usb_gadget *g)
{
	struct eth_dev *dev;

	dev = net->priv;
	dev->gadget = g;
	net->parent = &g->dev;
}
EXPORT_SYMBOL_GPL(gether_set_gadget);

int gether_get_host_addr(struct eth_device *net, char *host_addr, int len)
{
	struct eth_dev *dev;
	int ret;

	dev = net->priv;
	ret = get_ether_addr_str(dev->host_mac, host_addr, len);
	if (ret + 1 < len) {
		host_addr[ret++] = '\n';
		host_addr[ret] = '\0';
	}

	return ret;
}
EXPORT_SYMBOL_GPL(gether_get_host_addr);

int gether_get_host_addr_cdc(struct eth_device *net, char *host_addr, int len)
{
	struct eth_dev *dev;

	if (len < 13)
		return -EINVAL;

	dev = net->priv;
	snprintf(host_addr, len, "%pm", dev->host_mac);

	return strlen(host_addr);
}
EXPORT_SYMBOL_GPL(gether_get_host_addr_cdc);

void gether_get_host_addr_u8(struct eth_device *net, u8 host_mac[ETH_ALEN])
{
	struct eth_dev *dev;

	dev = net->priv;
	memcpy(host_mac, dev->host_mac, ETH_ALEN);
}
EXPORT_SYMBOL_GPL(gether_get_host_addr_u8);

void gether_set_qmult(struct eth_device *net, unsigned qmult)
{
	struct eth_dev *dev;

	dev = net->priv;
	dev->qmult = qmult;
}
EXPORT_SYMBOL_GPL(gether_set_qmult);

unsigned gether_get_qmult(struct eth_device *net)
{
	struct eth_dev *dev;

	dev = net->priv;
	return dev->qmult;
}
EXPORT_SYMBOL_GPL(gether_get_qmult);

/*
 * gether_cleanup - remove Ethernet-over-USB device
 * Context: may sleep
 *
 * This is called to free all resources allocated by @gether_setup().
 */
void gether_cleanup(struct eth_dev *dev)
{
	struct sk_buff *skb, *tmp;

	if (!dev)
		return;

	eth_unregister(&dev->net);

	/* free any remaining rx_frames */
	list_for_each_entry_safe(skb, tmp, &dev->rx_frames, list) {
		list_del(&skb->list);
		dev_kfree_skb_any(skb);
	}

	kfree(dev);
}
EXPORT_SYMBOL_GPL(gether_cleanup);

/**
 * gether_connect - notify network layer that USB link is active
 * @link: the USB link, set up with endpoints, descriptors matching
 *	current device speed, and any framing wrapper(s) set up.
 *
 * This is called to activate endpoints and let the network layer know
 * the connection is active ("carrier detect").  It may cause the I/O
 * queues to open and start letting network packets flow, but will in
 * any case activate the endpoints so that they respond properly to the
 * USB host.
 *
 * Verify eth_device pointer returned using IS_ERR().  If it doesn't
 * indicate some error code (negative errno), ep->driver_data values
 * have been overwritten.
 */
struct eth_device *gether_connect(struct gether *link)
{
	struct eth_dev		*dev = link->ioport;
	int			result = 0;

	if (!dev)
		return ERR_PTR(-EINVAL);

	link->in_ep->driver_data = dev;
	result = usb_ep_enable(link->in_ep);
	if (result != 0) {
		DBG(dev, "enable %s --> %d\n",
			link->in_ep->name, result);
		goto fail0;
	}

	link->out_ep->driver_data = dev;
	result = usb_ep_enable(link->out_ep);
	if (result != 0) {
		DBG(dev, "enable %s --> %d\n",
			link->out_ep->name, result);
		goto fail1;
	}

	if (result == 0)
		result = alloc_requests(dev, link, qlen(dev->gadget,
					dev->qmult));

	if (result == 0) {
		dev->zlp = link->is_zlp_ok;
		DBG(dev, "qlen %d\n", qlen(dev->gadget, dev->qmult));

		dev->header_len = link->header_len;
		dev->tailroom = link->tailroom;
		dev->unwrap = link->unwrap;
		dev->wrap = link->wrap;

		dev->port_usb = link;
		if (netif_running(&dev->net)) {
			if (link->open)
				link->open(link);
		} else {
			if (link->close)
				link->close(link);
		}

		if (netif_running(&dev->net))
			eth_start(dev);

	/* on error, disable any endpoints  */
	} else {
		(void) usb_ep_disable(link->out_ep);
fail1:
		(void) usb_ep_disable(link->in_ep);
	}
fail0:
	/* caller is responsible for cleanup on error */
	if (result < 0)
		return ERR_PTR(result);
	return &dev->net;
}
EXPORT_SYMBOL_GPL(gether_connect);

/**
 * gether_disconnect - notify network layer that USB link is inactive
 * @link: the USB link, on which gether_connect() was called
 *
 * This is called to deactivate endpoints and let the network layer know
 * the connection went inactive ("no carrier").
 *
 * On return, the state is as if gether_connect() had never been called.
 * The endpoints are inactive, and accordingly without active USB I/O.
 * Pointers to endpoint descriptors and endpoint private data are nulled.
 */
void gether_disconnect(struct gether *link)
{
	struct eth_dev		*dev = link->ioport;
	struct usb_request	*req;

	WARN_ON(!dev);
	if (!dev)
		return;

	DBG(dev, "%s\n", __func__);

	/* disable endpoints, forcing (synchronous) completion
	 * of all pending i/o.  then free the request objects
	 * and forget about the endpoints.
	 */
	usb_ep_disable(link->in_ep);
	while (!list_empty(&dev->tx_reqs)) {
		req = list_first_entry(&dev->tx_reqs, struct usb_request, list);
		list_del(&req->list);

		usb_ep_free_request(link->in_ep, req);
	}
	link->in_ep->desc = NULL;

	usb_ep_disable(link->out_ep);
	while (!list_empty(&dev->rx_reqs)) {
		req = list_first_entry(&dev->rx_reqs, struct usb_request, list);
		list_del(&req->list);

		dev_kfree_skb_any(req->context);
		usb_ep_free_request(link->out_ep, req);
	}
	link->out_ep->desc = NULL;

	/* finish forgetting about this USB link episode */
	dev->header_len = 0;
	dev->tailroom = 0;
	dev->unwrap = NULL;
	dev->wrap = NULL;

	dev->port_usb = NULL;
}
EXPORT_SYMBOL_GPL(gether_disconnect);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Brownell");
