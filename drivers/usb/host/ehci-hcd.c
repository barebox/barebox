/*-
 * Copyright (c) 2007-2008, Juniper Networks, Inc.
 * Copyright (c) 2008, Excito Elektronik i Skåne AB
 * Copyright (c) 2008, Michael Trimarchi <trimarchimichael@yahoo.it>
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*#define DEBUG */
#include <common.h>
#include <dma.h>
#include <asm/byteorder.h>
#include <usb/usb.h>
#include <io.h>
#include <malloc.h>
#include <driver.h>
#include <init.h>
#include <xfuncs.h>
#include <clock.h>
#include <errno.h>
#include <of.h>
#include <usb/ehci.h>
#include <linux/err.h>

#include "ehci.h"

struct ehci_host {
	int rootdev;
	struct device_d *dev;
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;
	struct usb_host host;
	struct QH *qh_list;
	struct qTD *td;
	int portreset;
	unsigned long flags;

	int (*init)(void *drvdata);
	int (*post_init)(void *drvdata);
	void *drvdata;
	int periodic_schedules;
	struct QH *periodic_queue;
	uint32_t *periodic_list;
};

struct int_queue {
	int elementsize;
	int queuesize;
	unsigned long pipe;
	struct QH *first;
	struct QH *current;
	struct QH *last;
	struct qTD *tds;
};

#define to_ehci(ptr) container_of(ptr, struct ehci_host, host)

#define NUM_QH	2
#define NUM_TD	3

static struct descriptor {
	struct usb_hub_descriptor hub;
	struct usb_device_descriptor device;
	struct usb_config_descriptor config;
	struct usb_interface_descriptor interface;
	struct usb_endpoint_descriptor endpoint;
}  __attribute__ ((packed)) descriptor = {
	.hub = {
		.bLength		= USB_DT_HUB_NONVAR_SIZE +
					  ((USB_MAXCHILDREN + 1 + 7) / 8),
		.bDescriptorType	= USB_DT_HUB,
		.bNbrPorts		= 2,	/* runtime modified */
		.wHubCharacteristics	= 0,
		.bPwrOn2PwrGood		= 10,
		.bHubContrCurrent	= 0,
		.u.hs.DeviceRemovable	= {},
		.u.hs.PortPwrCtrlMask	= {}
	},
	.device = {
		.bLength		= USB_DT_DEVICE_SIZE,
		.bDescriptorType	= USB_DT_DEVICE,
		.bcdUSB			= __constant_cpu_to_le16(0x0002), /* v2.0 */
		.bDeviceClass		= USB_CLASS_HUB,
		.bDeviceSubClass	= 0,
		.bDeviceProtocol	= 1,	/* bDeviceProtocol: UDPROTO_HSHUBSTT */
		.bMaxPacketSize0	= 64,
		.idVendor		= 0x0000,
		.idProduct		= 0x0000,
		.bcdDevice		= __constant_cpu_to_le16(0x0001),
		.iManufacturer		= 1,
		.iProduct		= 2,
		.iSerialNumber		= 0,
		.bNumConfigurations	= 1
	},
	.config = {
		.bLength		= USB_DT_CONFIG_SIZE,
		.bDescriptorType	= USB_DT_CONFIG,
		.wTotalLength		= __constant_cpu_to_le16(USB_DT_CONFIG_SIZE +
					 USB_DT_INTERFACE_SIZE + USB_DT_ENDPOINT_SIZE),
		.bNumInterfaces		= 1,
		.bConfigurationValue	= 1,
		.iConfiguration		= 0,
		.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
		.bMaxPower		= 0
	},
	.interface = {
		.bLength		= USB_DT_INTERFACE_SIZE,
		.bDescriptorType	= USB_DT_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 1,
		.bInterfaceClass	= USB_CLASS_HUB,
		.bInterfaceSubClass	= 0,
		.bInterfaceProtocol	= 0,	/* bInterfaceProtocol: UIPROTO_HSHUBSTT */
		.iInterface		= 0
	},
	.endpoint = {
		.bLength		= USB_DT_ENDPOINT_SIZE,
		.bDescriptorType	= USB_DT_ENDPOINT,
		.bEndpointAddress	= 0x81,	/* UE_DIR_IN | EHCI_INTR_ENDPT */
		.bmAttributes		= USB_ENDPOINT_XFER_INT,
		.wMaxPacketSize		= __constant_cpu_to_le16((USB_MAXCHILDREN + 1 + 7) / 8),
		.bInterval		= 255
	},
};

#define ehci_is_TDI()	(ehci->flags & EHCI_HAS_TT)

static int handshake(uint32_t *ptr, uint32_t mask, uint32_t done, int usec)
{
	uint32_t result;
	uint64_t start;

	start = get_time_ns();

	while (1) {
		result = ehci_readl(ptr);
		if (result == ~(uint32_t)0)
			return -1;
		result &= mask;
		if (result == done)
			return 0;
		if (is_timeout_non_interruptible(start, usec * USECOND))
			return -ETIMEDOUT;
	}
}

static int ehci_reset(struct ehci_host *ehci)
{
	uint32_t cmd;
	uint32_t tmp;
	uint32_t *reg_ptr;
	int ret = 0;

	cmd = ehci_readl(&ehci->hcor->or_usbcmd);
	cmd |= CMD_RESET;
	ehci_writel(&ehci->hcor->or_usbcmd, cmd);
	ret = handshake(&ehci->hcor->or_usbcmd, CMD_RESET, 0, 250 * 1000);
	if (ret < 0) {
		dev_err(ehci->dev, "fail to reset\n");
		goto out;
	}

	if (ehci_is_TDI()) {
		reg_ptr = (uint32_t *)((u8 *)ehci->hcor + USBMODE);
		tmp = ehci_readl(reg_ptr);
		tmp |= USBMODE_CM_HC;
#if defined(CONFIG_EHCI_MMIO_BIG_ENDIAN)
		tmp |= USBMODE_BE;
#endif
		ehci_writel(reg_ptr, tmp);
	}
out:
	return ret;
}

