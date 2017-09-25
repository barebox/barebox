/*
 * Handles the Intel 27x USB Device Controller (UDC)
 *
 * Inspired by original driver by Frank Becker, David Brownell, and others.
 * Copyright (C) 2008 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Taken from linux-2.6 kernel and adapted to barebox.
 */
#include <common.h>
#include <errno.h>
#include <clock.h>
#include <io.h>
#include <gpio.h>
#include <init.h>

#include <usb/ch9.h>
#include <usb/gadget.h>

#include "pxa27x_udc.h"
#include <mach/udc_pxa2xx.h>
#include <mach/pxa-regs.h>

#define	DRIVER_VERSION	"2008-04-18"
#define	DRIVER_DESC	"PXA 27x USB Device Controller driver"

static const char driver_name[] = "pxa27x_udc";
static struct pxa_udc *the_controller;

static void handle_ep(struct pxa_ep *ep);

static int is_match_usb_pxa(struct udc_usb_ep *udc_usb_ep, struct pxa_ep *ep,
		int config, int interface, int altsetting)
{
	if (usb_endpoint_num(&udc_usb_ep->desc) != ep->addr)
		return 0;
	if (usb_endpoint_dir_in(&udc_usb_ep->desc) != ep->dir_in)
		return 0;
	if (usb_endpoint_type(&udc_usb_ep->desc) != ep->type)
		return 0;
	if ((ep->config != config) || (ep->interface != interface)
			|| (ep->alternate != altsetting))
		return 0;
	return 1;
}

static struct pxa_ep *find_pxa_ep(struct pxa_udc *udc,
		struct udc_usb_ep *udc_usb_ep)
{
	int i;
	struct pxa_ep *ep;
	int cfg = udc->config;
	int iface = udc->last_interface;
	int alt = udc->last_alternate;

	if (udc_usb_ep == &udc->udc_usb_ep[0])
		return &udc->pxa_ep[0];

	for (i = 1; i < NR_PXA_ENDPOINTS; i++) {
		ep = &udc->pxa_ep[i];
		if (is_match_usb_pxa(udc_usb_ep, ep, cfg, iface, alt))
			return ep;
	}
	return NULL;
}

static void update_pxa_ep_matches(struct pxa_udc *udc)
{
	int i;
	struct udc_usb_ep *udc_usb_ep;

	for (i = 1; i < NR_USB_ENDPOINTS; i++) {
		udc_usb_ep = &udc->udc_usb_ep[i];
		if (udc_usb_ep->pxa_ep)
			udc_usb_ep->pxa_ep = find_pxa_ep(udc, udc_usb_ep);
	}
}

static void pio_irq_enable(struct pxa_ep *ep)
{
	struct pxa_udc *udc = ep->dev;
	int index = EPIDX(ep);
	u32 udcicr0 = udc_readl(udc, UDCICR0);
	u32 udcicr1 = udc_readl(udc, UDCICR1);

	if (index < 16)
		udc_writel(udc, UDCICR0, udcicr0 | (3 << (index * 2)));
	else
		udc_writel(udc, UDCICR1, udcicr1 | (3 << ((index - 16) * 2)));
}

static void pio_irq_disable(struct pxa_ep *ep)
{
	struct pxa_udc *udc = ep->dev;
	int index = EPIDX(ep);
	u32 udcicr0 = udc_readl(udc, UDCICR0);
	u32 udcicr1 = udc_readl(udc, UDCICR1);

	if (index < 16)
		udc_writel(udc, UDCICR0, udcicr0 & ~(3 << (index * 2)));
	else
		udc_writel(udc, UDCICR1, udcicr1 & ~(3 << ((index - 16) * 2)));
}

static inline void udc_set_mask_UDCCR(struct pxa_udc *udc, int mask)
{
	u32 udccr = udc_readl(udc, UDCCR);
	udc_writel(udc, UDCCR,
			(udccr & UDCCR_MASK_BITS) | (mask & UDCCR_MASK_BITS));
}

static inline void udc_clear_mask_UDCCR(struct pxa_udc *udc, int mask)
{
	u32 udccr = udc_readl(udc, UDCCR);
	udc_writel(udc, UDCCR,
			(udccr & UDCCR_MASK_BITS) & ~(mask & UDCCR_MASK_BITS));
}

static inline void ep_write_UDCCSR(struct pxa_ep *ep, int mask)
{
	if (is_ep0(ep))
		mask |= UDCCSR0_ACM;
	udc_ep_writel(ep, UDCCSR, mask);
}

static int ep_count_bytes_remain(struct pxa_ep *ep)
{
	if (ep->dir_in)
		return -EOPNOTSUPP;
	return udc_ep_readl(ep, UDCBCR) & 0x3ff;
}

static int ep_is_empty(struct pxa_ep *ep)
{
	int ret;

	if (!is_ep0(ep) && ep->dir_in)
		return -EOPNOTSUPP;
	if (is_ep0(ep))
		ret = !(udc_ep_readl(ep, UDCCSR) & UDCCSR0_RNE);
	else
		ret = !(udc_ep_readl(ep, UDCCSR) & UDCCSR_BNE);
	return ret;
}

static int ep_is_full(struct pxa_ep *ep)
{
	if (is_ep0(ep))
		return udc_ep_readl(ep, UDCCSR) & UDCCSR0_IPR;
	if (!ep->dir_in)
		return -EOPNOTSUPP;
	return !(udc_ep_readl(ep, UDCCSR) & UDCCSR_BNF);
}

static int epout_has_pkt(struct pxa_ep *ep)
{
	if (!is_ep0(ep) && ep->dir_in)
		return -EOPNOTSUPP;
	if (is_ep0(ep))
		return udc_ep_readl(ep, UDCCSR) & UDCCSR0_OPC;
	return udc_ep_readl(ep, UDCCSR) & UDCCSR_PC;
}

static void set_ep0state(struct pxa_udc *udc, int state)
{
	struct pxa_ep *ep = &udc->pxa_ep[0];
	char *old_stname = EP0_STNAME(udc);

	udc->ep0state = state;
	ep_dbg(ep, "state=%s->%s, udccsr0=0x%03x, udcbcr=%d\n", old_stname,
		EP0_STNAME(udc), udc_ep_readl(ep, UDCCSR),
		udc_ep_readl(ep, UDCBCR));
}

