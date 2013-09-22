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
#include <asm/mmu.h>

#include "ehci.h"

struct ehci_priv {
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
};

#define to_ehci(ptr) container_of(ptr, struct ehci_priv, host)

#define NUM_QH	2
#define NUM_TD	3

static struct descriptor {
	struct usb_hub_descriptor hub;
	struct usb_device_descriptor device;
	struct usb_linux_config_descriptor config;
	struct usb_linux_interface_descriptor interface;
	struct usb_endpoint_descriptor endpoint;
}  __attribute__ ((packed)) descriptor = {
	{
		0x8,		/* bDescLength */
		0x29,		/* bDescriptorType: hub descriptor */
		2,		/* bNrPorts -- runtime modified */
		0,		/* wHubCharacteristics */
		10,		/* bPwrOn2PwrGood */
		0,		/* bHubCntrCurrent */
		{},		/* Device removable */
		{}		/* at most 7 ports! XXX */
	},
	{
		0x12,		/* bLength */
		1,		/* bDescriptorType: UDESC_DEVICE */
		0x0002,		/* bcdUSB: v2.0 */
		9,		/* bDeviceClass: UDCLASS_HUB */
		0,		/* bDeviceSubClass: UDSUBCLASS_HUB */
		1,		/* bDeviceProtocol: UDPROTO_HSHUBSTT */
		64,		/* bMaxPacketSize: 64 bytes */
		0x0000,		/* idVendor */
		0x0000,		/* idProduct */
		0x0001,		/* bcdDevice */
		1,		/* iManufacturer */
		2,		/* iProduct */
		0,		/* iSerialNumber */
		1		/* bNumConfigurations: 1 */
	},
	{
		0x9,
		2,		/* bDescriptorType: UDESC_CONFIG */
		cpu_to_le16(0x19),
		1,		/* bNumInterface */
		1,		/* bConfigurationValue */
		0,		/* iConfiguration */
		0x40,		/* bmAttributes: UC_SELF_POWER */
		0		/* bMaxPower */
	},
	{
		0x9,		/* bLength */
		4,		/* bDescriptorType: UDESC_INTERFACE */
		0,		/* bInterfaceNumber */
		0,		/* bAlternateSetting */
		1,		/* bNumEndpoints */
		9,		/* bInterfaceClass: UICLASS_HUB */
		0,		/* bInterfaceSubClass: UISUBCLASS_HUB */
		0,		/* bInterfaceProtocol: UIPROTO_HSHUBSTT */
		0		/* iInterface */
	},
	{
		0x7,		/* bLength */
		5,		/* bDescriptorType: UDESC_ENDPOINT */
		0x81,		/* bEndpointAddress:
				 * UE_DIR_IN | EHCI_INTR_ENDPT
				 */
		3,		/* bmAttributes: UE_INTERRUPT */
		8, 0,		/* wMaxPacketSize */
		255		/* bInterval */
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
		if (is_timeout(start, usec * USECOND))
			return -ETIMEDOUT;
	}
}