static int ehci_td_buffer(struct qTD *td, void *buf, size_t sz)
{
	uint32_t addr, delta, next;
	int idx;

	addr = (uint32_t) buf;
	td->qtd_dma = addr;
	td->length = sz;

	idx = 0;
	while (idx < 5) {
		td->qt_buffer[idx] = cpu_to_hc32(addr);
		next = (addr + 4096) & ~4095;
		delta = next - addr;
		if (delta >= sz)
			break;
		sz -= delta;
		addr = next;
		idx++;
	}

	if (idx == 5) {
		pr_debug("out of buffer pointers (%u bytes left)\n", sz);
		return -1;
	}

	return 0;
}

static int
ehci_submit_async(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *req, int timeout_ms)
{
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);
	struct QH *qh;
	struct qTD *td;
	volatile struct qTD *vtd;
	uint32_t *tdp;
	uint32_t endpt, token, usbsts;
	uint32_t c, toggle;
	uint32_t cmd;
	int ret = 0, i;
	uint64_t start, timeout_val;

	dev_dbg(ehci->dev, "pipe=%lx, buffer=%p, length=%d, req=%p\n", pipe,
	      buffer, length, req);
	if (req != NULL)
		dev_dbg(ehci->dev, "(req=%u (%#x), type=%u (%#x), value=%u (%#x), index=%u\n",
		      req->request, req->request,
		      req->requesttype, req->requesttype,
		      le16_to_cpu(req->value), le16_to_cpu(req->value),
		      le16_to_cpu(req->index));

	memset(&ehci->qh_list[1], 0, sizeof(struct QH));
	memset(ehci->td, 0, sizeof(struct qTD) * NUM_TD);

	qh = &ehci->qh_list[1];
	qh->qh_link = cpu_to_hc32((uint32_t)ehci->qh_list | QH_LINK_TYPE_QH);
	c = (dev->speed != USB_SPEED_HIGH &&
	     usb_pipeendpoint(pipe) == 0) ? 1 : 0;
	endpt = (8 << 28) |
	    (c << 27) |
	    (usb_maxpacket(dev, pipe) << 16) |
	    (0 << 15) |
	    (1 << 14) |
	    (usb_pipeendpoint(pipe) << 8) |
	    (0 << 7) | (usb_pipedevice(pipe) << 0);
	switch (dev->speed) {
	case USB_SPEED_FULL:
		endpt |= 0 << 12;
		break;
	case USB_SPEED_LOW:
		endpt |= 1 << 12;
		break;
	case USB_SPEED_HIGH:
		endpt |= 2 << 12;
		break;
	default:
		return -EINVAL;
	}

	qh->qh_endpt1 = cpu_to_hc32(endpt);
	endpt = (1 << 30) |
	    (dev->portnr << 23) |
	    (dev->parent->devnum << 16) | (0 << 8) | (0 << 0);
	qh->qh_endpt2 = cpu_to_hc32(endpt);
	qh->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
	qh->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);

	td = NULL;
	tdp = &qh->qt_next;

	toggle =
	    usb_gettoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

	if (req != NULL) {
		td = &ehci->td[0];

		td->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
		td->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
		token = (0 << 31) |
		    (sizeof(*req) << 16) |
		    (0 << 15) | (0 << 12) | (3 << 10) | (2 << 8) | (0x80 << 0);
		td->qt_token = cpu_to_hc32(token);
		if (ehci_td_buffer(td, req, sizeof(*req)) != 0) {
			dev_dbg(ehci->dev, "unable construct SETUP td\n");
			goto fail;
		}
		*tdp = cpu_to_hc32((uint32_t) td);
		tdp = &td->qt_next;

		toggle = 1;
	}

	if (length > 0 || req == NULL) {
		td = &ehci->td[1];

		td->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
		td->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
		token = (toggle << 31) |
		    (length << 16) |
		    ((req == NULL ? 1 : 0) << 15) |
		    (0 << 12) |
		    (3 << 10) |
		    ((usb_pipein(pipe) ? 1 : 0) << 8) | (0x80 << 0);
		td->qt_token = cpu_to_hc32(token);
		if (ehci_td_buffer(td, buffer, length) != 0) {
			dev_err(ehci->dev, "unable construct DATA td\n");
			goto fail;
		}
		*tdp = cpu_to_hc32((uint32_t) td);
		tdp = &td->qt_next;
	}

	if (req) {
		td = &ehci->td[2];

		td->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
		td->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
		token = (toggle << 31) |
		    (0 << 16) |
		    (1 << 15) |
		    (0 << 12) |
		    (3 << 10) |
		    ((usb_pipein(pipe) ? 0 : 1) << 8) | (0x80 << 0);
		td->qt_token = cpu_to_hc32(token);
		*tdp = cpu_to_hc32((uint32_t)td);
		tdp = &td->qt_next;
	}

	ehci->qh_list->qh_link = cpu_to_hc32((uint32_t) qh | QH_LINK_TYPE_QH);

	/* Flush dcache */
	if (IS_ENABLED(CONFIG_MMU)) {
		for (i = 0; i < NUM_TD; i ++) {
			struct qTD *qtd = &ehci->td[i];
			if (!qtd->qtd_dma)
				continue;
			dma_sync_single_for_device((unsigned long)qtd->qtd_dma,
						   qtd->length,
						   DMA_BIDIRECTIONAL);
		}
	}

	usbsts = ehci_readl(&ehci->hcor->or_usbsts);
	ehci_writel(&ehci->hcor->or_usbsts, (usbsts & 0x3f));

	/* Enable async. schedule. */
	cmd = ehci_readl(&ehci->hcor->or_usbcmd);
	cmd |= CMD_ASE;
	ehci_writel(&ehci->hcor->or_usbcmd, cmd);

	ret = handshake(&ehci->hcor->or_usbsts, STD_ASS, STD_ASS, 100 * 1000);
	if (ret < 0) {
		dev_err(ehci->dev, "fail timeout STD_ASS set\n");
		goto fail;
	}

	/* Wait for TDs to be processed. */
	timeout_val = timeout_ms * MSECOND;
	start = get_time_ns();
	vtd = td;
	do {
		token = hc32_to_cpu(vtd->qt_token);
		if (is_timeout_non_interruptible(start, timeout_val)) {
			/* Disable async schedule. */
			cmd = ehci_readl(&ehci->hcor->or_usbcmd);
			cmd &= ~CMD_ASE;
			ehci_writel(&ehci->hcor->or_usbcmd, cmd);

			ret = handshake(&ehci->hcor->or_usbsts, STD_ASS, 0, 100 * 1000);
			ehci_writel(&qh->qt_token, 0);
			return -ETIMEDOUT;
		}
	} while (token & 0x80);

	if (IS_ENABLED(CONFIG_MMU)) {
		for (i = 0; i < NUM_TD; i ++) {
			struct qTD *qtd = &ehci->td[i];
			if (!qtd->qtd_dma)
				continue;
			dma_sync_single_for_cpu((unsigned long)qtd->qtd_dma,
						qtd->length, DMA_BIDIRECTIONAL);
		}
	}

	/* Disable async schedule. */
	cmd = ehci_readl(&ehci->hcor->or_usbcmd);
	cmd &= ~CMD_ASE;
	ehci_writel(&ehci->hcor->or_usbcmd, cmd);

	ret = handshake(&ehci->hcor->or_usbsts, STD_ASS, 0,
			100 * 1000);
	if (ret < 0) {
		dev_err(ehci->dev, "fail timeout STD_ASS reset\n");
		goto fail;
	}

	ehci->qh_list->qh_link = cpu_to_hc32((uint32_t)ehci->qh_list | QH_LINK_TYPE_QH);

	token = hc32_to_cpu(qh->qt_token);
	if (!(token & 0x80)) {
		dev_dbg(ehci->dev, "TOKEN=0x%08x\n", token);
		switch (token & 0xfc) {
		case 0:
			toggle = token >> 31;
			usb_settoggle(dev, usb_pipeendpoint(pipe),
				       usb_pipeout(pipe), toggle);
			dev->status = 0;
			break;
		case 0x40:
			dev->status = USB_ST_STALLED;
			break;
		case 0xa0:
		case 0x20:
			dev->status = USB_ST_BUF_ERR;
			break;
		case 0x50:
		case 0x10:
			dev->status = USB_ST_BABBLE_DET;
			break;
		default:
			dev->status = USB_ST_CRC_ERR;
			if ((token & 0x40) == 0x40)
				dev->status |= USB_ST_STALLED;
			break;
		}
		dev->act_len = length - ((token >> 16) & 0x7fff);
	} else {
		dev->act_len = 0;
		dev_dbg(ehci->dev, "dev=%u, usbsts=%#x, p[1]=%#x, p[2]=%#x\n",
		      dev->devnum, ehci_readl(&ehci->hcor->or_usbsts),
		      ehci_readl(&ehci->hcor->or_portsc[0]),
		      ehci_readl(&ehci->hcor->or_portsc[1]));
	}

	return (dev->status != USB_ST_NOT_PROC) ? 0 : -1;