static void ep0_idle(struct pxa_udc *dev)
{
	set_ep0state(dev, WAIT_FOR_SETUP);
}

static __init void pxa_ep_setup(struct pxa_ep *ep)
{
	u32 new_udccr;

	new_udccr = ((ep->config << UDCCONR_CN_S) & UDCCONR_CN)
		| ((ep->interface << UDCCONR_IN_S) & UDCCONR_IN)
		| ((ep->alternate << UDCCONR_AISN_S) & UDCCONR_AISN)
		| ((EPADDR(ep) << UDCCONR_EN_S) & UDCCONR_EN)
		| ((EPXFERTYPE(ep) << UDCCONR_ET_S) & UDCCONR_ET)
		| ((ep->dir_in) ? UDCCONR_ED : 0)
		| ((ep->fifo_size << UDCCONR_MPS_S) & UDCCONR_MPS)
		| UDCCONR_EE;

	udc_ep_writel(ep, UDCCR, new_udccr);
}

static __init void pxa_eps_setup(struct pxa_udc *dev)
{
	unsigned int i;

	dev_dbg(dev->dev, "%s: dev=%p\n", __func__, dev);

	for (i = 1; i < NR_PXA_ENDPOINTS; i++)
		pxa_ep_setup(&dev->pxa_ep[i]);
}

static struct usb_request *pxa_ep_alloc_request(struct usb_ep *_ep)
{
	struct pxa27x_request *req;

	req = xzalloc(sizeof *req);

	INIT_LIST_HEAD(&req->queue);
	req->in_use = 0;
	req->udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);

	return &req->req;
}

static void pxa_ep_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct pxa27x_request *req;

	req = container_of(_req, struct pxa27x_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

static void ep_add_request(struct pxa_ep *ep, struct pxa27x_request *req)
{
	if (unlikely(!req))
		return;
	ep_vdbg(ep, "req:%p, lg=%d, udccsr=0x%03x\n", req,
		req->req.length, udc_ep_readl(ep, UDCCSR));

	req->in_use = 1;
	list_add_tail(&req->queue, &ep->queue);
	pio_irq_enable(ep);
}

static void ep_del_request(struct pxa_ep *ep, struct pxa27x_request *req)
{
	if (unlikely(!req))
		return;
	ep_vdbg(ep, "req:%p, lg=%d, udccsr=0x%03x\n", req,
		req->req.length, udc_ep_readl(ep, UDCCSR));

	list_del_init(&req->queue);
	req->in_use = 0;
	if (!is_ep0(ep) && list_empty(&ep->queue))
		pio_irq_disable(ep);
}

static void req_done(struct pxa_ep *ep, struct pxa27x_request *req, int status)
{
	ep_del_request(ep, req);
	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	if (status && status != -ESHUTDOWN)
		ep_dbg(ep, "complete req %p stat %d len %u/%u\n",
			&req->req, status,
			req->req.actual, req->req.length);

	req->req.complete(&req->udc_usb_ep->usb_ep, &req->req);
}

static void ep_end_out_req(struct pxa_ep *ep, struct pxa27x_request *req)
{
	req_done(ep, req, 0);
}

static void ep0_end_out_req(struct pxa_ep *ep, struct pxa27x_request *req)
{
	set_ep0state(ep->dev, OUT_STATUS_STAGE);
	ep_end_out_req(ep, req);
	ep0_idle(ep->dev);
}

static void ep_end_in_req(struct pxa_ep *ep, struct pxa27x_request *req)
{
	req_done(ep, req, 0);
}

static void ep0_end_in_req(struct pxa_ep *ep, struct pxa27x_request *req)
{
	set_ep0state(ep->dev, IN_STATUS_STAGE);
	ep_end_in_req(ep, req);
}

static void nuke(struct pxa_ep *ep, int status)
{
	struct pxa27x_request	*req;

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct pxa27x_request, queue);
		req_done(ep, req, status);
	}
}

static int read_packet(struct pxa_ep *ep, struct pxa27x_request *req)
{
	u32 *buf;
	int bytes_ep, bufferspace, count, i;

	bytes_ep = ep_count_bytes_remain(ep);
	bufferspace = req->req.length - req->req.actual;

	buf = (u32 *)(req->req.buf + req->req.actual);

	if (likely(!ep_is_empty(ep)))
		count = min(bytes_ep, bufferspace);
	else /* zlp */
		count = 0;

	for (i = count; i > 0; i -= 4)
		*buf++ = udc_ep_readl(ep, UDCDR);
	req->req.actual += count;

	ep_write_UDCCSR(ep, UDCCSR_PC);

	return count;
}

static int write_packet(struct pxa_ep *ep, struct pxa27x_request *req,
			unsigned int max)
{
	int length, count, remain, i;
	u32 *buf;
	u8 *buf_8;

	buf = (u32 *)(req->req.buf + req->req.actual);
	prefetch(buf);

	length = min(req->req.length - req->req.actual, max);
	req->req.actual += length;

	remain = length & 0x3;
	count = length & ~(0x3);
	for (i = count; i > 0 ; i -= 4)
		udc_ep_writel(ep, UDCDR, *buf++);

	buf_8 = (u8 *)buf;
	for (i = remain; i > 0; i--)
		udc_ep_writeb(ep, UDCDR, *buf_8++);

	ep_vdbg(ep, "length=%d+%d, udccsr=0x%03x\n", count, remain,
		udc_ep_readl(ep, UDCCSR));

	return length;
}

static int read_fifo(struct pxa_ep *ep, struct pxa27x_request *req)
{
	int count, is_short, completed = 0;

	while (epout_has_pkt(ep)) {
		count = read_packet(ep, req);

		is_short = (count < ep->fifo_size);
		ep_dbg(ep, "read udccsr:%03x, count:%d bytes%s req %p %d/%d\n",
			udc_ep_readl(ep, UDCCSR), count, is_short ? "/S" : "",
			&req->req, req->req.actual, req->req.length);

		/* completion */
		if (is_short || req->req.actual == req->req.length) {
			completed = 1;
			break;
		}
		/* finished that packet.  the next one may be waiting... */
	}
	return completed;
}

