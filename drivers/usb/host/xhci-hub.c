/*
 * xHCI USB 3.0 Root Hub
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This currently does not support any SuperSpeed capabilities.
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
#include <io.h>
#include <linux/err.h>
#include <usb/usb.h>
#include <usb/xhci.h>

#include "xhci.h"

static const struct usb_root_hub_info usb_rh_info = {
	.hub = {
		.bLength		= USB_DT_HUB_NONVAR_SIZE +
					  ((USB_MAXCHILDREN + 1 + 7) / 8),
		.bDescriptorType	= USB_DT_HUB,
		.bNbrPorts		= 0,	/* runtime modified */
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
		.bDeviceProtocol	= USB_HUB_PR_HS_MULTI_TT,
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
		.bInterfaceProtocol	= 0,
		.iInterface		= 0
	},
	.endpoint = {
		.bLength		= USB_DT_ENDPOINT_SIZE,
		.bDescriptorType	= USB_DT_ENDPOINT,
		.bEndpointAddress	= 0x81,	/* UE_DIR_IN | EHCI_INTR_ENDPT */
		.bmAttributes		= USB_ENDPOINT_XFER_INT,
		.wMaxPacketSize		= __constant_cpu_to_le16((USB_MAXCHILDREN + 1 + 7) / 8),
		.bInterval		= 255
	}
};

static void xhci_setup_common_hub_descriptor(struct xhci_hcd *xhci,
				     struct usb_hub_descriptor *desc, int ports)
{
	u16 val;

	/* xhci section 5.4.9 says 20ms max */
	desc->bPwrOn2PwrGood = 10;
	desc->bHubContrCurrent = 0;
	desc->bNbrPorts = xhci->num_usb_ports;

	val = 0;
	/* Bits 1:0 - support per-port power switching, or power always on */
	if (HCC_PPC(xhci->hcc_params))
		val |= HUB_CHAR_INDV_PORT_LPSM;
	else
		val |= HUB_CHAR_NO_LPSM;
	/* Bit  2 - root hubs are not part of a compound device */
	/* Bits 4:3 - individual port over current protection */
	val |= HUB_CHAR_INDV_PORT_OCPM;
	/* Bits 6:5 - no TTs in root ports */
	/* Bit  7 - no port indicators */
	desc->wHubCharacteristics = cpu_to_le16(val);
}

static void xhci_setup_usb2_hub_descriptor(struct xhci_hcd *xhci)
{
	struct usb_hub_descriptor *desc = &xhci->usb_info.hub;
	__u8 port_removable[(USB_MAXCHILDREN + 1 + 7) / 8];
	int ports;
	u32 portsc;
	u16 val;
	int i;

	ports = xhci->num_usb_ports;
	xhci_setup_common_hub_descriptor(xhci, desc, ports);
	desc->bDescriptorType = USB_DT_HUB;
	val = 1 + (ports / 8);
	desc->bLength = USB_DT_HUB_NONVAR_SIZE + 2 * val;

	/* The Device Removable bits are reported on a byte granularity.
	 * If the port doesn't exist within that byte, the bit is set to 0.
	 */
	memset(port_removable, 0, sizeof(port_removable));
	for (i = 0; i < ports; i++) {
		portsc = readl(xhci->usb_ports[i]);
		/* If a device is removable, PORTSC reports a 0, same as in the
		 * hub descriptor DeviceRemovable bits.
		 */
		if (portsc & PORT_DEV_REMOVE)
			/* This math is hairy because bit 0 of DeviceRemovable
			 * is reserved, and bit 1 is for port 1, etc.
			 */
			port_removable[(i + 1) / 8] |= 1 << ((i + 1) % 8);
	}

	/* ch11.h defines a hub descriptor that has room for USB_MAXCHILDREN
	 * ports on it.  The USB 2.0 specification says that there are two
	 * variable length fields at the end of the hub descriptor:
	 * DeviceRemovable and PortPwrCtrlMask.  But since we can have less than
	 * USB_MAXCHILDREN ports, we may need to use the DeviceRemovable array
	 * to set PortPwrCtrlMask bits.  PortPwrCtrlMask must always be set to
	 * 0xFF, so we initialize the both arrays (DeviceRemovable and
	 * PortPwrCtrlMask) to 0xFF.  Then we set the DeviceRemovable for each
	 * set of ports that actually exist.
	 */
	memset(desc->u.hs.DeviceRemovable, 0xff,
	       sizeof(desc->u.hs.DeviceRemovable));
	memset(desc->u.hs.PortPwrCtrlMask, 0xff,
	       sizeof(desc->u.hs.PortPwrCtrlMask));

	for (i = 0; i < (ports + 1 + 7) / 8; i++)
		memset(&desc->u.hs.DeviceRemovable[i], port_removable[i],
		       sizeof(__u8));
}