fail:
	dev_err(ehci->dev, "fail1\n");
	td = (void *)hc32_to_cpu(qh->qt_next);
	while (td != (void *)QT_NEXT_TERMINATE) {
		qh->qt_next = td->qt_next;
		td = (void *)hc32_to_cpu(qh->qt_next);
	}
	return -1;
}

#if defined(CONFIG_MACH_EFIKA_MX_SMARTBOOK) && defined(CONFIG_USB_ULPI)
#include <usb/ulpi.h>
/*
 * Add support for setting CHRGVBUS to workaround a hardware bug on efika mx/sb
 * boards.
 * See http://lists.infradead.org/pipermail/linux-arm-kernel/2011-January/037341.html
 */
static void ehci_powerup_fixup(struct ehci_host *ehci)
{
	void *viewport = (void *)ehci->hcor + 0x30;

	if (!of_machine_is_compatible("genesi,imx51-sb"))
		return;

	ulpi_write(ULPI_OTG_CHRG_VBUS, ULPI_OTGCTL + ULPI_REG_SET,
			viewport);
}
#else
static inline void ehci_powerup_fixup(struct ehci_host *ehci)
{
}
#endif

static void pass_to_companion(struct ehci_host *ehci, int port)
{
	uint32_t *status_reg = (uint32_t *)&ehci->hcor->or_portsc[port - 1];
	uint32_t reg = ehci_readl(status_reg);

	reg &= ~EHCI_PS_CLEAR;
	dev_dbg(ehci->dev, "port %d --> companion\n",
	      port - 1);
	reg |= EHCI_PS_PO;
	ehci_writel(status_reg, reg);
}