static int write_fifo(struct pxa_ep *ep, struct pxa27x_request *req)
{
	unsigned max;
	int count, is_short, is_last = 0, completed = 0, totcount = 0;
	u32 udccsr;

	max = ep->fifo_size;
	do {
		is_short = 0;

		udccsr = udc_ep_readl(ep, UDCCSR);
		if (udccsr & UDCCSR_PC) {
			ep_vdbg(ep, "Clearing Transmit Complete, udccsr=%x\n",
				udccsr);
			ep_write_UDCCSR(ep, UDCCSR_PC);
		}
		if (udccsr & UDCCSR_TRN) {
			ep_vdbg(ep, "Clearing Underrun on, udccsr=%x\n",
				udccsr);
			ep_write_UDCCSR(ep, UDCCSR_TRN);
		}

		count = write_packet(ep, req, max);
		totcount += count;

		/* last packet is usually short (or a zlp) */
		if (unlikely(count < max)) {
			is_last = 1;
			is_short = 1;
		} else {
			if (likely(req->req.length > req->req.actual)
					|| req->req.zero)
				is_last = 0;
			else
				is_last = 1;
			/* interrupt/iso maxpacket may not fill the fifo */
			is_short = unlikely(max < ep->fifo_size);
		}

		if (is_short)
			ep_write_UDCCSR(ep, UDCCSR_SP);

		/* requests complete when all IN data is in the FIFO */
		if (is_last) {
			completed = 1;
			break;
		}
	} while (!ep_is_full(ep));

	ep_dbg(ep, "wrote count:%d bytes%s%s, left:%d req=%p\n",
			totcount, is_last ? "/L" : "", is_short ? "/S" : "",
			req->req.length - req->req.actual, &req->req);

	return completed;
}

static int read_ep0_fifo(struct pxa_ep *ep, struct pxa27x_request *req)
{
	int count, is_short, completed = 0;

	while (epout_has_pkt(ep)) {
		count = read_packet(ep, req);
		ep_write_UDCCSR(ep, UDCCSR0_OPC);

		is_short = (count < ep->fifo_size);
		ep_dbg(ep, "read udccsr:%03x, count:%d bytes%s req %p %d/%d\n",
			udc_ep_readl(ep, UDCCSR), count, is_short ? "/S" : "",
			&req->req, req->req.actual, req->req.length);

		if (is_short || req->req.actual >= req->req.length) {
			completed = 1;
			break;
		}
	}

	return completed;
}

static int write_ep0_fifo(struct pxa_ep *ep, struct pxa27x_request *req)
{
	unsigned	count;
	int		is_last, is_short;

	count = write_packet(ep, req, EP0_FIFO_SIZE);

	is_short = (count < EP0_FIFO_SIZE);
	is_last = ((count == 0) || (count < EP0_FIFO_SIZE));

	/* Sends either a short packet or a 0 length packet */
	if (unlikely(is_short))
		ep_write_UDCCSR(ep, UDCCSR0_IPR);

	ep_dbg(ep, "in %d bytes%s%s, %d left, req=%p, udccsr0=0x%03x\n",
		count, is_short ? "/S" : "", is_last ? "/L" : "",
		req->req.length - req->req.actual,
		&req->req, udc_ep_readl(ep, UDCCSR));

	return is_last;
}

static int pxa_ep_queue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct udc_usb_ep	*udc_usb_ep;
	struct pxa_ep		*ep;
	struct pxa27x_request	*req;
	struct pxa_udc		*dev;
	int			rc = 0;
	int			is_first_req;
	unsigned		length;

	req = container_of(_req, struct pxa27x_request, req);
	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);

	if (unlikely(!_req || !_req->complete || !_req->buf))
		return -EINVAL;

	if (unlikely(!_ep))
		return -EINVAL;

	dev = udc_usb_ep->dev;
	ep = udc_usb_ep->pxa_ep;
	if (unlikely(!ep))
		return -EINVAL;

	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		ep_dbg(ep, "bogus device state\n");
		return -ESHUTDOWN;
	}

	/* iso is always one packet per request, that's the only way
	 * we can report per-packet status.  that also helps with dma.
	 */
	if (unlikely(EPXFERTYPE_is_ISO(ep)
			&& req->req.length > ep->fifo_size))
		return -EMSGSIZE;

	is_first_req = list_empty(&ep->queue);
	ep_dbg(ep, "queue req %p(first=%s), len %d buf %p\n",
			_req, is_first_req ? "yes" : "no",
			_req->length, _req->buf);

	if (!ep->enabled) {
		_req->status = -ESHUTDOWN;
		rc = -ESHUTDOWN;
		goto out_locked;
	}

	if (req->in_use) {
		ep_err(ep, "refusing to queue req %p (already queued)\n", req);
		goto out_locked;
	}

	length = _req->length;
	_req->status = -EINPROGRESS;
	_req->actual = 0;

	ep_add_request(ep, req);

	if (is_ep0(ep)) {
		switch (dev->ep0state) {
		case WAIT_ACK_SET_CONF_INTERF:
			if (length == 0) {
				ep_end_in_req(ep, req);
			} else {
				ep_err(ep, "got a request of %d bytes while"
					"in state WAIT_ACK_SET_CONF_INTERF\n",
					length);
				ep_del_request(ep, req);
				rc = -EL2HLT;
			}
			ep0_idle(ep->dev);
			break;
		case IN_DATA_STAGE:
			if (!ep_is_full(ep))
				if (write_ep0_fifo(ep, req))
					ep0_end_in_req(ep, req);
			break;
		case OUT_DATA_STAGE:
			if ((length == 0) || !epout_has_pkt(ep))
				if (read_ep0_fifo(ep, req))
					ep0_end_out_req(ep, req);
			break;
		default:
			ep_err(ep, "odd state %s to send me a request\n",
				EP0_STNAME(ep->dev));
			ep_del_request(ep, req);
			rc = -EL2HLT;
			break;
		}
	} else {
		handle_ep(ep);
	}

