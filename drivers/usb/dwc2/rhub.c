// SPDX-License-Identifier: GPL-2.0+
#include "dwc2.h"

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
		.bNbrPorts		= 1,
		.wHubCharacteristics	= 0,
		.bPwrOn2PwrGood		= 0,
		.bHubContrCurrent	= 0,
		.u.hs.DeviceRemovable	= {0xff},
		.u.hs.PortPwrCtrlMask	= {}
	},
	.device = {
		.bLength		= USB_DT_DEVICE_SIZE,
		.bDescriptorType	= USB_DT_DEVICE,
		.bcdUSB			= __constant_cpu_to_le16(2), /* v2.0 */
		.bDeviceClass		= USB_CLASS_HUB,
		.bDeviceSubClass	= 0,
		.bDeviceProtocol	= USB_HUB_PR_HS_NO_TT,
		.bMaxPacketSize0	= 64,
		.idVendor		= 0x0000,
		.idProduct		= 0x0000,
		.bcdDevice		= 0x0000,
		.iManufacturer		= 1,
		.iProduct		= 2,
		.iSerialNumber		= 0,
		.bNumConfigurations	= 1
	},
	.config = {
		.bLength		= USB_DT_CONFIG_SIZE,
		.bDescriptorType	= USB_DT_CONFIG,
		.wTotalLength		= __constant_cpu_to_le16(
						USB_DT_CONFIG_SIZE +
						USB_DT_INTERFACE_SIZE +
						USB_DT_ENDPOINT_SIZE),
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
		.bInterfaceProtocol	= 0,
		.iInterface		= 0
	},
	.endpoint = {
		.bLength		= USB_DT_ENDPOINT_SIZE,
		.bDescriptorType	= USB_DT_ENDPOINT,
		.bEndpointAddress	= 1 | USB_DIR_IN, /* 0x81 */
		.bmAttributes		= USB_ENDPOINT_XFER_INT,
		.wMaxPacketSize		= __constant_cpu_to_le16(
						(USB_MAXCHILDREN + 1 + 7) / 8),
		.bInterval		= 255
	},
};

static char *language_string = "\x09\x04";
static char *vendor_string = "u-boot";
static char *product_string = "DWC2 root hub";

/*
 * DWC2 to USB API interface
 */
static int dwc2_submit_rh_msg_in_status(struct dwc2 *dwc2,
					   struct usb_device *dev, void *buffer,
					   int txlen, struct devrequest *cmd)
{
	struct usb_port_status *portsts;
	uint32_t hprt0 = 0;
	uint32_t port_status = 0;
	uint32_t port_change = 0;
	int len = 0;
	int speed;

	switch (cmd->requesttype & (USB_TYPE_MASK | USB_RECIP_MASK)) {
	case USB_TYPE_STANDARD | USB_RECIP_DEVICE:
		*(uint16_t *)buffer = cpu_to_le16(1);
		len = 2;
		break;
	case USB_TYPE_STANDARD | USB_RECIP_INTERFACE:
	case USB_TYPE_STANDARD | USB_RECIP_ENDPOINT:
		*(uint16_t *)buffer = cpu_to_le16(0);
		len = 2;
		break;
	case USB_RT_HUB:	/* USB_TYPE_CLASS | USB_RECIP_DEVICE */
		*(uint32_t *)buffer = cpu_to_le32(0);
		len = 4;
		break;
	case USB_RT_PORT:	/* USB_TYPE_CLASS | USB_RECIP_OTHER */
		hprt0 = dwc2_readl(dwc2, HPRT0);

		if (hprt0 & HPRT0_CONNSTS)
			port_status |= USB_PORT_STAT_CONNECTION;
		if (hprt0 & HPRT0_ENA)
			port_status |= USB_PORT_STAT_ENABLE;
		if (hprt0 & HPRT0_SUSP)
			port_status |= USB_PORT_STAT_SUSPEND;
		if (hprt0 & HPRT0_OVRCURRACT)
			port_status |= USB_PORT_STAT_OVERCURRENT;
		if (hprt0 & HPRT0_RST)
			port_status |= USB_PORT_STAT_RESET;
		if (hprt0 & HPRT0_PWR)
			port_status |= USB_PORT_STAT_POWER;

		speed = (hprt0 & HPRT0_SPD_MASK) >> HPRT0_SPD_SHIFT;
		if (speed == HPRT0_SPD_HIGH_SPEED)
			port_status |= USB_PORT_STAT_HIGH_SPEED;
		else if (speed == HPRT0_SPD_LOW_SPEED)
			port_status |= USB_PORT_STAT_LOW_SPEED;

		if (hprt0 & HPRT0_ENACHG)
			port_change |= USB_PORT_STAT_C_ENABLE;
		if (hprt0 & HPRT0_CONNDET)
			port_change |= USB_PORT_STAT_C_CONNECTION;
		if (hprt0 & HPRT0_OVRCURRCHG)
			port_change |= USB_PORT_STAT_C_OVERCURRENT;

		portsts = buffer;
		portsts->wPortStatus = cpu_to_le16(port_status);
		portsts->wPortChange = cpu_to_le16(port_change);
		len = sizeof(*portsts);

		break;
	default:
		goto unknown;
	}

	dev->act_len = min(len, txlen);
	dev->status = 0;

	return 0;

unknown:
	dev->act_len = 0;
	dev->status = USB_ST_STALLED;

	return -1;
}