static int
ehci_submit_root(struct usb_device *dev, unsigned long pipe, void *buffer,
		 int length, struct devrequest *req)
{
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);
	uint8_t tmpbuf[4];
	u16 typeReq;
	void *srcptr = NULL;
	int len, srclen;
	uint32_t reg;
	uint32_t *status_reg;
	int port = le16_to_cpu(req->index);

	srclen = 0;

	dev_dbg(ehci->dev, "req=%u (%#x), type=%u (%#x), value=%u, index=%u\n",
	      req->request, req->request,
	      req->requesttype, req->requesttype,
	      le16_to_cpu(req->value), le16_to_cpu(req->index));

	typeReq = req->request | (req->requesttype << 8);

	switch (typeReq) {
	case USB_REQ_GET_STATUS | ((USB_RT_PORT | USB_DIR_IN) << 8):
	case USB_REQ_SET_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
	case USB_REQ_CLEAR_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		if (!port || port > CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS) {
			printf("The request port(%d) is not configured\n", port - 1);
			return -1;
		}
		status_reg = (uint32_t *)&ehci->hcor->or_portsc[port - 1];
		if (ehci_readl(status_reg) & EHCI_PS_PO) {
			dev_dbg(ehci->dev, "Port %d is owned by companion controller\n", port);
			return -1;
		}
		break;
	default:
		status_reg = NULL;
		break;
	}

	switch (typeReq) {
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_DEVICE:
			dev_dbg(ehci->dev, "USB_DT_DEVICE request\n");
			srcptr = &descriptor.device;
			srclen = descriptor.device.bLength;
			break;
		case USB_DT_CONFIG:
			dev_dbg(ehci->dev, "USB_DT_CONFIG config\n");
			srcptr = &descriptor.config;
			srclen = le16_to_cpu(descriptor.config.wTotalLength);
			break;
		case USB_DT_STRING:
			dev_dbg(ehci->dev, "USB_DT_STRING config\n");
			switch (le16_to_cpu(req->value) & 0xff) {
			case 0:	/* Language */
				srcptr = "\4\3\1\0";
				srclen = 4;
				break;
			case 1:	/* Vendor */
				srcptr = "\16\3u\0-\0b\0o\0o\0t\0";
				srclen = 14;
				break;
			case 2:	/* Product */
				srcptr = "\52\3E\0H\0C\0I\0 "
					 "\0H\0o\0s\0t\0 "
					 "\0C\0o\0n\0t\0r\0o\0l\0l\0e\0r\0";
				srclen = 42;
				break;
			default:
				dev_dbg(ehci->dev, "unknown value DT_STRING %x\n",
					le16_to_cpu(req->value));
				goto unknown;
			}
			break;
		default:
			dev_dbg(ehci->dev, "unknown value %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		break;
	case ((USB_DIR_IN | USB_RT_HUB) << 8) | USB_REQ_GET_DESCRIPTOR:
		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_HUB:
			dev_dbg(ehci->dev, "USB_DT_HUB config\n");
			srcptr = &descriptor.hub;
			srclen = descriptor.hub.bLength;
			break;
		default:
			dev_dbg(ehci->dev, "unknown value %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		break;
	case USB_REQ_SET_ADDRESS | (USB_RECIP_DEVICE << 8):
		dev_dbg(ehci->dev, "USB_REQ_SET_ADDRESS\n");
		ehci->rootdev = le16_to_cpu(req->value);
		break;
	case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
		dev_dbg(ehci->dev, "USB_REQ_SET_CONFIGURATION\n");
		/* Nothing to do */
		break;
	case USB_REQ_GET_STATUS | ((USB_DIR_IN | USB_RT_HUB) << 8):
		tmpbuf[0] = 1;	/* USB_STATUS_SELFPOWERED */
		tmpbuf[1] = 0;
		srcptr = tmpbuf;
		srclen = 2;
		break;
	case USB_REQ_GET_STATUS | ((USB_RT_PORT | USB_DIR_IN) << 8):
		memset(tmpbuf, 0, 4);
		reg = ehci_readl(status_reg);
		if (reg & EHCI_PS_CS)
			tmpbuf[0] |= USB_PORT_STAT_CONNECTION;
		if (reg & EHCI_PS_PE)
			tmpbuf[0] |= USB_PORT_STAT_ENABLE;
		if (reg & EHCI_PS_SUSP)
			tmpbuf[0] |= USB_PORT_STAT_SUSPEND;
		if (reg & EHCI_PS_OCA)
			tmpbuf[0] |= USB_PORT_STAT_OVERCURRENT;
		if (reg & EHCI_PS_PR &&
		    (ehci->portreset & (1 << port))) {
			int ret;
			/* force reset to complete */
			reg = reg & ~(EHCI_PS_PR | EHCI_PS_CLEAR);
			ehci_writel(status_reg, reg);
			ret = handshake(status_reg, EHCI_PS_PR, 0, 2 * 1000);
			if (!ret)
				tmpbuf[0] |= USB_PORT_STAT_RESET;
			else
				dev_err(ehci->dev, "port(%d) reset error\n",
					port - 1);
		}
		if (reg & EHCI_PS_PP)
			tmpbuf[1] |= USB_PORT_STAT_POWER >> 8;

		if (ehci_is_TDI()) {
			switch ((reg >> 26) & 3) {
			case 0:
				break;
			case 1:
				tmpbuf[1] |= USB_PORT_STAT_LOW_SPEED >> 8;
				break;
			case 2:
			default:
				tmpbuf[1] |= USB_PORT_STAT_HIGH_SPEED >> 8;
				break;
			}
		} else {
			tmpbuf[1] |= USB_PORT_STAT_HIGH_SPEED >> 8;
		}

		if (reg & EHCI_PS_CSC)
			tmpbuf[2] |= USB_PORT_STAT_C_CONNECTION;
		if (reg & EHCI_PS_PEC)
			tmpbuf[2] |= USB_PORT_STAT_C_ENABLE;
		if (reg & EHCI_PS_OCC)
			tmpbuf[2] |= USB_PORT_STAT_C_OVERCURRENT;
		if (ehci->portreset & (1 << port))
			tmpbuf[2] |= USB_PORT_STAT_C_RESET;

		srcptr = tmpbuf;
		srclen = 4;
		break;
	case USB_REQ_SET_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		reg = ehci_readl(status_reg);
		reg &= ~EHCI_PS_CLEAR;
		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
			reg |= EHCI_PS_PE;
			ehci_writel(status_reg, reg);
			break;
		case USB_PORT_FEAT_POWER:
			if (HCS_PPC(ehci_readl(&ehci->hccr->cr_hcsparams))) {
				reg |= EHCI_PS_PP;
				ehci_writel(status_reg, reg);
			}
			break;
		case USB_PORT_FEAT_RESET:
			if ((reg & (EHCI_PS_PE | EHCI_PS_CS)) == EHCI_PS_CS &&
			    !ehci_is_TDI() &&
			    EHCI_PS_IS_LOWSPEED(reg)) {
				pass_to_companion(ehci, port);
				break;
			} else {
				int ret;

				reg |= EHCI_PS_PR;
				reg &= ~EHCI_PS_PE;
				ehci_writel(status_reg, reg);
				/*
				 * caller must wait, then call GetPortStatus
				 * usb 2.0 specification say 50 ms resets on
				 * root
				 */
				ehci_powerup_fixup(ehci);
				mdelay(50);
				ehci->portreset |= 1 << port;
				/* terminate the reset */
				ehci_writel(status_reg, reg & ~EHCI_PS_PR);
				/*
				 * A host controller must terminate the reset
				 * and stabilize the state of the port within
				 * 2 milliseconds
				 */
				ret = handshake(status_reg, EHCI_PS_PR, 0,
						2 * 1000);
				if (!ret)
					ehci->portreset |=
						1 << port;
				else
					dev_err(ehci->dev, "port(%d) reset error\n",
						port - 1);
				mdelay(200);
				reg = ehci_readl(status_reg);
				if (!(reg & EHCI_PS_PE))
					pass_to_companion(ehci, port);
			}
			break;
		default:
			dev_dbg(ehci->dev, "unknown feature %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		/* unblock posted writes */
		(void) ehci_readl(&ehci->hcor->or_usbcmd);
		break;
	case USB_REQ_CLEAR_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		reg = ehci_readl(status_reg);
		reg &= ~EHCI_PS_CLEAR;
		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
			reg &= ~EHCI_PS_PE;
			break;
		case USB_PORT_FEAT_C_ENABLE:
			reg |= EHCI_PS_PEC;
			break;
		case USB_PORT_FEAT_POWER:
			if (HCS_PPC(ehci_readl(&ehci->hccr->cr_hcsparams)))
				reg &= ~ EHCI_PS_PP;
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			reg |= EHCI_PS_CSC;
			break;
		case USB_PORT_FEAT_OVER_CURRENT:
			reg |= EHCI_PS_OCC;
			break;
		case USB_PORT_FEAT_C_RESET:
			ehci->portreset &= ~(1 << port);
			break;
		default:
			dev_dbg(ehci->dev, "unknown feature %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		ehci_writel(status_reg, reg);
		/* unblock posted write */
		(void) ehci_readl(&ehci->hcor->or_usbcmd);
		break;
	default:
		dev_dbg(ehci->dev, "Unknown request\n");
		goto unknown;
	}

	mdelay(1);
	len = min3(srclen, (int)le16_to_cpu(req->length), length);
	if (srcptr != NULL && len > 0)
		memcpy(buffer, srcptr, len);
	else
		dev_dbg(ehci->dev, "Len is 0\n");

	dev->act_len = len;
	dev->status = 0;
	return 0;

unknown:
	dev_dbg(ehci->dev, "requesttype=%x, request=%x, value=%x, index=%x, length=%x\n",
	      req->requesttype, req->request, le16_to_cpu(req->value),
	      le16_to_cpu(req->index), le16_to_cpu(req->length));

	dev->act_len = 0;
	dev->status = USB_ST_STALLED;
	return -1;
}

/* force HC to halt state from unknown (EHCI spec section 2.3) */
static int ehci_halt(struct ehci_host *ehci)
{
	u32	temp = ehci_readl(&ehci->hcor->or_usbsts);

	/* disable any irqs left enabled by previous code */
	ehci_writel(&ehci->hcor->or_usbintr, 0);

	if (temp & STS_HALT)
		return 0;

	temp = ehci_readl(&ehci->hcor->or_usbcmd);
	temp &= ~CMD_RUN;
	ehci_writel(&ehci->hcor->or_usbcmd, temp);

	return handshake(&ehci->hcor->or_usbsts,
			  STS_HALT, STS_HALT, 16 * 125);
}

static int ehci_init(struct usb_host *host)
{
	struct ehci_host *ehci = to_ehci(host);
	uint32_t reg;
	uint32_t cmd;
	int ret = 0;
	struct QH *periodic;
	int i;

	ehci_halt(ehci);

	/* EHCI spec section 4.1 */
	if (ehci_reset(ehci) != 0)
		return -1;

	if (ehci->init) {
		ret = ehci->init(ehci->drvdata);
		if (ret)
			return ret;
	}

	memset(ehci->qh_list, 0, sizeof(struct QH) * NUM_TD);

	ehci->qh_list->qh_link = cpu_to_hc32((uint32_t)ehci->qh_list | QH_LINK_TYPE_QH);
	ehci->qh_list->qh_endpt1 = cpu_to_hc32((1 << 15) | (USB_SPEED_HIGH << 12));
	ehci->qh_list->qh_curtd = cpu_to_hc32(QT_NEXT_TERMINATE);
	ehci->qh_list->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
	ehci->qh_list->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
	ehci->qh_list->qt_token = cpu_to_hc32(0x40);

	/* Set async. queue head pointer. */
	ehci_writel(&ehci->hcor->or_asynclistaddr, (uint32_t)ehci->qh_list);

	/*
	 * Set up periodic list
	 * Step 1: Parent QH for all periodic transfers.
	 */
	ehci->periodic_schedules = 0;
	periodic = ehci->periodic_queue;
	memset(periodic, 0, sizeof(*periodic));
	periodic->qh_link = cpu_to_hc32(QH_LINK_TERMINATE);
	periodic->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
	periodic->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);

	/*
	 * Step 2: Setup frame-list: Every microframe, USB tries the same list.
	 *         In particular, device specifications on polling frequency
	 *         are disregarded. Keyboards seem to send NAK/NYet reliably
	 *         when polled with an empty buffer.
	 *
	 *         Split Transactions will be spread across microframes using
	 *         S-mask and C-mask.
	 */
	if (ehci->periodic_list == NULL)
		/*
		 * FIXME: this memory chunk have to be 4k aligned AND
		 * reside in coherent memory. Current implementation of
		 * dma_alloc_coherent() allocates PAGE_SIZE aligned memory chunks.
		 * PAGE_SIZE less then 4k will break this code.
		 */
		ehci->periodic_list = dma_alloc_coherent(1024 * 4,
							 DMA_ADDRESS_BROKEN);
	for (i = 0; i < 1024; i++) {
		ehci->periodic_list[i] = cpu_to_hc32((unsigned long)periodic
						| QH_LINK_TYPE_QH);
	}

	/* Set periodic list base address */
	ehci_writel(&ehci->hcor->or_periodiclistbase,
		(unsigned long)ehci->periodic_list);

	reg = ehci_readl(&ehci->hccr->cr_hcsparams);
	descriptor.hub.bNbrPorts = HCS_N_PORTS(reg);

	/* Port Indicators */
	if (HCS_INDICATOR(reg))
		descriptor.hub.wHubCharacteristics |= 0x80;
	/* Port Power Control */
	if (HCS_PPC(reg))
		descriptor.hub.wHubCharacteristics |= 0x01;

	/* Start the host controller. */
	cmd = ehci_readl(&ehci->hcor->or_usbcmd);
	/*
	 * Philips, Intel, and maybe others need CMD_RUN before the
	 * root hub will detect new devices (why?); NEC doesn't
	 */
	cmd &= ~(CMD_LRESET|CMD_IAAD|CMD_PSE|CMD_ASE|CMD_RESET);
	cmd |= CMD_RUN;
	ehci_writel(&ehci->hcor->or_usbcmd, cmd);

	/* take control over the ports */
	cmd = ehci_readl(&ehci->hcor->or_configflag);
	cmd |= FLAG_CF;
	ehci_writel(&ehci->hcor->or_configflag, cmd);
	/* unblock posted write */
	cmd = ehci_readl(&ehci->hcor->or_usbcmd);
	mdelay(5);

	ehci->rootdev = 0;

	if (ehci->post_init)
		ret = ehci->post_init(ehci->drvdata);

	return ret;
}

static int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int length, int timeout)
{
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);

	if (usb_pipetype(pipe) != PIPE_BULK) {
		dev_dbg(ehci->dev, "non-bulk pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}
	return ehci_submit_async(dev, pipe, buffer, length, NULL, timeout);
}

static int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *setup, int timeout)
{
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);

	if (usb_pipetype(pipe) != PIPE_CONTROL) {
		dev_dbg(ehci->dev, "non-control pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}

	if (usb_pipedevice(pipe) == ehci->rootdev) {
		if (ehci->rootdev == 0)
			dev->speed = USB_SPEED_HIGH;
		return ehci_submit_root(dev, pipe, buffer, length, setup);
	}
	return ehci_submit_async(dev, pipe, buffer, length, setup, timeout);
}

static int
disable_periodic(struct ehci_host *ehci)
{
	uint32_t cmd;
	struct ehci_hcor *hcor = ehci->hcor;
	int ret;

	cmd = ehci_readl(&hcor->or_usbcmd);
	cmd &= ~CMD_PSE;
	ehci_writel(&hcor->or_usbcmd, cmd);

	ret = handshake((uint32_t *)&hcor->or_usbsts,
			STS_PSS, 0, 100 * 1000);
	if (ret < 0) {
		printf("EHCI failed: timeout when disabling periodic list\n");
		return -ETIMEDOUT;
	}
	return 0;
}

#define NEXT_QH(qh) (struct QH *)((unsigned long)hc32_to_cpu((qh)->qh_link) & ~0x1f)

static int
enable_periodic(struct ehci_host *ehci)
{
	uint32_t cmd;
	struct ehci_hcor *hcor = ehci->hcor;
	int ret;

	cmd = ehci_readl(&hcor->or_usbcmd);
	cmd |= CMD_PSE;
	ehci_writel(&hcor->or_usbcmd, cmd);
	ret = handshake((uint32_t *)&hcor->or_usbsts,
			STS_PSS, STS_PSS, 100 * 1000);
	if (ret < 0) {
		printf("EHCI failed: timeout when enabling periodic list\n");
		return -ETIMEDOUT;
	}

	mdelay(1);

	return 0;
}

static inline u8 ehci_encode_speed(enum usb_device_speed speed)
{
	#define QH_HIGH_SPEED	2
	#define QH_FULL_SPEED	0
	#define QH_LOW_SPEED	1
	if (speed == USB_SPEED_HIGH)
		return QH_HIGH_SPEED;
	if (speed == USB_SPEED_LOW)
		return QH_LOW_SPEED;
	return QH_FULL_SPEED;
}

static void ehci_update_endpt2_dev_n_port(struct usb_device *udev,
					  struct QH *qh)
{
	struct usb_device *ttdev;
	int parent_devnum;

	if (udev->speed != USB_SPEED_LOW && udev->speed != USB_SPEED_FULL)
		return;

	/*
	 * For full / low speed devices we need to get the devnum and portnr of
	 * the tt, so of the first upstream usb-2 hub, there may be usb-1 hubs
	 * in the tree before that one!
	 */

	ttdev = udev;
	while (ttdev->parent && ttdev->parent->speed != USB_SPEED_HIGH)
		ttdev = ttdev->parent;
	if (!ttdev->parent)
		return;
	parent_devnum = ttdev->parent->devnum;

	qh->qh_endpt2 |= cpu_to_hc32(QH_ENDPT2_PORTNUM(ttdev->portnr) |
				     QH_ENDPT2_HUBADDR(parent_devnum));
}

static struct int_queue *ehci_create_int_queue(struct usb_device *dev,
			unsigned long pipe, int queuesize, int elementsize,
			void *buffer, int interval)
{
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);
	struct int_queue *result = NULL;
	uint32_t i;
	struct QH *list = ehci->periodic_queue;

	/*
	 * Interrupt transfers requiring several transactions are not supported
	 * because bInterval is ignored.
	 *
	 * Also, ehci_submit_async() relies on wMaxPacketSize being a power of 2
	 * <= PKT_ALIGN if several qTDs are required, while the USB
	 * specification does not constrain this for interrupt transfers. That
	 * means that ehci_submit_async() would support interrupt transfers
	 * requiring several transactions only as long as the transfer size does
	 * not require more than a single qTD.
	 */
	if (elementsize > usb_maxpacket(dev, pipe)) {
		dev_err(&dev->dev,
			"%s: xfers requiring several transactions are not supported.\n",
			__func__);
		return NULL;
	}

	debug("Enter create_int_queue\n");
	if (usb_pipetype(pipe) != PIPE_INTERRUPT) {
		dev_dbg(&dev->dev,
			"non-interrupt pipe (type=%lu)",
			usb_pipetype(pipe));
		return NULL;
	}

	/* limit to 4 full pages worth of data -
	 * we can safely fit them in a single TD,
	 * no matter the alignment
	 */
	if (elementsize >= 16384) {
		dev_dbg(&dev->dev,
			"too large elements for interrupt transfers\n");
		return NULL;
	}

	result = xzalloc(sizeof(*result));
	result->elementsize = elementsize;
	result->queuesize = queuesize;
	result->pipe = pipe;
	result->first = dma_alloc_coherent(sizeof(struct QH) * queuesize,
					   DMA_ADDRESS_BROKEN);
	result->current = result->first;
	result->last = result->first + queuesize - 1;
	result->tds = dma_alloc_coherent(sizeof(struct qTD) * queuesize,
					 DMA_ADDRESS_BROKEN);
	memset(result->first, 0, sizeof(struct QH) * queuesize);
	memset(result->tds, 0, sizeof(struct qTD) * queuesize);

	for (i = 0; i < queuesize; i++) {
		struct QH *qh = result->first + i;
		struct qTD *td = result->tds + i;
		void **buf = &qh->buffer;

		qh->qh_link = cpu_to_hc32((unsigned long)(qh+1) | QH_LINK_TYPE_QH);
		if (i == queuesize - 1)
			qh->qh_link = cpu_to_hc32(QH_LINK_TERMINATE);

		qh->qt_next = cpu_to_hc32((unsigned long)td);
		qh->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
		qh->qh_endpt1 =
			cpu_to_hc32((0 << 28) | /* No NAK reload (ehci 4.9) */
			(usb_maxpacket(dev, pipe) << 16) | /* MPS */
			QH_ENDPT1_EPS(ehci_encode_speed(dev->speed)) |
			(usb_pipeendpoint(pipe) << 8) | /* Endpoint Number */
			(usb_pipedevice(pipe) << 0));
		qh->qh_endpt2 = cpu_to_hc32((1 << 30) | /* 1 Tx per mframe */
			(1 << 0)); /* S-mask: microframe 0 */
		if (dev->speed == USB_SPEED_LOW ||
				dev->speed == USB_SPEED_FULL) {
			/* C-mask: microframes 2-4 */
			qh->qh_endpt2 |= cpu_to_hc32((0x1c << 8));
		}
		ehci_update_endpt2_dev_n_port(dev, qh);

		td->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
		td->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
		dev_dbg(&dev->dev,
			"communication direction is '%s'\n",
			usb_pipein(pipe) ? "in" : "out");
		td->qt_token = cpu_to_hc32(
			(elementsize << 16) |
			((usb_pipein(pipe) ? 1 : 0) << 8) | /* IN/OUT token */
			0x80); /* active */
		td->qt_buffer[0] =
		    cpu_to_hc32((unsigned long)buffer + i * elementsize);
		td->qt_buffer[1] =
		    cpu_to_hc32((td->qt_buffer[0] + 0x1000) & ~0xfff);
		td->qt_buffer[2] =
		    cpu_to_hc32((td->qt_buffer[0] + 0x2000) & ~0xfff);
		td->qt_buffer[3] =
		    cpu_to_hc32((td->qt_buffer[0] + 0x3000) & ~0xfff);
		td->qt_buffer[4] =
		    cpu_to_hc32((td->qt_buffer[0] + 0x4000) & ~0xfff);

		*buf = buffer + i * elementsize;
	}

	if (ehci->periodic_schedules > 0) {
		if (disable_periodic(ehci) < 0) {
			dev_err(&dev->dev,
				"FATAL: periodic should never fail, but did");
			goto fail3;
		}
	}

	/* hook up to periodic list */
	result->last->qh_link = list->qh_link;
	list->qh_link = cpu_to_hc32((unsigned long)result->first | QH_LINK_TYPE_QH);

	if (enable_periodic(ehci) < 0) {
		dev_err(&dev->dev,
			"FATAL: periodic should never fail, but did");
		goto fail3;
	}
	ehci->periodic_schedules++;

	dev_dbg(&dev->dev, "Exit create_int_queue\n");
	return result;
fail3:
	dma_free_coherent(result->tds, 0, sizeof(struct qTD) * queuesize);
	dma_free_coherent(result->first, 0, sizeof(struct QH) * queuesize);
	free(result);
	return NULL;
}

static void *ehci_poll_int_queue(struct usb_device *dev,
				  struct int_queue *queue)
{
	struct QH *cur = queue->current;
	struct qTD *cur_td;
	uint32_t token;

	/* depleted queue */
	if (cur == NULL) {
		dev_dbg(&dev->dev, "Exit poll_int_queue with completed queue\n");
		return NULL;
	}
	/* still active */
	cur_td = &queue->tds[queue->current - queue->first];
	token = hc32_to_cpu(cur_td->qt_token);
	if (QT_TOKEN_GET_STATUS(token) & QT_TOKEN_STATUS_ACTIVE) {
		dev_dbg(&dev->dev,
			"Exit poll_int_queue with no completed intr transfer. token is %x\n",
			token);
		return NULL;
	}

	if (!(cur->qh_link & QH_LINK_TERMINATE))
		queue->current++;
	else
		queue->current = NULL;

	dev_dbg(&dev->dev,
		"Exit poll_int_queue with completed intr transfer. token is %x at %p (first at %p)\n",
		token, cur, queue->first);
	return cur->buffer;
}

static int ehci_destroy_int_queue(struct usb_device *dev,
				   struct int_queue *queue)
{
	int result = -EINVAL;
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);
	struct QH *cur = ehci->periodic_queue;

	if (disable_periodic(ehci) < 0) {
		dev_err(&dev->dev,
			"FATAL: periodic should never fail, but did\n");
		goto out;
	}
	ehci->periodic_schedules--;

	while (!(cur->qh_link & cpu_to_hc32(QH_LINK_TERMINATE))) {
		dev_dbg(&dev->dev,
			"considering %p, with qh_link %x\n",
			cur, cur->qh_link);
		if (NEXT_QH(cur) == queue->first) {
			dev_dbg(&dev->dev,
				"found candidate. removing from chain\n");
			cur->qh_link = queue->last->qh_link;
			result = 0;
			break;
		}
		cur = NEXT_QH(cur);
	}

	if (ehci->periodic_schedules > 0) {
		result = enable_periodic(ehci);
		if (result < 0)
			dev_err(&dev->dev,
				"FATAL: periodic should never fail, but did");
	}

out:
	dma_free_coherent(queue->tds, 0, sizeof(struct qTD) * queue->queuesize);
	dma_free_coherent(queue->first, 0, sizeof(struct QH) * queue->queuesize);
	free(queue);
	return result;
}

