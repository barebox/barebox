// SPDX-License-Identifier: GPL-2.0-or-later
#include "dwc2.h"

static struct descriptor {
	struct usb_hub_descriptor hub;
	struct usb_device_descriptor device;
	struct usb_config_descriptor config;
	struct usb_interface_descriptor interface;
	struct usb_endpoint_descriptor endpoint;
}  __packed descriptor = {
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
		.bcdUSB			= cpu_to_le16(2), /* v2.0 */
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
		.wTotalLength		= cpu_to_le16(
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
		.wMaxPacketSize		= cpu_to_le16(
						(USB_MAXCHILDREN + 1 + 7) / 8),
		.bInterval		= 255
	},
};

static int dwc2_get_port_status(struct dwc2 *dwc2, struct usb_device *dev,
		void *buf, int len)
{
	struct usb_port_status *portsts;
	uint32_t hprt0;
	uint32_t status = 0;
	uint32_t change = 0;
	int speed;

	if (!buf || len < sizeof(*portsts))
		return -1;

	hprt0 = dwc2_readl(dwc2, HPRT0);

	if (hprt0 & HPRT0_CONNSTS)
		status |= USB_PORT_STAT_CONNECTION;
	if (hprt0 & HPRT0_ENA)
		status |= USB_PORT_STAT_ENABLE;
	if (hprt0 & HPRT0_SUSP)
		status |= USB_PORT_STAT_SUSPEND;
	if (hprt0 & HPRT0_OVRCURRACT)
		status |= USB_PORT_STAT_OVERCURRENT;
	if (hprt0 & HPRT0_RST)
		status |= USB_PORT_STAT_RESET;
	if (hprt0 & HPRT0_PWR)
		status |= USB_PORT_STAT_POWER;

	speed = (hprt0 & HPRT0_SPD_MASK) >> HPRT0_SPD_SHIFT;
	if (speed == HPRT0_SPD_HIGH_SPEED)
		status |= USB_PORT_STAT_HIGH_SPEED;
	else if (speed == HPRT0_SPD_LOW_SPEED)
		status |= USB_PORT_STAT_LOW_SPEED;

	if (hprt0 & HPRT0_ENACHG)
		change |= USB_PORT_STAT_C_ENABLE;
	if (hprt0 & HPRT0_CONNDET)
		change |= USB_PORT_STAT_C_CONNECTION;
	if (hprt0 & HPRT0_OVRCURRCHG)
		change |= USB_PORT_STAT_C_OVERCURRENT;

	portsts = buf;
	portsts->wPortStatus = cpu_to_le16(status);
	portsts->wPortChange = cpu_to_le16(change);

	dev->act_len = sizeof(*portsts);
	dev->status = 0;

	return 0;
}

static int dwc2_get_hub_status(struct dwc2 *dwc2, struct usb_device *dev,
		void *buf, int len)
{
	if (!buf || len < 4)
		return -1;

	*(uint32_t *)buf = 0;
	dev->act_len = 4;
	dev->status  = 0;

	return 0;
}

static int dwc2_get_hub_descriptor(struct dwc2 *dwc2, struct usb_device *dev,
		void *buf, int len)
{
	if (!buf)
		return -1;

	dev->act_len = min_t(int, len, descriptor.hub.bLength);
	dev->status = 0;
	memcpy(buf, &descriptor.hub, dev->act_len);

	return 0;
}

static void strle16(__le16 *dest, char *src, size_t n)
{
	unsigned int i;

	for (i = 0; i < n && *src != '\0'; i++, src++)
		dest[i] = cpu_to_le16(*src);
}

static int dwc2_get_string_descriptor(struct dwc2 *dwc2, struct usb_device *dev,
			       void *buf, int len, int index)
{
	char *src, *str = buf;
	__le16 *le16 = (__le16 *)(str + 2);
	int size;

	if (!buf || len < 2)
		return -1;

	switch (index) {
	case 0: /* Language */
		src = "\x09\x04";
		size = strlen(src) + 2;
		len = min_t(int, len, size);

		str[0] = size;
		str[1] = 0x03;
		memcpy(str + 2, src, len - 2);
		break;
	case 1: /* Vendor */
		src = "u-boot";
		size = 2 * strlen(src) + 2;
		len = min_t(int, len, size);

		str[0] = size;
		str[1] = 0x03;
		strle16(le16, src, (len - 2) / 2);
		break;
	case 2: /* Product */
		src = "DWC2 root hub";
		size = 2 * strlen(src) + 2;
		len = min_t(int, len, size);

		str[0] = size;
		str[1] = 0x03;
		strle16(le16, src, (len - 2) / 2);
		break;
	default:
		dwc2_err(dwc2, "roothub: unknown string descriptor: 0x%x\n",
				index);
		return -1;
	}

	dev->act_len = len;
	dev->status = 0;

	return 0;
}