static int ehci_reset(struct ehci_priv *ehci)
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
		   int length, struct devrequest *req)
{
	struct usb_host *host = dev->host;
	struct ehci_priv *ehci = to_ehci(host);
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
	c = (usb_pipespeed(pipe) != USB_SPEED_HIGH &&
	     usb_pipeendpoint(pipe) == 0) ? 1 : 0;
	endpt = (8 << 28) |
	    (c << 27) |
	    (usb_maxpacket(dev, pipe) << 16) |
	    (0 << 15) |
	    (1 << 14) |
	    (usb_pipespeed(pipe) << 12) |
	    (usb_pipeendpoint(pipe) << 8) |
	    (0 << 7) | (usb_pipedevice(pipe) << 0);
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
			dma_flush_range(qtd->qtd_dma, qtd->qtd_dma + qtd->length);
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
	timeout_val = usb_pipebulk(pipe) ? (SECOND << 2) : (SECOND >> 2);
	start = get_time_ns();
	vtd = td;
	do {
		token = hc32_to_cpu(vtd->qt_token);
		if (is_timeout(start, timeout_val)) {
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
			dma_inv_range(qtd->qtd_dma, qtd->qtd_dma + qtd->length);
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

static inline int min3(int a, int b, int c)
{

	if (b < a)
		a = b;
	if (c < a)
		a = c;
	return a;
}

#ifdef CONFIG_MACH_EFIKA_MX_SMARTBOOK
#include <usb/ulpi.h>
/*
 * Add support for setting CHRGVBUS to workaround a hardware bug on efika mx/sb
 * boards.
 * See http://lists.infradead.org/pipermail/linux-arm-kernel/2011-January/037341.html
 */
static void ehci_powerup_fixup(struct ehci_priv *ehci)
{
	void *viewport = (void *)ehci->hcor + 0x30;

	if (!of_machine_is_compatible("genesi,imx51-sb"))
		return;

	ulpi_write(ULPI_OTG_CHRG_VBUS, ULPI_OTGCTL + ULPI_REG_SET,
			viewport);
}
#else
static inline void ehci_powerup_fixup(struct ehci_priv *ehci)
{
}
#endif

static int
ehci_submit_root(struct usb_device *dev, unsigned long pipe, void *buffer,
		 int length, struct devrequest *req)
{
	struct usb_host *host = dev->host;
	struct ehci_priv *ehci = to_ehci(host);
	uint8_t tmpbuf[4];
	u16 typeReq;
	void *srcptr = NULL;
	int len, srclen;
	uint32_t reg;
	uint32_t *status_reg;

	if (le16_to_cpu(req->index) >= CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS) {
		dev_err(ehci->dev, "The request port(%d) is not configured\n",
			le16_to_cpu(req->index) - 1);
		return -1;
	}
	status_reg = (uint32_t *)&ehci->hcor->or_portsc[le16_to_cpu(req->index) - 1];
	srclen = 0;

	dev_dbg(ehci->dev, "req=%u (%#x), type=%u (%#x), value=%u, index=%u\n",
	      req->request, req->request,
	      req->requesttype, req->requesttype,
	      le16_to_cpu(req->value), le16_to_cpu(req->index));

	typeReq = req->request | (req->requesttype << 8);

	switch (typeReq) {
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_DEVICE:
			dev_dbg(ehci->dev, "USB_DT_DEVICE request\n");
			srcptr = &descriptor.device;
			srclen = 0x12;
			break;
		case USB_DT_CONFIG:
			dev_dbg(ehci->dev, "USB_DT_CONFIG config\n");
			srcptr = &descriptor.config;
			srclen = 0x19;
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
			srclen = 0x8;
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
		    (ehci->portreset & (1 << le16_to_cpu(req->index)))) {
			int ret;
			/* force reset to complete */
			reg = reg & ~(EHCI_PS_PR | EHCI_PS_CLEAR);
			ehci_writel(status_reg, reg);
			ret = handshake(status_reg, EHCI_PS_PR, 0, 2 * 1000);
			if (!ret)
				tmpbuf[0] |= USB_PORT_STAT_RESET;
			else
				dev_err(ehci->dev, "port(%d) reset error\n",
					le16_to_cpu(req->index) - 1);
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
		if (ehci->portreset & (1 << le16_to_cpu(req->index)))
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
				/* Low speed device, give up ownership. */
				dev_dbg(ehci->dev, "port %d low speed --> companion\n",
				      req->index - 1);
				reg |= EHCI_PS_PO;
				ehci_writel(status_reg, reg);
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
				wait_ms(50);
				ehci->portreset |= 1 << le16_to_cpu(req->index);
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
						1 << le16_to_cpu(req->index);
				else
					dev_err(ehci->dev, "port(%d) reset error\n",
						le16_to_cpu(req->index) - 1);

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
		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
			reg &= ~EHCI_PS_PE;
			break;
		case USB_PORT_FEAT_C_ENABLE:
			reg = (reg & ~EHCI_PS_CLEAR) | EHCI_PS_PE;
			break;
		case USB_PORT_FEAT_POWER:
			if (HCS_PPC(ehci_readl(&ehci->hccr->cr_hcsparams)))
				reg = reg & ~(EHCI_PS_CLEAR | EHCI_PS_PP);
		case USB_PORT_FEAT_C_CONNECTION:
			reg = (reg & ~EHCI_PS_CLEAR) | EHCI_PS_CSC;
			break;
		case USB_PORT_FEAT_OVER_CURRENT:
			reg = (reg & ~EHCI_PS_CLEAR) | EHCI_PS_OCC;
			break;
		case USB_PORT_FEAT_C_RESET:
			ehci->portreset &= ~(1 << le16_to_cpu(req->index));
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

	wait_ms(1);
	len = min3(srclen, le16_to_cpu(req->length), length);
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
static int ehci_halt(struct ehci_priv *ehci)
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
	struct ehci_priv *ehci = to_ehci(host);
	uint32_t reg;
	uint32_t cmd;

	ehci_halt(ehci);

	/* EHCI spec section 4.1 */
	if (ehci_reset(ehci) != 0)
		return -1;

	if (ehci->init)
		ehci->init(ehci->drvdata);

	ehci->qh_list->qh_link = cpu_to_hc32((uint32_t)ehci->qh_list | QH_LINK_TYPE_QH);
	ehci->qh_list->qh_endpt1 = cpu_to_hc32((1 << 15) | (USB_SPEED_HIGH << 12));
	ehci->qh_list->qh_curtd = cpu_to_hc32(QT_NEXT_TERMINATE);
	ehci->qh_list->qt_next = cpu_to_hc32(QT_NEXT_TERMINATE);
	ehci->qh_list->qt_altnext = cpu_to_hc32(QT_NEXT_TERMINATE);
	ehci->qh_list->qt_token = cpu_to_hc32(0x40);

	/* Set async. queue head pointer. */
	ehci_writel(&ehci->hcor->or_asynclistaddr, (uint32_t)ehci->qh_list);

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
	wait_ms(5);

	ehci->rootdev = 0;

	if (ehci->post_init)
		ehci->post_init(ehci->drvdata);

	return 0;
}

static int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int length, int timeout)
{
	struct usb_host *host = dev->host;
	struct ehci_priv *ehci = to_ehci(host);

	if (usb_pipetype(pipe) != PIPE_BULK) {
		dev_dbg(ehci->dev, "non-bulk pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}
	return ehci_submit_async(dev, pipe, buffer, length, NULL);
}

static int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *setup, int timeout)
{
	struct usb_host *host = dev->host;
	struct ehci_priv *ehci = to_ehci(host);

	if (usb_pipetype(pipe) != PIPE_CONTROL) {
		dev_dbg(ehci->dev, "non-control pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}

	if (usb_pipedevice(pipe) == ehci->rootdev) {
		if (ehci->rootdev == 0)
			dev->speed = USB_SPEED_HIGH;
		return ehci_submit_root(dev, pipe, buffer, length, setup);
	}
	return ehci_submit_async(dev, pipe, buffer, length, setup);
}

static int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
	       int length, int interval)
{
	struct usb_host *host = dev->host;
	struct ehci_priv *ehci = to_ehci(host);

	dev_dbg(ehci->dev, "dev=%p, pipe=%lu, buffer=%p, length=%d, interval=%d",
	      dev, pipe, buffer, length, interval);
	return -1;
}

static int ehci_detect(struct device_d *dev)
{
	struct ehci_priv *ehci = dev->priv;

	return usb_host_detect(&ehci->host, 0);
}

int ehci_register(struct device_d *dev, struct ehci_data *data)
{
	struct usb_host *host;
	struct ehci_priv *ehci;
	uint32_t reg;

	ehci = xzalloc(sizeof(struct ehci_priv));
	host = &ehci->host;
	dev->priv = ehci;
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

	ehci->qh_list = dma_alloc_coherent(sizeof(struct QH) * NUM_TD);
	ehci->td = dma_alloc_coherent(sizeof(struct qTD) * NUM_TD);

	host->hw_dev = dev;
	host->init = ehci_init;
	host->submit_int_msg = submit_int_msg;
	host->submit_control_msg = submit_control_msg;
	host->submit_bulk_msg = submit_bulk_msg;

	if (ehci->flags & EHCI_HAS_TT) {
		ehci_reset(ehci);
	}

	dev->detect = ehci_detect;

	usb_register_host(host);

	reg = HC_VERSION(ehci_readl(&ehci->hccr->cr_capbase));
	dev_info(dev, "USB EHCI %x.%02x\n", reg >> 8, reg & 0xff);

	return 0;
}

static int ehci_probe(struct device_d *dev)
{
	struct ehci_data data = {};
	struct ehci_platform_data *pdata = dev->platform_data;

	/* default to EHCI_HAS_TT to not change behaviour of boards
	 * without platform_data
	 */
	if (pdata)
		data.flags = pdata->flags;
	else
		data.flags = EHCI_HAS_TT;

	data.hccr = dev_request_mem_region(dev, 0);
	if (dev->num_resources > 1)
		data.hcor = dev_request_mem_region(dev, 1);
	else
		data.hcor = NULL;

	return ehci_register(dev, &data);
}

static void ehci_remove(struct device_d *dev)
{
	struct ehci_priv *ehci = dev->priv;
	ehci_halt(ehci);
}

static struct driver_d ehci_driver = {
	.name  = "ehci",
	.probe = ehci_probe,
	.remove = ehci_remove,
};
device_platform_driver(ehci_driver);