static int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
	       int length, int interval)
{
	struct usb_host *host = dev->host;
	struct ehci_host *ehci = to_ehci(host);
	struct int_queue *queue;
	uint64_t start;
	void *backbuffer;
	int result = 0, ret;

	dev_dbg(ehci->dev, "dev=%p, pipe=%lu, buffer=%p, length=%d, interval=%d",
	      dev, pipe, buffer, length, interval);

	dma_sync_single_for_device((unsigned long)buffer, length,
				   DMA_BIDIRECTIONAL);

	queue = ehci_create_int_queue(dev, pipe, 1, length, buffer, interval);
	if (!queue)
		return -EINVAL;

	start = get_time_ns();
	while ((backbuffer = ehci_poll_int_queue(dev, queue)) == NULL)
		if (is_timeout_non_interruptible(start,
						 USB_CNTL_TIMEOUT * MSECOND)) {
			dev_err(&dev->dev,
				"Timeout poll on interrupt endpoint\n");
			result = -ETIMEDOUT;
			break;
		}

	if (backbuffer != buffer) {
		dev_err(&dev->dev,
			"got wrong buffer back (%p instead of %p)\n",
			backbuffer, buffer);
		if (!result)
			result = -EINVAL;
	}

	dma_sync_single_for_cpu((unsigned long)buffer, length,
				DMA_BIDIRECTIONAL);

	ret = ehci_destroy_int_queue(dev, queue);
	if (!result)
		result = ret;
	return result;
}