out:
	return rc;
out_locked:
	goto out;
}

static int pxa_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct pxa_ep		*ep;
	struct udc_usb_ep	*udc_usb_ep;
	struct pxa27x_request	*req;
	int			rc = -EINVAL;

	if (!_ep)
		return rc;
	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);
	ep = udc_usb_ep->pxa_ep;
	if (!ep || is_ep0(ep))
		return rc;

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req) {
			rc = 0;
			break;
		}
	}

	if (!rc)
		req_done(ep, req, -ECONNRESET);
	return rc;
}

static int pxa_ep_set_halt(struct usb_ep *_ep, int value)
{
	struct pxa_ep		*ep;
	struct udc_usb_ep	*udc_usb_ep;
	int rc;


	if (!_ep)
		return -EINVAL;
	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);
	ep = udc_usb_ep->pxa_ep;
	if (!ep || is_ep0(ep))
		return -EINVAL;

	if (value == 0) {
		/*
		 * This path (reset toggle+halt) is needed to implement
		 * SET_INTERFACE on normal hardware.  but it can't be
		 * done from software on the PXA UDC, and the hardware
		 * forgets to do it as part of SET_INTERFACE automagic.
		 */
		ep_dbg(ep, "only host can clear halt\n");
		return -EROFS;
	}

	rc = -EAGAIN;
	if (ep->dir_in	&& (ep_is_full(ep) || !list_empty(&ep->queue)))
		goto out;

	/* FST, FEF bits are the same for control and non control endpoints */
	rc = 0;
	ep_write_UDCCSR(ep, UDCCSR_FST | UDCCSR_FEF);
	if (is_ep0(ep))
		set_ep0state(ep->dev, STALL);

out:
	return rc;
}

static int pxa_ep_fifo_status(struct usb_ep *_ep)
{
	struct pxa_ep		*ep;
	struct udc_usb_ep	*udc_usb_ep;

	if (!_ep)
		return -ENODEV;
	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);
	ep = udc_usb_ep->pxa_ep;
	if (!ep || is_ep0(ep))
		return -ENODEV;

	if (ep->dir_in)
		return -EOPNOTSUPP;
	if (ep->dev->gadget.speed == USB_SPEED_UNKNOWN || ep_is_empty(ep))
		return 0;
	else
		return ep_count_bytes_remain(ep) + 1;
}

static void pxa_ep_fifo_flush(struct usb_ep *_ep)
{
	struct pxa_ep		*ep;
	struct udc_usb_ep	*udc_usb_ep;

	if (!_ep)
		return;
	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);
	ep = udc_usb_ep->pxa_ep;
	if (!ep || is_ep0(ep))
		return;

	if (unlikely(!list_empty(&ep->queue)))
		ep_dbg(ep, "called while queue list not empty\n");
	ep_dbg(ep, "called\n");

	/* for OUT, just read and discard the FIFO contents. */
	if (!ep->dir_in) {
		while (!ep_is_empty(ep))
			udc_ep_readl(ep, UDCDR);
	} else {
		/* most IN status is the same, but ISO can't stall */
		ep_write_UDCCSR(ep,
				UDCCSR_PC | UDCCSR_FEF | UDCCSR_TRN
				| (EPXFERTYPE_is_ISO(ep) ? 0 : UDCCSR_SST));
	}
}

static int pxa_ep_enable(struct usb_ep *_ep,
	const struct usb_endpoint_descriptor *desc)
{
	struct pxa_ep		*ep;
	struct udc_usb_ep	*udc_usb_ep;
	struct pxa_udc		*udc;

	if (!_ep || !desc)
		return -EINVAL;

	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);
	if (udc_usb_ep->pxa_ep) {
		ep = udc_usb_ep->pxa_ep;
		ep_warn(ep, "usb_ep %s already enabled, doing nothing\n",
			_ep->name);
	} else {
		ep = find_pxa_ep(udc_usb_ep->dev, udc_usb_ep);
	}

	if (!ep || is_ep0(ep)) {
		dev_err(udc_usb_ep->dev->dev,
			"unable to match pxa_ep for ep %s\n",
			_ep->name);
		return -EINVAL;
	}

	if ((desc->bDescriptorType != USB_DT_ENDPOINT)
			|| (ep->type != usb_endpoint_type(desc))) {
		ep_err(ep, "type mismatch\n");
		return -EINVAL;
	}

	if (ep->fifo_size < le16_to_cpu(desc->wMaxPacketSize)) {
		ep_err(ep, "bad maxpacket\n");
		return -ERANGE;
	}

	udc_usb_ep->pxa_ep = ep;
	udc = ep->dev;

	if (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN) {
		ep_err(ep, "bogus device state\n");
		return -ESHUTDOWN;
	}

	ep->enabled = 1;

	/* flush fifo (mostly for OUT buffers) */
	pxa_ep_fifo_flush(_ep);

	ep_dbg(ep, "enabled\n");
	return 0;
}

static int pxa_ep_disable(struct usb_ep *_ep)
{
	struct pxa_ep		*ep;
	struct udc_usb_ep	*udc_usb_ep;

	if (!_ep)
		return -EINVAL;

	udc_usb_ep = container_of(_ep, struct udc_usb_ep, usb_ep);
	ep = udc_usb_ep->pxa_ep;
	if (!ep || is_ep0(ep) || !list_empty(&ep->queue))
		return -EINVAL;

	ep->enabled = 0;
	nuke(ep, -ESHUTDOWN);

	pxa_ep_fifo_flush(_ep);
	udc_usb_ep->pxa_ep = NULL;

	ep_dbg(ep, "disabled\n");
	return 0;
}

static struct usb_ep_ops pxa_ep_ops = {
	.enable		= pxa_ep_enable,
	.disable	= pxa_ep_disable,

	.alloc_request	= pxa_ep_alloc_request,
	.free_request	= pxa_ep_free_request,

	.queue		= pxa_ep_queue,
	.dequeue	= pxa_ep_dequeue,