static void strtodesc(char *dest, char *src, size_t n)
{
	unsigned int i;

	dest[0] = n;
	dest[1] = 0x3;
	for (i = 2; i < n && *src != '\0'; i += 2) {
		dest[i] = *(src++);
		dest[i + 1] = 0;
	}
}

/* Direction: In ; Request: Descriptor */
static int dwc2_submit_rh_msg_in_descriptor(struct usb_device *dev,
					       void *buffer, int txlen,
					       struct devrequest *cmd)
{
	int len = 0;
	char *src;
	uint16_t wValue = le16_to_cpu(cmd->value);
	uint16_t wLength = le16_to_cpu(cmd->length);

	switch (cmd->requesttype & (USB_TYPE_MASK | USB_RECIP_MASK)) {
	case USB_TYPE_STANDARD | USB_RECIP_DEVICE:
		switch (wValue >> 8) {
		case USB_DT_DEVICE:
			debug("USB_DT_DEVICE request\n");
			len = min3(txlen, (int)descriptor.device.bLength, (int)wLength);
			memcpy(buffer, &descriptor.device, len);
			break;
		case USB_DT_CONFIG:
			debug("USB_DT_CONFIG config\n");
			len = min3(txlen, (int)descriptor.config.wTotalLength, (int)wLength);
			memcpy(buffer, &descriptor.config, len);
			break;
		case USB_DT_STRING:
			debug("USB_DT_STRING: %#x\n", wValue);
			switch (wValue & 0xff) {
			case 0: /* Language */
				src = language_string;
				len = strlen(src) + 2;
				((char *)buffer)[0] = len;
				((char *)buffer)[1] = 0x03;
				memcpy(buffer + 2, src, strlen(src));
				break;
			case 1: /* Vendor */
				src = vendor_string;
				len = 2 * strlen(src) + 2;
				strtodesc(buffer, src, len);
				break;
			case 2: /* Product */
				src = product_string;
				len = 2 * strlen(src) + 2;
				strtodesc(buffer, src, len);
				break;
			default:
				debug("%s(): unknown string index 0x%x\n", __func__, wValue & 0xff);
				goto unknown;
			}
			len = min3(txlen, len, (int)wLength);
			break;
		default:
			debug("%s(): unknown requesttype: 0x%x\n", __func__,
			       cmd->requesttype & (USB_TYPE_MASK | USB_RECIP_MASK));
			goto unknown;
		}
		break;

	case USB_RT_HUB:	/* USB_TYPE_CLASS | USB_RECIP_DEVICE */
		debug("USB_RT_HUB\n");

		len = min3(txlen, (int)descriptor.hub.bLength, (int)wLength);
		memcpy(buffer, &descriptor.hub, len);
		break;
	default:
		goto unknown;
	}

	dev->act_len = len;
	dev->status = 0;

	return 0;

unknown:
	dev->act_len = 0;
	dev->status = USB_ST_STALLED;

	return -1;
}

/* Direction: In ; Request: Configuration */
static int dwc2_submit_rh_msg_in_configuration(struct usb_device *dev,
					       void *buffer, int txlen,
					       struct devrequest *cmd)
{
	int len = 0;

	switch (cmd->requesttype & (USB_TYPE_MASK | USB_RECIP_MASK)) {
	case USB_TYPE_STANDARD | USB_RECIP_DEVICE:
		*(uint8_t *)buffer = 0x01;
		len = min(txlen, 1);
		break;
	default:
		goto unknown;
	}

	dev->act_len = len;
	dev->status = 0;

	return 0;

unknown:
	dev->act_len = 0;
	dev->status = USB_ST_STALLED;

	return -1;
}