int ehci_detect(struct ehci_host *ehci)
{
	return usb_host_detect(&ehci->host);
}

struct ehci_host *ehci_register(struct device_d *dev, struct ehci_data *data)
{
	struct usb_host *host;
	struct ehci_host *ehci;
	uint32_t reg;

	ehci = xzalloc(sizeof(struct ehci_host));
	host = &ehci->host;
	ehci->flags = data->flags;
	ehci->hccr = data->hccr;
	ehci->dev = dev;

	if (data->hcor)
		ehci->hcor = data->hcor;
	else
		ehci->hcor = (void __iomem *)ehci->hccr +
			HC_LENGTH(ehci_readl(&ehci->hccr->cr_capbase));

	ehci->drvdata = data->drvdata;
	ehci->init = data->init;
	ehci->post_init = data->post_init;

	ehci->qh_list = dma_alloc_coherent(sizeof(struct QH) * NUM_TD,
					   DMA_ADDRESS_BROKEN);
	ehci->periodic_queue = dma_alloc_coherent(sizeof(struct QH),
						  DMA_ADDRESS_BROKEN);
	ehci->td = dma_alloc_coherent(sizeof(struct qTD) * NUM_TD,
				      DMA_ADDRESS_BROKEN);

	host->hw_dev = dev;
	host->init = ehci_init;
	host->usbphy = data->usbphy;
	host->submit_int_msg = submit_int_msg;
	host->submit_control_msg = submit_control_msg;
	host->submit_bulk_msg = submit_bulk_msg;