	.set_halt	= pxa_ep_set_halt,
	.fifo_status	= pxa_ep_fifo_status,
	.fifo_flush	= pxa_ep_fifo_flush,
};

static void dplus_pullup(struct pxa_udc *udc, int on)
{
	if (on) {
		if (udc->mach->gpio_pullup > 0)
			gpio_set_value(udc->mach->gpio_pullup,
				       !udc->mach->gpio_pullup_inverted);
		if (udc->mach->udc_command)
			udc->mach->udc_command(PXA2XX_UDC_CMD_CONNECT);
	} else {
		if (udc->mach->gpio_pullup > 0)
			gpio_set_value(udc->mach->gpio_pullup,
				       udc->mach->gpio_pullup_inverted);
		if (udc->mach->udc_command)
			udc->mach->udc_command(PXA2XX_UDC_CMD_DISCONNECT);
	}
	udc->pullup_on = on;
}

/**
 * pxa_udc_get_frame - Returns usb frame number
 * @_gadget: usb gadget
 */
static int pxa_udc_get_frame(struct usb_gadget *_gadget)
{
	struct pxa_udc *udc = to_gadget_udc(_gadget);

	return udc_readl(udc, UDCFNR) & 0x7ff;
}

static int pxa_udc_wakeup(struct usb_gadget *_gadget)
{
	struct pxa_udc *udc = to_gadget_udc(_gadget);

	/* host may not have enabled remote wakeup */
	if ((udc_readl(udc, UDCCR) & UDCCR_DWRE) == 0)
		return -EHOSTUNREACH;
	udc_set_mask_UDCCR(udc, UDCCR_UDR);
	return 0;
}

static void udc_enable(struct pxa_udc *udc);
static void udc_disable(struct pxa_udc *udc);

static int should_enable_udc(struct pxa_udc *udc)
{
	int put_on;

	put_on = ((udc->pullup_on) && (udc->driver));
	put_on &= udc->vbus_sensed;
	return put_on;
}

static int should_disable_udc(struct pxa_udc *udc)
{
	int put_off;

	put_off = ((!udc->pullup_on) || (!udc->driver));
	put_off |= !udc->vbus_sensed;
	return put_off;
}

static int pxa_udc_pullup(struct usb_gadget *_gadget, int is_active)
{
	struct pxa_udc *udc = to_gadget_udc(_gadget);

	if ((udc->mach->gpio_pullup < 0) && !udc->mach->udc_command)
		return -EOPNOTSUPP;

	dplus_pullup(udc, is_active);

	if (should_enable_udc(udc))
		udc_enable(udc);
	if (should_disable_udc(udc))
		udc_disable(udc);
	return 0;
}

static int pxa_udc_vbus_session(struct usb_gadget *_gadget, int is_active)
{
	struct pxa_udc *udc = to_gadget_udc(_gadget);

	udc->vbus_sensed = is_active;
	if (should_enable_udc(udc))
		udc_enable(udc);
	if (should_disable_udc(udc))
		udc_disable(udc);

	return 0;
}

static int pxa_udc_start(struct usb_gadget *gadget, struct usb_gadget_driver *driver);
static int pxa_udc_stop(struct usb_gadget *gadget, struct usb_gadget_driver *driver);
static void pxa_udc_gadget_poll(struct usb_gadget *gadget);

static const struct usb_gadget_ops pxa_udc_ops = {
	.get_frame	= pxa_udc_get_frame,
	.wakeup		= pxa_udc_wakeup,
	.pullup		= pxa_udc_pullup,
	.vbus_session	= pxa_udc_vbus_session,
	.udc_start	= pxa_udc_start,
	.udc_stop	= pxa_udc_stop,
	.udc_poll	= pxa_udc_gadget_poll,
};

static void clk_enable(void)
{
	CKEN |= CKEN_USB;
}

static void clk_disable(void)
{
	CKEN &= ~CKEN_USB;
}

static void udc_disable(struct pxa_udc *udc)
{
	if (!udc->enabled)
		return;

	udc_writel(udc, UDCICR0, 0);
	udc_writel(udc, UDCICR1, 0);

	udc_clear_mask_UDCCR(udc, UDCCR_UDE);
	clk_disable();

	ep0_idle(udc);
	udc->gadget.speed = USB_SPEED_UNKNOWN;

	udc->enabled = 0;
}

static __init void udc_init_data(struct pxa_udc *dev)
{
	int i;
	struct pxa_ep *ep;

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->udc_usb_ep[0].pxa_ep = &dev->pxa_ep[0];
	ep0_idle(dev);

	/* PXA endpoints init */
	for (i = 0; i < NR_PXA_ENDPOINTS; i++) {
		ep = &dev->pxa_ep[i];

		ep->enabled = is_ep0(ep);
		INIT_LIST_HEAD(&ep->queue);
	}

	/* USB endpoints init */
	for (i = 1; i < NR_USB_ENDPOINTS; i++)
		list_add_tail(&dev->udc_usb_ep[i].usb_ep.ep_list,
				&dev->gadget.ep_list);
}

static void udc_enable(struct pxa_udc *udc)
{
	if (udc->enabled)
		return;

	udc_writel(udc, UDCICR0, 0);
	udc_writel(udc, UDCICR1, 0);
	udc_clear_mask_UDCCR(udc, UDCCR_UDE);

	clk_enable();

	ep0_idle(udc);
	udc->gadget.speed = USB_SPEED_FULL;
	memset(&udc->stats, 0, sizeof(udc->stats));

	udc_set_mask_UDCCR(udc, UDCCR_UDE);
	ep_write_UDCCSR(&udc->pxa_ep[0], UDCCSR0_ACM);
	udelay(2);
	if (udc_readl(udc, UDCCR) & UDCCR_EMCE)
		dev_err(udc->dev, "Configuration errors, udc disabled\n");

	/*
	 * Caller must be able to sleep in order to cope with startup transients
	 */
	mdelay(100);

	/* enable suspend/resume and reset irqs */
	udc_writel(udc, UDCICR1,
			UDCICR1_IECC | UDCICR1_IERU
			| UDCICR1_IESU | UDCICR1_IERS);

	pio_irq_enable(&udc->pxa_ep[0]);
	udc->enabled = 1;
}