/* FIXME: usb core does not know about USB_SPEED_SUPER at all */
static __maybe_unused void xhci_setup_usb3_hub_descriptor(struct xhci_hcd *xhci)
{
	struct usb_hub_descriptor *desc = &xhci->usb_info.hub;
	int ports;
	u16 port_removable;
	u32 portsc;
	int i;

	ports = xhci->num_usb_ports;
	xhci_setup_common_hub_descriptor(xhci, desc, ports);
	desc->bDescriptorType = USB_DT_SS_HUB;
	desc->bLength = USB_DT_SS_HUB_SIZE;
	/*
	 * header decode latency should be zero for roothubs,
	 * see section 4.23.5.2.
	 */
	desc->u.ss.bHubHdrDecLat = 0;
	desc->u.ss.wHubDelay = 0;
	port_removable = 0;
	/* bit 0 is reserved, bit 1 is for port 1, etc. */
	for (i = 0; i < ports; i++) {
		portsc = readl(xhci->usb_ports[i]);
		if (portsc & PORT_DEV_REMOVE)
			port_removable |= 1 << (i + 1);
	}
	desc->u.ss.DeviceRemovable = cpu_to_le16(port_removable);
}

static void xhci_add_in_port(struct xhci_hcd *xhci, unsigned int num_ports,
		     __le32 __iomem *addr, u8 major_revision, int max_caps)
{
	u32 reg, port_offset, port_count;
	int i;

	if (major_revision > 0x03) {
		dev_warn(xhci->dev, "Ignoring unknown port speed, Ext Cap %p, rev %02x\n",
			 addr, major_revision);
		return;
	}

	/* Port offset and count in the third dword, see section 7.2 */
	reg = readl(addr + 2);
	port_offset = XHCI_EXT_PORT_OFF(reg);
	port_count = XHCI_EXT_PORT_COUNT(reg);

	/* Port count includes the current port offset */
	if (port_offset == 0 || (port_offset + port_count - 1) > num_ports)
		/* WTF? "Valid values are ‘1’ to MaxPorts" */
		return;

	/* cache usb2 port capabilities */
	if (major_revision < 0x03 && xhci->num_ext_caps < max_caps)
		xhci->ext_caps[xhci->num_ext_caps++] = reg;

	port_offset--;
	for (i = port_offset; i < (port_offset + port_count); i++) {
		/* Duplicate entry.  Ignore the port if the revisions differ. */
		if (xhci->port_array[i] != 0) {
			dev_warn(xhci->dev, "Duplicate port entry, Ext Cap %p, port %u\n",
				 addr, i);
			dev_warn(xhci->dev, "Port was marked as USB %u, duplicated as USB %u\n",
				 xhci->port_array[i], major_revision);
			/*
			 * Only adjust the roothub port counts if we haven't
			 * found a similar duplicate.
			 */
			if (xhci->port_array[i] != major_revision &&
			    xhci->port_array[i] != DUPLICATE_ENTRY) {
				xhci->num_usb_ports--;
				xhci->port_array[i] = DUPLICATE_ENTRY;
			}
			continue;
		}
		xhci->port_array[i] = major_revision;
		xhci->num_usb_ports++;
	}
}

int xhci_hub_setup_ports(struct xhci_hcd *xhci)
{
	u32 offset, tmp_offset;
	__le32 __iomem *addr, *tmp_addr;
	unsigned int num_ports;
	int i, cap_count = 0;

	offset = HCC_EXT_CAPS(xhci->hcc_params);
	if (offset == 0) {
		dev_err(xhci->dev, "No Extended Capability Registers\n");
		return -ENODEV;
	}

	addr = &xhci->cap_regs->hc_capbase + offset;

	/* count extended protocol capability entries for later caching */
	tmp_addr = addr;
	tmp_offset = offset;
	do {
		u32 cap_id = readl(tmp_addr);

		if (XHCI_EXT_CAPS_ID(cap_id) == XHCI_EXT_CAPS_PROTOCOL)
			cap_count++;

		tmp_offset = XHCI_EXT_CAPS_NEXT(cap_id);
		tmp_addr += tmp_offset;
	} while (tmp_offset);

	num_ports = HCS_MAX_PORTS(xhci->hcs_params1);
	xhci->port_array = xzalloc(num_ports * sizeof(*xhci->port_array));
	xhci->ext_caps = xzalloc(cap_count * sizeof(*xhci->ext_caps));

	while (1) {
		u32 cap_id = readl(addr);

		if (XHCI_EXT_CAPS_ID(cap_id) == XHCI_EXT_CAPS_PROTOCOL)
			xhci_add_in_port(xhci, num_ports, addr,
					 (u8)XHCI_EXT_PORT_MAJOR(cap_id),
					 cap_count);
		offset = XHCI_EXT_CAPS_NEXT(cap_id);
		if (!offset || xhci->num_usb_ports == num_ports)
                        break;
		addr += offset;
	}

	if (xhci->num_usb_ports == 0) {
		dev_err(xhci->dev, "No ports on the roothubs?\n");
		return -ENODEV;
	}

	xhci->usb_ports = xzalloc(num_ports * sizeof(*xhci->usb_ports));
	for (i = 0; i < num_ports; i++)
		xhci->usb_ports[i] = &xhci->op_regs->port_status_base +
			NUM_PORT_REGS * i;
	memcpy(&xhci->usb_info, &usb_rh_info, sizeof(usb_rh_info));
	xhci_setup_usb2_hub_descriptor(xhci);

	return 0;
}