	if (ehci->flags & EHCI_HAS_TT) {
		ehci_reset(ehci);
	}

	usb_register_host(host);

	reg = HC_VERSION(ehci_readl(&ehci->hccr->cr_capbase));
	dev_info(dev, "USB EHCI %x.%02x\n", reg >> 8, reg & 0xff);

	return ehci;
}

void ehci_unregister(struct ehci_host *ehci)
{
	ehci_halt(ehci);

	usb_unregister_host(&ehci->host);

	free(ehci);
}

static int ehci_dev_detect(struct device_d *dev)
{
	struct ehci_host *ehci = dev->priv;

	return ehci_detect(ehci);
}

static int ehci_probe(struct device_d *dev)
{
	struct resource *iores;
	struct ehci_data data = {};
	struct ehci_platform_data *pdata = dev->platform_data;
	struct device_node *dn = dev->device_node;
	struct ehci_host *ehci;

	if (pdata)
		data.flags = pdata->flags;
	else if (dn) {
		data.flags = 0;
		if (of_property_read_bool(dn, "has-transaction-translator"))
			data.flags |= EHCI_HAS_TT;
	} else
		/* default to EHCI_HAS_TT to not change behaviour of boards
		 * without platform_data
		 */
		data.flags = EHCI_HAS_TT;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	data.hccr = IOMEM(iores->start);

	if (dev->num_resources > 1) {
		iores = dev_request_mem_resource(dev, 1);
		if (IS_ERR(iores))
			return PTR_ERR(iores);
		data.hcor = IOMEM(iores->start);
	}
	else
		data.hcor = NULL;

	ehci = ehci_register(dev, &data);
	if (IS_ERR(ehci))
		return PTR_ERR(ehci);

	dev->priv = ehci;
	dev->detect = ehci_dev_detect;

	return 0;
}

static void ehci_remove(struct device_d *dev)
{
	struct ehci_host *ehci = dev->priv;

	ehci_unregister(ehci);
}

static __maybe_unused struct of_device_id ehci_platform_dt_ids[] = {
	{
		.compatible = "generic-ehci",
	}, {
		/* sentinel */
	}
};

static struct driver_d ehci_driver = {
	.name  = "ehci",
	.probe = ehci_probe,
	.remove = ehci_remove,
	.of_compatible = DRV_OF_COMPAT(ehci_platform_dt_ids),
};
device_platform_driver(ehci_driver);