static int pxa_udc_start(struct usb_gadget *gadget, struct usb_gadget_driver *driver)
{
	struct pxa_udc *udc = the_controller;

	/* first hook up the driver ... */
	udc->driver = driver;

	dev_dbg(udc->dev, "registered gadget function '%s'\n",
		driver->function);

	if (should_enable_udc(udc))
		udc_enable(udc);
	return 0;
}

static void stop_activity(struct pxa_udc *udc, struct usb_gadget_driver *driver)
{
	int i;

	/* don't disconnect drivers more than once */
	if (udc->gadget.speed == USB_SPEED_UNKNOWN)
		driver = NULL;
	udc->gadget.speed = USB_SPEED_UNKNOWN;

	for (i = 0; i < NR_USB_ENDPOINTS; i++)
		pxa_ep_disable(&udc->udc_usb_ep[i].usb_ep);

	if (driver)
		driver->disconnect(&udc->gadget);
}

static int pxa_udc_stop(struct usb_gadget *gadget, struct usb_gadget_driver *driver)
{
	struct pxa_udc *udc = the_controller;

	if (!udc)
		return -ENODEV;
	if (!driver || driver != udc->driver || !driver->unbind)
		return -EINVAL;

	stop_activity(udc, driver);
	udc_disable(udc);

	driver->disconnect(&udc->gadget);
	driver->unbind(&udc->gadget);
	udc->driver = NULL;

	/*
	dev_info(udc->dev, "unregistered gadget driver '%s'\n",
		 driver->driver.name);
	*/
	return 0;
}

static void handle_ep0_ctrl_req(struct pxa_udc *udc,
				struct pxa27x_request *req)
{
	struct pxa_ep *ep = &udc->pxa_ep[0];
	union {
		struct usb_ctrlrequest	r;
		u32			word[2];
	} u;
	int i;
	int have_extrabytes = 0;

	nuke(ep, -EPROTO);

	/*
	 * In the PXA320 manual, in the section about Back-to-Back setup
	 * packets, it describes this situation.  The solution is to set OPC to
	 * get rid of the status packet, and then continue with the setup
	 * packet. Generalize to pxa27x CPUs.
	 */
	if (epout_has_pkt(ep) && (ep_count_bytes_remain(ep) == 0))
		ep_write_UDCCSR(ep, UDCCSR0_OPC);

	/* read SETUP packet */
	for (i = 0; i < 2; i++) {
		if (unlikely(ep_is_empty(ep)))
			goto stall;
		u.word[i] = udc_ep_readl(ep, UDCDR);
	}

	have_extrabytes = !ep_is_empty(ep);
	while (!ep_is_empty(ep)) {
		i = udc_ep_readl(ep, UDCDR);
		ep_err(ep, "wrong to have extra bytes for setup : 0x%08x\n", i);
	}

	ep_dbg(ep, "SETUP %02x.%02x v%04x i%04x l%04x\n",
		u.r.bRequestType, u.r.bRequest,
		le16_to_cpu(u.r.wValue), le16_to_cpu(u.r.wIndex),
		le16_to_cpu(u.r.wLength));
	if (unlikely(have_extrabytes))
		goto stall;

	if (u.r.bRequestType & USB_DIR_IN)
		set_ep0state(udc, IN_DATA_STAGE);
	else
		set_ep0state(udc, OUT_DATA_STAGE);

	/* Tell UDC to enter Data Stage */
	ep_write_UDCCSR(ep, UDCCSR0_SA | UDCCSR0_OPC);

	i = udc->driver->setup(&udc->gadget, &u.r);
	if (i < 0)
		goto stall;
out:
	return;
stall:
	ep_dbg(ep, "protocol STALL, udccsr0=%03x err %d\n",
		udc_ep_readl(ep, UDCCSR), i);
	ep_write_UDCCSR(ep, UDCCSR0_FST | UDCCSR0_FTF);
	set_ep0state(udc, STALL);
	goto out;
}

static void handle_ep0(struct pxa_udc *udc, int fifo_irq, int opc_irq)
{
	u32			udccsr0;
	struct pxa_ep		*ep = &udc->pxa_ep[0];
	struct pxa27x_request	*req = NULL;
	int			completed = 0;

	if (!list_empty(&ep->queue))
		req = list_entry(ep->queue.next, struct pxa27x_request, queue);

	udccsr0 = udc_ep_readl(ep, UDCCSR);
	ep_dbg(ep, "state=%s, req=%p, udccsr0=0x%03x, udcbcr=%d, irq_msk=%x\n",
		EP0_STNAME(udc), req, udccsr0, udc_ep_readl(ep, UDCBCR),
		(fifo_irq << 1 | opc_irq));

	if (udccsr0 & UDCCSR0_SST) {
		ep_dbg(ep, "clearing stall status\n");
		nuke(ep, -EPIPE);
		ep_write_UDCCSR(ep, UDCCSR0_SST);
		ep0_idle(udc);
	}

	if (udccsr0 & UDCCSR0_SA) {
		nuke(ep, 0);
		set_ep0state(udc, SETUP_STAGE);
	}

	switch (udc->ep0state) {
	case WAIT_FOR_SETUP:
		/*
		 * Hardware bug : beware, we cannot clear OPC, since we would
		 * miss a potential OPC irq for a setup packet.
		 * So, we only do ... nothing, and hope for a next irq with
		 * UDCCSR0_SA set.
		 */
		break;
	case SETUP_STAGE:
		udccsr0 &= UDCCSR0_CTRL_REQ_MASK;
		if (likely(udccsr0 == UDCCSR0_CTRL_REQ_MASK))
			handle_ep0_ctrl_req(udc, req);
		break;
	case IN_DATA_STAGE:			/* GET_DESCRIPTOR */
		if (epout_has_pkt(ep))
			ep_write_UDCCSR(ep, UDCCSR0_OPC);
		if (req && !ep_is_full(ep))
			completed = write_ep0_fifo(ep, req);
		if (completed)
			ep0_end_in_req(ep, req);
		break;
	case OUT_DATA_STAGE:			/* SET_DESCRIPTOR */
		if (epout_has_pkt(ep) && req)
			completed = read_ep0_fifo(ep, req);
		if (completed)
			ep0_end_out_req(ep, req);
		break;
	case STALL:
		ep_write_UDCCSR(ep, UDCCSR0_FST);
		break;
	case IN_STATUS_STAGE:
		/*
		 * Hardware bug : beware, we cannot clear OPC, since we would
		 * miss a potential PC irq for a setup packet.
		 * So, we only put the ep0 into WAIT_FOR_SETUP state.
		 */
		if (opc_irq)
			ep0_idle(udc);
		break;
	case OUT_STATUS_STAGE:
	case WAIT_ACK_SET_CONF_INTERF:
		ep_warn(ep, "should never get in %s state here!!!\n",
				EP0_STNAME(ep->dev));
		ep0_idle(udc);
		break;
	}
}