/*
 * These bits are Read Only (RO) and should be saved and written to the
 * registers: 0, 3, 10:13, 30
 * connect status, over-current status, port speed, and device removable.
 * connect status and port speed are also sticky - meaning they're in
 * the AUX well and they aren't changed by a hot, warm, or cold reset.
 */
#define XHCI_PORT_RO	(PORT_CONNECT | PORT_OC | DEV_SPEED_MASK | \
			 PORT_DEV_REMOVE)
/*
 * These bits are RW; writing a 0 clears the bit, writing a 1 sets the bit:
 * bits 5:8, 9, 14:15, 25:27
 * link state, port power, port indicator state, "wake on" enable state
 */
#define XHCI_PORT_RWS	(PORT_PLS_MASK | PORT_POWER | PORT_LED_MASK | \
			 PORT_WKCONN_E | PORT_WKDISC_E | PORT_WKOC_E)
/*
 * These bits are RW; writing a 1 sets the bit, writing a 0 has no effect:
 * bit 4 (port reset)
 */
#define XHCI_PORT_RW1S	(PORT_RESET)
/*
 * These bits are RW; writing a 1 clears the bit, writing a 0 has no effect:
 * bits 1, 17, 18, 19, 20, 21, 22, 23
 * port enable/disable, and
 * change bits: connect, PED, warm port reset changed (reserved 0 for USB 2.0),
 * over-current, reset, link state, and L1 change
 */
#define XHCI_PORT_RW1CS	(PORT_PE | PORT_CSC | PORT_PEC | PORT_WRC | \
			 PORT_OCC | PORT_RC | PORT_PLC | PORT_CEC)
/*
 * Bit 16 is RW, and writing a '1' to it causes the link state control to be
 * latched in
 */
#define XHCI_PORT_RW	(PORT_LINK_STROBE)
/*
 * These bits are Reserved Zero (RsvdZ) and zero should be written to them:
 * bits 2, 24, 28:31
 */
#define XHCI_PORT_RZ	(BIT(2) | BIT(24) | (0xf<<28))

/*
 * Given a port state, this function returns a value that would result in the
 * port being in the same state, if the value was written to the port status
 * control register.
 * Save Read Only (RO) bits and save read/write bits where
 * writing a 0 clears the bit and writing a 1 sets the bit (RWS).
 * For all other types (RW1S, RW1CS, RW, and RZ), writing a '0' has no effect.
 */
static u32 inline xhci_port_state_to_neutral(u32 state)
{
	/* Save read-only status and port state */
        return (state & XHCI_PORT_RO) | (state & XHCI_PORT_RWS);
}

static int xhci_hub_finish_port_detach(struct xhci_hcd *xhci, int port)
{
	struct xhci_virtual_device *vdev, *temp;
	union xhci_trb trb;
	int ret;

	ret = xhci_wait_for_event(xhci, TRB_PORT_STATUS, &trb);
	if (ret)
		return ret;

	/* Tear-down any attached virtual devices */
	list_for_each_entry_safe(vdev, temp, &xhci->vdev_list, list)
		if (vdev->udev && vdev->udev->portnr == port)
			xhci_virtdev_detach(vdev);

	return 0;
}

static int xhci_hub_finish_port_reset(struct xhci_hcd *xhci, int port)
{
	struct xhci_virtual_device *vdev;
	union xhci_trb trb;
	int ret;

	ret = xhci_wait_for_event(xhci, TRB_PORT_STATUS, &trb);
	if (ret)
		return ret;

	/* Reset any attached virtual devices */
	list_for_each_entry(vdev, &xhci->vdev_list, list)
		if (vdev->udev && vdev->udev->portnr == port)
			xhci_virtdev_reset(vdev);

	return 0;
}