/* Direction: In */
static int dwc2_submit_rh_msg_in(struct dwc2 *dwc2,
				 struct usb_device *dev, void *buffer,
				 int txlen, struct devrequest *cmd)
{
	switch (cmd->request) {
	case USB_REQ_GET_STATUS:
		return dwc2_submit_rh_msg_in_status(dwc2, dev, buffer,
						       txlen, cmd);
	case USB_REQ_GET_DESCRIPTOR:
		return dwc2_submit_rh_msg_in_descriptor(dev, buffer,
							   txlen, cmd);
	case USB_REQ_GET_CONFIGURATION:
		return dwc2_submit_rh_msg_in_configuration(dev, buffer,
							      txlen, cmd);
	default:
		dev->act_len = 0;
		dev->status = USB_ST_STALLED;

		return -1;
	}
}

/* Direction: Out */
static int dwc2_submit_rh_msg_out(struct dwc2 *dwc2,
				  struct usb_device *dev,
				  void *buffer, int txlen,
				  struct devrequest *cmd)
{
	uint16_t wValue = le16_to_cpu(cmd->value);
	uint32_t hprt0;

	switch (cmd->requesttype & (USB_TYPE_MASK | USB_RECIP_MASK)) {
	case USB_TYPE_STANDARD | USB_RECIP_DEVICE:
		switch (cmd->request) {
		case USB_REQ_SET_ADDRESS:
			dwc2_dbg(dwc2, "set root hub addr %d\n", wValue);
			dwc2->root_hub_devnum = wValue;
			break;
		case USB_REQ_SET_CONFIGURATION:
			break;
		default:
			goto unknown;
		}
		break;
	case USB_TYPE_STANDARD | USB_RECIP_ENDPOINT:
	case USB_RT_HUB:	/* USB_TYPE_CLASS | USB_RECIP_DEVICE */
		switch (cmd->request) {
		case USB_REQ_CLEAR_FEATURE:
			break;
		}
		break;
	case USB_RT_PORT:	/* USB_TYPE_CLASS | USB_RECIP_OTHER */
		switch (cmd->request) {
		case USB_REQ_CLEAR_FEATURE:
			hprt0 = dwc2_readl(dwc2, HPRT0);
			hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET
				   | HPRT0_ENACHG | HPRT0_OVRCURRCHG);
			switch (wValue) {
			case USB_PORT_FEAT_ENABLE:
				hprt0 |= HPRT0_ENA;
				break;
			case USB_PORT_FEAT_SUSPEND:
				break;
			case USB_PORT_FEAT_POWER:
				break;
			case USB_PORT_FEAT_C_CONNECTION:
				hprt0 |= HPRT0_CONNDET;
				break;
			case USB_PORT_FEAT_C_ENABLE:
				hprt0 |= HPRT0_ENACHG;
				break;
			case USB_PORT_FEAT_C_OVER_CURRENT:
				hprt0 |= HPRT0_OVRCURRCHG;
				break;
			default:
				dwc2_dbg(dwc2, "unknown feature 0x%x\n", wValue);
				goto unknown;
			}
			dwc2_writel(dwc2, hprt0, HPRT0);
			break;
		case USB_REQ_SET_FEATURE:
			hprt0 = dwc2_readl(dwc2, HPRT0);
			hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET
				   | HPRT0_ENACHG | HPRT0_OVRCURRCHG);
			switch (wValue) {
			case USB_PORT_FEAT_SUSPEND:
				break;
			case USB_PORT_FEAT_RESET:
				hprt0 |= HPRT0_RST;
				dwc2_writel(dwc2, hprt0, HPRT0);

				mdelay(60);

				hprt0 = dwc2_readl(dwc2, HPRT0);
				hprt0 &= ~HPRT0_RST;
				dwc2_writel(dwc2, hprt0, HPRT0);
				break;
			case USB_PORT_FEAT_POWER:
				break;
			case USB_PORT_FEAT_ENABLE:
				/* Set by the core after a reset */
				break;
			default:
				dwc2_dbg(dwc2, "unknown feature 0x%x\n", wValue);
				goto unknown;
			}
			break;
		default: goto unknown;
		}
		break;
	default:
		goto unknown;
	}

	dev->act_len = 0;
	dev->status = 0;

	return 0;

unknown:
	dev->act_len = 0;
	dev->status = USB_ST_STALLED;
	return -1;
}

int
dwc2_submit_rh_msg(struct dwc2 *dwc2, struct usb_device *dev,
				 unsigned long pipe, void *buffer, int txlen,
				 struct devrequest *cmd)
{
	int stat = 0;

	if (usb_pipeint(pipe)) {
		dwc2_err(dwc2, "Root-Hub submit IRQ: NOT implemented\n");
		return 0;
	}

	if (cmd->requesttype & USB_DIR_IN)
		stat = dwc2_submit_rh_msg_in(dwc2, dev, buffer, txlen, cmd);
	else
		stat = dwc2_submit_rh_msg_out(dwc2, dev, buffer, txlen, cmd);

	mdelay(1);
	return stat;
}