static void handle_ep(struct pxa_ep *ep)
{
	struct pxa27x_request	*req;
	int completed;
	u32 udccsr;
	int is_in = ep->dir_in;
	int loop = 0;

	do {
		completed = 0;
		udccsr = udc_ep_readl(ep, UDCCSR);

		if (likely(!list_empty(&ep->queue)))
			req = list_entry(ep->queue.next,
					struct pxa27x_request, queue);
		else
			req = NULL;

		ep_dbg(ep, "req:%p, udccsr 0x%03x loop=%d\n",
				req, udccsr, loop++);

		if (unlikely(udccsr & (UDCCSR_SST | UDCCSR_TRN)))
			udc_ep_writel(ep, UDCCSR,
					udccsr & (UDCCSR_SST | UDCCSR_TRN));
		if (!req)
			break;

		if (unlikely(is_in)) {
			if (likely(!ep_is_full(ep)))
				completed = write_fifo(ep, req);
		} else {
			if (likely(epout_has_pkt(ep)))
				completed = read_fifo(ep, req);
		}

		if (completed) {
			if (is_in)
				ep_end_in_req(ep, req);
			else
				ep_end_out_req(ep, req);
		}
	} while (completed);

	return;
}

static void pxa27x_change_configuration(struct pxa_udc *udc, int config)
{
	struct usb_ctrlrequest req ;

	dev_dbg(udc->dev, "config=%d\n", config);

	udc->config = config;
	udc->last_interface = 0;
	udc->last_alternate = 0;

	req.bRequestType = 0;
	req.bRequest = USB_REQ_SET_CONFIGURATION;
	req.wValue = config;
	req.wIndex = 0;
	req.wLength = 0;

	set_ep0state(udc, WAIT_ACK_SET_CONF_INTERF);
	udc->driver->setup(&udc->gadget, &req);
}

static void __maybe_unused pxa27x_change_interface(struct pxa_udc *udc, int iface, int alt)
{
	struct usb_ctrlrequest  req;

	dev_dbg(udc->dev, "interface=%d, alternate setting=%d\n", iface, alt);

	udc->last_interface = iface;
	udc->last_alternate = alt;

	req.bRequestType = USB_RECIP_INTERFACE;
	req.bRequest = USB_REQ_SET_INTERFACE;
	req.wValue = alt;
	req.wIndex = iface;
	req.wLength = 0;

	set_ep0state(udc, WAIT_ACK_SET_CONF_INTERF);
	ep_write_UDCCSR(&udc->pxa_ep[0], UDCCSR0_AREN);
	udc->driver->setup(&udc->gadget, &req);
}

static void irq_handle_data(struct pxa_udc *udc)
{
	int i;
	struct pxa_ep *ep;
	u32 udcisr0 = udc_readl(udc, UDCISR0) & UDCCISR0_EP_MASK;
	u32 udcisr1 = udc_readl(udc, UDCISR1) & UDCCISR1_EP_MASK;

	if (udcisr0 & UDCISR_INT_MASK) {
		udc_writel(udc, UDCISR0, UDCISR_INT(0, UDCISR_INT_MASK));
		handle_ep0(udc, !!(udcisr0 & UDCICR_FIFOERR),
				!!(udcisr0 & UDCICR_PKTCOMPL));
	}

	udcisr0 >>= 2;
	for (i = 1; udcisr0 != 0 && i < 16; udcisr0 >>= 2, i++) {
		if (!(udcisr0 & UDCISR_INT_MASK))
			continue;

		udc_writel(udc, UDCISR0, UDCISR_INT(i, UDCISR_INT_MASK));

		WARN_ON(i >= ARRAY_SIZE(udc->pxa_ep));
		if (i < ARRAY_SIZE(udc->pxa_ep)) {
			ep = &udc->pxa_ep[i];
			if (ep->enabled)
				handle_ep(ep);
		}
	}

	for (i = 16; udcisr1 != 0 && i < 24; udcisr1 >>= 2, i++) {
		udc_writel(udc, UDCISR1, UDCISR_INT(i - 16, UDCISR_INT_MASK));
		if (!(udcisr1 & UDCISR_INT_MASK))
			continue;

		WARN_ON(i >= ARRAY_SIZE(udc->pxa_ep));
		if (i < ARRAY_SIZE(udc->pxa_ep)) {
			ep = &udc->pxa_ep[i];
			if (ep->enabled)
				handle_ep(ep);
		}
	}

}

static void irq_udc_suspend(struct pxa_udc *udc)
{
	udc_writel(udc, UDCISR1, UDCISR1_IRSU);

	if (udc->gadget.speed != USB_SPEED_UNKNOWN
			&& udc->driver && udc->driver->suspend)
		udc->driver->suspend(&udc->gadget);
	ep0_idle(udc);
}

static void irq_udc_resume(struct pxa_udc *udc)
{
	udc_writel(udc, UDCISR1, UDCISR1_IRRU);

	if (udc->gadget.speed != USB_SPEED_UNKNOWN
			&& udc->driver && udc->driver->resume)
		udc->driver->resume(&udc->gadget);
}