static int dwc2_get_descriptor(struct dwc2 *dwc2, struct usb_device *dev,
			       void *buf, int len, int value)
{
	int index = value >> 8;

	if (!buf || len < 0)
		return -1;

	switch (index) {
	case USB_DT_DEVICE:
		len = min(len, (int)descriptor.device.bLength);
		memcpy(buf, &descriptor.device, len);
		break;
	case USB_DT_CONFIG:
		len = min(len, (int)descriptor.config.wTotalLength);
		memcpy(buf, &descriptor.config, len);
		break;
	case USB_DT_STRING:
		value &= 0xff;
		return dwc2_get_string_descriptor(dwc2, dev, buf, len, value);
	default:
		dwc2_err(dwc2, "roothub: unknown descriptor: 0x%x\n", index);
		return -1;
	}

	dev->act_len = len;
	dev->status = 0;

	return 0;
}

static int dwc2_set_port_feature(struct dwc2 *dwc2, struct usb_device *dev,
		int feature)
{
	uint32_t hprt0;

	hprt0 = dwc2_readl(dwc2, HPRT0);
	hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET | HPRT0_ENACHG | HPRT0_OVRCURRCHG);

	switch (feature) {
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
		dwc2_dbg(dwc2, "roothub: unsupported set port feature 0x%x\n",
				feature);
		return -1;
	}

	dev->act_len = 0;
	dev->status = 0;

	return 0;
}

static int dwc2_clear_port_feature(struct dwc2 *dwc2, struct usb_device *dev,
		int feature)
{
	uint32_t hprt0;

	hprt0 = dwc2_readl(dwc2, HPRT0);
	hprt0 &= ~(HPRT0_ENA | HPRT0_CONNDET | HPRT0_ENACHG | HPRT0_OVRCURRCHG);

	switch (feature) {
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
		dwc2_dbg(dwc2, "roothub: unsupported clear port feature 0x%x\n",
				feature);
		return -1;
	}

	dwc2_writel(dwc2, hprt0, HPRT0);

	dev->act_len = 0;
	dev->status = 0;

	return 0;
}

static int dwc2_set_address(struct dwc2 *dwc2, struct usb_device *dev, int addr)
{
	dwc2_dbg(dwc2, "roothub: set address to %d\n", addr);
	dwc2->root_hub_devnum = addr;

	dev->act_len = 0;
	dev->status = 0;

	return 0;
}

int dwc2_submit_roothub(struct dwc2 *dwc2, struct usb_device *dev,
		unsigned long pipe, void *buf, int len,
		struct devrequest *setup)
{
	unsigned char reqtype = setup->requesttype;
	unsigned char request = setup->request;
	unsigned short value = le16_to_cpu(setup->value);
	unsigned short size  = le16_to_cpu(setup->length);
	int minlen = min_t(int, len, size);

	if (usb_pipeint(pipe)) {
		dwc2_err(dwc2, "roothub: submit IRQ NOT implemented\n");
		return 0;
	}

	dev->act_len = 0;
	dev->status = USB_ST_STALLED;

#define REQ(l, u) ((l) | ((u) << 8))

	switch (REQ(request, reqtype)) {
	case REQ(USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB):
		return dwc2_get_hub_descriptor(dwc2, dev, buf, minlen);

	case REQ(USB_REQ_GET_DESCRIPTOR, USB_DIR_IN):
		return dwc2_get_descriptor(dwc2, dev, buf, minlen, value);

	case REQ(USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB):
		return dwc2_get_hub_status(dwc2, dev, buf, len);

	case REQ(USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT):
		return dwc2_get_port_status(dwc2, dev, buf, len);

	case REQ(USB_REQ_SET_FEATURE, USB_DIR_OUT | USB_RT_PORT):
		return dwc2_set_port_feature(dwc2, dev, value);

	case REQ(USB_REQ_CLEAR_FEATURE, USB_DIR_OUT | USB_RT_PORT):
		return dwc2_clear_port_feature(dwc2, dev, value);

	case REQ(USB_REQ_SET_ADDRESS, USB_DIR_OUT):
		return dwc2_set_address(dwc2, dev, value);

	case REQ(USB_REQ_SET_CONFIGURATION, USB_DIR_OUT):
		dev->act_len = 0;
		dev->status = 0;
		return 0;

	case REQ(USB_REQ_GET_CONFIGURATION, USB_DIR_IN):
		*(char *)buf = 1;
		dev->act_len = 1;
		dev->status = 0;
		return 0;
	}

	dwc2_err(dwc2, "roothub: unsupported request 0x%x requesttype 0x%x\n",
		 request, reqtype);

	return 0;
}