void xhci_hub_port_power(struct xhci_hcd *xhci, int port,
			 bool enable)
{
	u32 reg = readl(xhci->usb_ports[port]);

	reg = xhci_port_state_to_neutral(reg);
	if (enable)
		reg |= PORT_POWER;
	else
		reg &= ~PORT_POWER;
	writel(reg, xhci->usb_ports[port]);
}

static __maybe_unused int xhci_hub_port_warm_reset(struct xhci_hcd *xhci, int port)
{
	void __iomem *portsc = xhci->usb_ports[port];
	u32 reg;

	reg = xhci_port_state_to_neutral(readl(portsc));
	writel(reg | PORT_WR, portsc);
	return xhci_handshake(portsc, PORT_RESET, 0, 10 * SECOND/USECOND);
}

int xhci_hub_control(struct usb_device *dev, unsigned long pipe,
		     void *buffer, int length, struct devrequest *req)
{
	struct usb_host *host = dev->host;
	struct xhci_hcd *xhci = to_xhci_hcd(host);
	struct usb_root_hub_info *info;
	__le32 __iomem **port_array;
	int max_ports;
	void *srcptr = NULL;
	u8 tmpbuf[4];
	u16 typeReq;
	int len, port, srclen = 0;
	u32 reg;

	dev_dbg(xhci->dev, "%s req %u (%#x), type %u (%#x), value %u (%#x), index %u (%#x), length %u (%#x)\n",
		__func__, req->request, req->request,
		req->requesttype, req->requesttype,
		le16_to_cpu(req->value), le16_to_cpu(req->value),
		le16_to_cpu(req->index), le16_to_cpu(req->index),
		le16_to_cpu(req->length), le16_to_cpu(req->length));

	info = &xhci->usb_info;
	port_array = xhci->usb_ports;
	max_ports = xhci->num_usb_ports;

	typeReq = (req->requesttype << 8) | req->request;
	switch (typeReq) {
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		dev_dbg(xhci->dev, "GetDeviceDescriptor %u\n",
			le16_to_cpu(req->value) >> 8);

		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_DEVICE:
			srcptr = &info->device;
			srclen = info->device.bLength;
			break;
		case USB_DT_CONFIG:
			srcptr = &info->config;
			srclen = le16_to_cpu(info->config.wTotalLength);
			break;
		case USB_DT_STRING:
			switch (le16_to_cpu(req->value) & 0xff) {
			case 0:	/* Language */
				srcptr = "\4\3\1\0";
				srclen = 4;
				break;
			case 1:	/* Vendor: "barebox" */
				srcptr = "\20\3b\0a\0r\0e\0b\0o\0x\0";
				srclen = 16;
				break;
			case 2:	/* Product: "USB 3.0 Root Hub" */
				srcptr = "\42\3U\0S\0B\0 \0\63\0.\0\60\0 \0R\0o\0o\0t\0 \0H\0u\0b";
				srclen = 34;
				break;
			default:
				dev_warn(xhci->dev, "unknown string descriptor %x\n",
					 le16_to_cpu(req->value) >> 8);
				goto unknown;
			}
			break;
		}
		break;
	case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
		dev_dbg(xhci->dev, "SetDeviceConfiguration\n");
		/* Nothing to do */
		break;
	case DeviceOutRequest | USB_REQ_SET_ADDRESS:
		dev_dbg(xhci->dev, "SetDeviceAddress %u\n",
			le16_to_cpu(req->value));

		xhci->rootdev = le16_to_cpu(req->value);
		break;
	case GetHubDescriptor:
		dev_dbg(xhci->dev, "GetHubDescriptor %u\n",
			le16_to_cpu(req->value) >> 8);

		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_HUB:
			srcptr = &info->hub;
			srclen = info->hub.bLength;
			break;
		default:
			dev_warn(xhci->dev, "unknown descriptor %x\n",
				 le16_to_cpu(req->value) >> 8);
			goto unknown;
		}
		break;
	case GetHubStatus:
		dev_dbg(xhci->dev, "GetHubStatus\n");

		/* No power source, over-current reported per port */
		tmpbuf[0] = 0x00;
		tmpbuf[1] = 0x00;
		srcptr = tmpbuf;
		srclen = 2;
		break;
	case GetPortStatus:
		dev_dbg(xhci->dev, "GetPortStatus %u\n",
			le16_to_cpu(req->index));

		memset(tmpbuf, 0, 4);

		port = le16_to_cpu(req->index);
		if (!port || port > max_ports)
			goto unknown;
		port--;

		/* read PORTSC register */
		reg = readl(port_array[port]);

		if (reg & PORT_CONNECT) {
			tmpbuf[0] |= USB_PORT_STAT_CONNECTION;
			if (DEV_LOWSPEED(reg))
				tmpbuf[1] |= USB_PORT_STAT_LOW_SPEED >> 8;
			else if (DEV_HIGHSPEED(reg))
				tmpbuf[1] |= USB_PORT_STAT_HIGH_SPEED >> 8;
		}
		if (reg & PORT_PE)
			tmpbuf[0] |= USB_PORT_STAT_ENABLE;
		if (reg & PORT_OC)
			tmpbuf[0] |= USB_PORT_STAT_OVERCURRENT;
		if (reg & PORT_RESET)
			tmpbuf[0] |= USB_PORT_STAT_RESET;
		/* USB 2.0 only */
		if ((reg & PORT_PLS_MASK) == XDEV_U3 && reg & PORT_POWER)
                        tmpbuf[0] |= USB_PORT_STAT_SUSPEND;
		/* USB 2.0 only */
		if (reg & PORT_POWER)
			tmpbuf[1] |= USB_PORT_STAT_POWER >> 8;
		if (reg & PORT_CSC)
			tmpbuf[2] |= USB_PORT_STAT_C_CONNECTION;
		if (reg & PORT_PEC)
			tmpbuf[2] |= USB_PORT_STAT_C_ENABLE;
		if (reg & PORT_OCC)
			tmpbuf[2] |= USB_PORT_STAT_C_OVERCURRENT;
		if (reg & PORT_RC)
			tmpbuf[2] |= USB_PORT_STAT_C_RESET;
		srcptr = tmpbuf;
		srclen = 4;
		break;
	case ClearPortFeature:
		dev_dbg(xhci->dev, "ClearPortFeature %u %u\n",
			le16_to_cpu(req->index), le16_to_cpu(req->value));

		port = le16_to_cpu(req->index);
		if (!port || port > max_ports)
			goto unknown;
		port--;

		reg = xhci_port_state_to_neutral(readl(port_array[port]));

		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
                        reg &= ~PORT_PE;
			break;
		case USB_PORT_FEAT_POWER:
                        reg &= ~PORT_POWER;
			break;
                case USB_PORT_FEAT_C_CONNECTION:
			reg |= PORT_CSC;
			break;
                case USB_PORT_FEAT_C_ENABLE:
			reg |= PORT_PEC;
			break;
                case USB_PORT_FEAT_C_OVER_CURRENT:
			reg |= PORT_OCC;
			break;
                case USB_PORT_FEAT_C_RESET:
			reg |= PORT_RC;
			break;
		default:
			dev_warn(xhci->dev, "unknown feature %u\n",
				 le16_to_cpu(req->value));
			goto unknown;
		}
		writel(reg, port_array[port]);
		readl(port_array[port]);

		if ((reg & PORT_CONNECT) == 0 &&
		    le16_to_cpu(req->value) == USB_PORT_FEAT_C_CONNECTION)
			xhci_hub_finish_port_detach(xhci, port + 1);

		break;
	case SetPortFeature:
		dev_dbg(xhci->dev, "SetPortFeature %u %u\n",
			le16_to_cpu(req->index), le16_to_cpu(req->value));

		port = le16_to_cpu(req->index);
		if (!port || port > max_ports)
			goto unknown;
		port--;

		reg = xhci_port_state_to_neutral(readl(port_array[port]));

		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_POWER:
			reg |= PORT_POWER;
			break;
		case USB_PORT_FEAT_RESET:
			reg |= PORT_RESET;
			break;
		default:
			dev_warn(xhci->dev, "unknown feature %u\n",
				 le16_to_cpu(req->value));
			goto unknown;
		}
		writel(reg, port_array[port]);
		readl(port_array[port]);

		if (le16_to_cpu(req->value) == USB_PORT_FEAT_RESET)
			xhci_hub_finish_port_reset(xhci, port + 1);

		break;
	default:
		dev_warn(xhci->dev, "unknown root hub request %u (%#x) type %u (%#x)\n",
			 req->request, req->request,
			 req->requesttype, req->requesttype);
		goto unknown;
	}

	len = min3(srclen, (int)le16_to_cpu(req->length), length);
	if (srcptr && len)
		memcpy(buffer, srcptr, len);
	dev->act_len = len;
	dev->status = 0;

	return 0;

unknown:
	dev->act_len = 0;
	dev->status = USB_ST_STALLED;
	return -ENOTSUPP;
}