static void irq_udc_reconfig(struct pxa_udc *udc)
{
	unsigned config, interface, alternate, config_change;
	u32 udccr = udc_readl(udc, UDCCR);

	udc_writel(udc, UDCISR1, UDCISR1_IRCC);

	config = (udccr & UDCCR_ACN) >> UDCCR_ACN_S;
	config_change = (config != udc->config);
	if (config_change)
		update_pxa_ep_matches(udc);
	udc_set_mask_UDCCR(udc, UDCCR_SMAC);
	ep_write_UDCCSR(&udc->pxa_ep[0], UDCCSR0_AREN);

	pxa27x_change_configuration(udc, config);
	interface = (udccr & UDCCR_AIN) >> UDCCR_AIN_S;
	alternate = (udccr & UDCCR_AAISN) >> UDCCR_AAISN_S;

	/*
	 * barebox specific: do not call change interface, as change_config has
	 * already setup the gadget.
	 * pxa27x_change_interface(udc, interface, alternate);
	 */
}

static void irq_udc_reset(struct pxa_udc *udc)
{
	u32 udccr = udc_readl(udc, UDCCR);
	struct pxa_ep *ep = &udc->pxa_ep[0];

	dev_info(udc->dev, "USB reset\n");
	udc_writel(udc, UDCISR1, UDCISR1_IRRS);

	if ((udccr & UDCCR_UDA) == 0) {
		dev_dbg(udc->dev, "USB reset start\n");
		stop_activity(udc, udc->driver);
	}
	udc->gadget.speed = USB_SPEED_FULL;

	nuke(ep, -EPROTO);
	ep_write_UDCCSR(ep, UDCCSR0_FTF | UDCCSR0_OPC);
	ep0_idle(udc);
}

static void pxa_udc_gadget_poll(struct usb_gadget *gadget)
{
	struct pxa_udc *udc = to_gadget_udc(gadget);
	u32 udcisr0 = udc_readl(udc, UDCISR0);
	u32 udcisr1 = udc_readl(udc, UDCISR1);
	u32 udccr = udc_readl(udc, UDCCR);
	u32 udcisr1_spec;

	udc->vbus_sensed = udc->mach->udc_is_connected();
	if (should_enable_udc(udc))
		udc_enable(udc);
	if (should_disable_udc(udc)) {
		stop_activity(udc, udc->driver);
		udc_disable(udc);
	}

	if (!udc->enabled)
		return;

	dev_dbg(udc->dev, "Interrupt, UDCISR0:0x%08x, UDCISR1:0x%08x, "
		"UDCCR:0x%08x\n", udcisr0, udcisr1, udccr);

	udcisr1_spec = udcisr1 & 0xf8000000;
	if (unlikely(udcisr1_spec & UDCISR1_IRSU))
		irq_udc_suspend(udc);
	if (unlikely(udcisr1_spec & UDCISR1_IRRU))
		irq_udc_resume(udc);
	if (unlikely(udcisr1_spec & UDCISR1_IRCC))
		irq_udc_reconfig(udc);
	if (unlikely(udcisr1_spec & UDCISR1_IRRS))
		irq_udc_reset(udc);

	if ((udcisr0 & UDCCISR0_EP_MASK) | (udcisr1 & UDCCISR1_EP_MASK))
		irq_handle_data(udc);
}

static struct pxa_udc memory = {
	.gadget = {
		.ops		= &pxa_udc_ops,
		.ep0		= &memory.udc_usb_ep[0].usb_ep,
		.name		= driver_name,
	},

	.udc_usb_ep = {
		USB_EP_CTRL,
		USB_EP_OUT_BULK(1),
		USB_EP_IN_BULK(2),
		USB_EP_IN_ISO(3),
		USB_EP_OUT_ISO(4),
		USB_EP_IN_INT(5),
	},

	.pxa_ep = {
		PXA_EP_CTRL,
		/* Endpoints for gadget zero */
		PXA_EP_OUT_BULK(1, 1, 3, 0, 0),
		PXA_EP_IN_BULK(2,  2, 3, 0, 0),
		/* Endpoints for ether gadget, file storage gadget */
		PXA_EP_OUT_BULK(3, 1, 1, 0, 0),
		PXA_EP_IN_BULK(4,  2, 1, 0, 0),
		PXA_EP_IN_ISO(5,   3, 1, 0, 0),
		PXA_EP_OUT_ISO(6,  4, 1, 0, 0),
		PXA_EP_IN_INT(7,   5, 1, 0, 0),
		/* Endpoints for RNDIS, serial */
		PXA_EP_OUT_BULK(8, 1, 2, 0, 0),
		PXA_EP_IN_BULK(9,  2, 2, 0, 0),
		PXA_EP_IN_INT(10,  5, 2, 0, 0),
		/*
		 * All the following endpoints are only for completion.  They
		 * won't never work, as multiple interfaces are really broken on
		 * the pxa.
		*/
		PXA_EP_OUT_BULK(11, 1, 2, 1, 0),
		PXA_EP_IN_BULK(12,  2, 2, 1, 0),
		/* Endpoint for CDC Ether */
		PXA_EP_OUT_BULK(13, 1, 1, 1, 1),
		PXA_EP_IN_BULK(14,  2, 1, 1, 1),
	}
};

static int __init pxa_udc_probe(struct device_d *dev)
{
	struct resource *iores;
	struct pxa_udc *udc = &memory;
	int gpio, ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	udc->regs = IOMEM(iores->start);

	udc->dev = dev;
	udc->mach = dev->platform_data;

	gpio = udc->mach->gpio_pullup;
	if (gpio >= 0) {
		gpio_direction_output(gpio,
				      udc->mach->gpio_pullup_inverted);
	}

	udc->vbus_sensed = 0;

	the_controller = udc;
	udc_init_data(udc);
	pxa_eps_setup(udc);

	ret = usb_add_gadget_udc_release(dev, &udc->gadget, NULL);
	if (ret)
		return ret;

	return 0;
}

#define pxa27x_clear_otgph()   do {} while (0)

static struct driver_d udc_driver = {
	.name		= "pxa27x-udc",
	.probe		= pxa_udc_probe,
};
device_platform_driver(udc_driver);
