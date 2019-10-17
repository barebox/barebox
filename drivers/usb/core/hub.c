/*
 * hub.c - USB hub support
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <errno.h>
#include <usb/phy.h>
#include <usb/usb.h>
#include <usb/usb_defs.h>

#include "usb.h"
#include "hub.h"

#define USB_BUFSIZ  512

#define HUB_SHORT_RESET_TIME	20
#define HUB_LONG_RESET_TIME	200

#define PORT_OVERCURRENT_MAX_SCAN_COUNT         3

struct usb_device_scan {
	struct usb_device *dev;		/* USB hub device to scan */
	struct usb_hub_device *hub;	/* USB hub struct */
	int port;			/* USB port to scan */
	struct list_head list;
};

static LIST_HEAD(usb_scan_list);

static int usb_get_hub_descriptor(struct usb_device *dev, void *data, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
		USB_DT_HUB << 8, 0, data, size, USB_CNTL_TIMEOUT);
}

static int usb_clear_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature,
				port, NULL, 0, USB_CNTL_TIMEOUT);
}

static int usb_set_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_FEATURE, USB_RT_PORT, feature,
				port, NULL, 0, USB_CNTL_TIMEOUT);
}

static int usb_get_hub_status(struct usb_device *dev, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
			data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}

static int usb_get_port_status(struct usb_device *dev, int port, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port,
			data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}


static void usb_hub_power_on(struct usb_hub_device *hub)
{
	int i;
	struct usb_device *dev;
	unsigned pgood_delay = hub->desc.bPwrOn2PwrGood * 2;

	dev = hub->pusb_dev;

	/*
	 * Enable power to the ports:
	 * Here we Power-cycle the ports: aka,
	 * turning them off and turning on again.
	 */
	for (i = 0; i < dev->maxchild; i++) {
		usb_clear_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		dev_dbg(&dev->dev, "port %d returns %lX\n", i + 1, dev->status);
	}

	/* Enable power to the ports */
	dev_dbg(&dev->dev, "enabling power on all ports\n");

	for (i = 0; i < dev->maxchild; i++) {
		usb_set_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		dev_dbg(&dev->dev, "port %d returns %lX\n", i + 1, dev->status);
	}

	/*
	 * Do a minimum delay of the larger value of 100ms or pgood_delay
	 * so that the power can stabilize before the devices are queried
	 */
	hub->query_delay = get_time_ns() +
		max(100, (int) pgood_delay) * MSECOND;

	/*
	 * Record the power-on timeout here. The max delay (timeout)
	 * will be done based on this value in the USB port loop in
	 * usb_hub_configure_ports() later.
	 */
	hub->connect_timeout = hub->query_delay + 1000 * MSECOND;

	dev_dbg(&dev->dev, "devnum=%d poweron: query_delay=%d \
			connect_timeout=%d\n",
			dev->devnum, max(100, (int) pgood_delay),
			max(100, (int) pgood_delay) + 1000);
}

#define MAX_TRIES 5

static inline char *portspeed(int portstatus)
{
	if (portstatus & (1 << USB_PORT_FEAT_HIGHSPEED))
		return "480 Mb/s";
	else if (portstatus & (1 << USB_PORT_FEAT_LOWSPEED))
		return "1.5 Mb/s";
	else
		return "12 Mb/s";
}

int hub_port_reset(struct usb_device *hub, int port, struct usb_device *usb)
{
	int tries;
	struct usb_port_status portsts;
	unsigned short portstatus, portchange;
	int delay = HUB_SHORT_RESET_TIME; /* start with short reset delay */

	dev_dbg(&hub->dev, "hub_port_reset: resetting port %d...\n", port);
	for (tries = 0; tries < MAX_TRIES; tries++) {

		usb_set_port_feature(hub, port + 1, USB_PORT_FEAT_RESET);
		mdelay(delay);

		if (usb_get_port_status(hub, port + 1, &portsts) < 0) {
			dev_dbg(&hub->dev, "get_port_status failed status %lX\n",
					hub->status);
			return -1;
		}
		portstatus = le16_to_cpu(portsts.wPortStatus);
		portchange = le16_to_cpu(portsts.wPortChange);

		dev_dbg(&hub->dev, "portstatus %x, change %x, %s\n",
				portstatus, portchange,
				portspeed(portstatus));

		dev_dbg(&hub->dev, "STAT_C_CONNECTION = %d STAT_CONNECTION = %d" \
			       "  USB_PORT_STAT_ENABLE %d\n",
			(portchange & USB_PORT_STAT_C_CONNECTION) ? 1 : 0,
			(portstatus & USB_PORT_STAT_CONNECTION) ? 1 : 0,
			(portstatus & USB_PORT_STAT_ENABLE) ? 1 : 0);

		if ((portchange & USB_PORT_STAT_C_CONNECTION) ||
		    !(portstatus & USB_PORT_STAT_CONNECTION))
			return -1;

		if (portstatus & USB_PORT_STAT_ENABLE)
			break;

		/* Switch to long reset delay for the next round */
		delay = HUB_LONG_RESET_TIME;
	}

	if (tries == MAX_TRIES) {
		dev_dbg(&hub->dev, "Cannot enable port %i after %i retries, " \
				"disabling port.\n", port + 1, MAX_TRIES);
		dev_dbg(&hub->dev, "Maybe the USB cable is bad?\n");
		return -1;
	}

	usb_clear_port_feature(hub, port + 1, USB_PORT_FEAT_C_RESET);

	if (portstatus & USB_PORT_STAT_HIGH_SPEED)
		usb->speed = USB_SPEED_HIGH;
	else if (portstatus & USB_PORT_STAT_LOW_SPEED)
		usb->speed = USB_SPEED_LOW;
	else
		usb->speed = USB_SPEED_FULL;

	return 0;
}


static void usb_hub_port_connect_change(struct usb_device *dev, int port)
{
	struct usb_device *usb;
	struct usb_port_status portsts;
	unsigned short portstatus, portchange;

	/* Check status */
	if (usb_get_port_status(dev, port + 1, &portsts) < 0) {
		dev_dbg(&dev->dev, "get_port_status failed\n");
		return;
	}

	portstatus = le16_to_cpu(portsts.wPortStatus);
	portchange = le16_to_cpu(portsts.wPortChange);
	dev_dbg(&dev->dev, "portstatus %x, change %x, %s\n",
			portstatus, portchange, portspeed(portstatus));

	/* Clear the connection change status */
	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_CONNECTION);

	/* Disconnect any existing devices under this port */
	if (dev->children[port] && !(portstatus & USB_PORT_STAT_CONNECTION)) {
		dev_dbg(&dev->dev, "disconnect detected on port %d\n", port + 1);
		usb_remove_device(dev->children[port]);

		if (!dev->parent && dev->host->usbphy)
			usb_phy_notify_disconnect(dev->host->usbphy, dev->speed);

		return;
	}

	/* Remove disabled but connected devices */
	if (dev->children[port] && !(portstatus & USB_PORT_STAT_ENABLE))
		usb_remove_device(dev->children[port]);

	/* Allocate a new device struct for the port */
	usb = usb_alloc_new_device();
	usb->dev.parent = &dev->dev;
	usb->host = dev->host;

	/* Reset it */
	if (hub_port_reset(dev, port, usb) < 0) {
		dev_warn(&dev->dev, "cannot reset port %i!?\n", port + 1);
		usb_free_device(usb);
		return;
	}

	dev->children[port] = usb;
	usb->parent = dev;
	usb->portnr = port + 1;

	/* Run it through the hoops (find a driver, etc) */
	if (usb_new_device(usb)) {
		/* Woops, disable the port */
		dev_dbg(&dev->dev, "hub: disabling port %d\n", port + 1);
		usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_ENABLE);
		usb_free_device(usb);
		dev->children[port] = NULL;
		return;
	}

	if (!dev->parent && dev->host->usbphy)
		usb_phy_notify_connect(dev->host->usbphy, usb->speed);

	device_detect(&usb->dev);
}

static int usb_scan_port(struct usb_device_scan *usb_scan)
{
	struct usb_port_status portsts;
	unsigned short portstatus, portchange;
	struct usb_device *dev;
	struct usb_hub_device *hub;
	int port;

	dev = usb_scan->dev;
	hub = usb_scan->hub;
	port = usb_scan->port;

	/*
	 * Don't talk to the device before the query delay is expired.
	 * This is needed for voltages to stabilize.
	 */
	if (get_time_ns() < hub->query_delay)
		return 0;

	if (usb_get_port_status(dev, port + 1, &portsts) < 0) {
		dev_dbg(&dev->dev, "get_port_status failed\n");
		if(get_time_ns() >= hub->connect_timeout) {
			dev_dbg(&dev->dev, "port=%d: timeout\n", port + 1);
			/* Remove this device from scanning list */
			goto remove;
		}
		return 0;
	}

	portstatus = le16_to_cpu(portsts.wPortStatus);
	portchange = le16_to_cpu(portsts.wPortChange);
	dev_dbg(&dev->dev, "Port %d Status %X Change %X\n",
			port + 1, portstatus, portchange);

	if (!(portchange & USB_PORT_STAT_C_CONNECTION)) {
		if(get_time_ns() >= hub->connect_timeout) {
			dev_dbg(&dev->dev, "port=%d: timeout\n", port + 1);
			/* Remove this device from scanning list */
			goto remove;
		}
		return 0;
	}

	/* Test if the connection came up, and if not exit */
	if(!(portstatus & USB_PORT_STAT_CONNECTION))
		return 0;

	/* A new USB device is ready at this point */
	dev_dbg(&dev->dev, "port=%d: USB dev found\n", port + 1);

	usb_hub_port_connect_change(dev, port);

	if (portchange & USB_PORT_STAT_C_ENABLE) {
		dev_dbg(&dev->dev, "port %d enable change, status %x\n",
				port + 1, portstatus);
		usb_clear_port_feature(dev, port + 1,
					USB_PORT_FEAT_C_ENABLE);

		/* EM interference sometimes causes bad shielded USB
		 * devices to be shutdown by the hub, this hack enables
		 * them again. Works at least with mouse driver */
		if (!(portstatus & USB_PORT_STAT_ENABLE) &&
		     (portstatus & USB_PORT_STAT_CONNECTION) &&
		     ((dev->children[port]))) {
			dev_dbg(&dev->dev, "already running port %i "  \
					"disabled by hub (EMI?), " \
					"re-enabling...\n", port + 1);
				usb_hub_port_connect_change(dev, port);
		}
	}

	if (portstatus & USB_PORT_STAT_SUSPEND) {
		dev_dbg(&dev->dev, "port %d suspend change\n", port + 1);
		usb_clear_port_feature(dev, port + 1,
					USB_PORT_FEAT_SUSPEND);
	}

	if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
		dev_dbg(&dev->dev, "port %d over-current change\n", port + 1);
		usb_clear_port_feature(dev, port + 1,
					USB_PORT_FEAT_C_OVER_CURRENT);
		/* Only power-on this one port */
		usb_set_port_feature(dev, port + 1, USB_PORT_FEAT_POWER);
		hub->overcurrent_count[port]++;

		/*
		 * If the max-scan-count is not reached, return without removing
		 * the device from scan-list. This will re-issue a new scan.
		 */
		if (hub->overcurrent_count[port] <=
				PORT_OVERCURRENT_MAX_SCAN_COUNT)
			return 0;

		/* Otherwise the device will get removed */
		dev_dbg(&dev->dev,"Port %d over-current occurred %d times\n",
				port + 1, hub->overcurrent_count[port]);
	}

	if (portchange & USB_PORT_STAT_C_RESET) {
		dev_dbg(&dev->dev, "port %d reset change\n", port + 1);
		usb_clear_port_feature(dev, port + 1,
					USB_PORT_FEAT_C_RESET);
	}

remove:
	/*
	 * We're done with this device, so let's remove this device from
	 * scanning list
	 */
	list_del(&usb_scan->list);
	free(usb_scan);

	return 0;
}

static int usb_device_list_scan(void)
{
	struct usb_device_scan *usb_scan;
	struct usb_device_scan *tmp;
	static int running;
	int ret = 0;

	/* Only run this loop once for each controller */
	if (running)
		return 0;

	running = 1;

	while (1) {
		/* We're done, once the list is empty again */
		if (list_empty(&usb_scan_list))
			goto out;

		list_for_each_entry_safe(usb_scan, tmp, &usb_scan_list, list) {
			/* Scan this port */
			ret = usb_scan_port(usb_scan);
			if (ret)
				goto out;
		}
	}

out:
	/*
	 * This USB controller has finished scanning all its connected
	 * USB devices. Set "running" back to 0, so that other USB controllers
	 * will scan their devices too.
	 */
	running = 0;

	return ret;
}

static int usb_hub_configure(struct usb_device *dev)
{
	unsigned char buffer[USB_BUFSIZ], *bitmap;
	struct usb_hub_descriptor *descriptor;
	struct usb_hub_status *hubsts;
	int i;
	struct usb_hub_device *hub;

	hub = xzalloc(sizeof (*hub));
	dev->hub = hub;

	hub->pusb_dev = dev;
	/* Get the the hub descriptor */
	if (usb_get_hub_descriptor(dev, buffer, 4) < 0) {
		dev_dbg(&dev->dev, "%s: failed to get hub " \
				   "descriptor, giving up %lX\n", __func__, dev->status);
		return -1;
	}
	descriptor = (struct usb_hub_descriptor *)buffer;

	/* silence compiler warning if USB_BUFSIZ is > 256 [= sizeof(char)] */
	i = descriptor->bLength;
	if (i > USB_BUFSIZ) {
		dev_dbg(&dev->dev, "%s: failed to get hub " \
				"descriptor - too long: %d\n", __func__,
				descriptor->bLength);
		return -1;
	}

	if (usb_get_hub_descriptor(dev, buffer, descriptor->bLength) < 0) {
		dev_dbg(&dev->dev, "%s: failed to get hub " \
				"descriptor 2nd giving up %lX\n", __func__, dev->status);
		return -1;
	}
	memcpy((unsigned char *)&hub->desc, buffer, descriptor->bLength);
	/* adjust 16bit values */
	hub->desc.wHubCharacteristics =
				le16_to_cpu(descriptor->wHubCharacteristics);
	/* set the bitmap */
	bitmap = (unsigned char *)&hub->desc.u.hs.DeviceRemovable[0];
	/* devices not removable by default */
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8);
	bitmap = (unsigned char *)&hub->desc.u.hs.PortPwrCtrlMask[0];
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8); /* PowerMask = 1B */

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++)
		hub->desc.u.hs.DeviceRemovable[i] = descriptor->u.hs.DeviceRemovable[i];

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++)
		hub->desc.u.hs.PortPwrCtrlMask[i] = descriptor->u.hs.PortPwrCtrlMask[i];

	dev->maxchild = descriptor->bNbrPorts;
	dev_dbg(&dev->dev, "%d ports detected\n", dev->maxchild);

	switch (hub->desc.wHubCharacteristics & HUB_CHAR_LPSM) {
	case 0x00:
		dev_dbg(&dev->dev, "ganged power switching\n");
		break;
	case 0x01:
		dev_dbg(&dev->dev, "individual port power switching\n");
		break;
	case 0x02:
	case 0x03:
		dev_dbg(&dev->dev, "unknown reserved power switching mode\n");
		break;
	}

	if (hub->desc.wHubCharacteristics & HUB_CHAR_COMPOUND)
		dev_dbg(&dev->dev, "part of a compound device\n");
	else
		dev_dbg(&dev->dev, "standalone hub\n");

	switch (hub->desc.wHubCharacteristics & HUB_CHAR_OCPM) {
	case 0x00:
		dev_dbg(&dev->dev, "global over-current protection\n");
		break;
	case 0x08:
		dev_dbg(&dev->dev, "individual port over-current protection\n");
		break;
	case 0x10:
	case 0x18:
		dev_dbg(&dev->dev, "no over-current protection\n");
		break;
	}

	dev_dbg(&dev->dev, "power on to power good time: %dms\n",
			descriptor->bPwrOn2PwrGood * 2);
	dev_dbg(&dev->dev, "hub controller current requirement: %dmA\n",
			descriptor->bHubContrCurrent);

	for (i = 0; i < dev->maxchild; i++)
		dev_dbg(&dev->dev, "port %d is%s removable\n", i + 1,
			hub->desc.u.hs.DeviceRemovable[(i + 1) / 8] & \
					   (1 << ((i + 1) % 8)) ? " not" : "");

	if (sizeof(struct usb_hub_status) > USB_BUFSIZ) {
		dev_dbg(&dev->dev, "%s: failed to get Status - " \
				"too long: %d\n", __func__, descriptor->bLength);
		return -1;
	}

	if (usb_get_hub_status(dev, buffer) < 0) {
		dev_dbg(&dev->dev, "%s: failed to get Status %lX\n", __func__,
				dev->status);
		return -1;
	}

	hubsts = (struct usb_hub_status *)buffer;
	dev_dbg(&dev->dev, "get_hub_status returned status %X, change %X\n",
			le16_to_cpu(hubsts->wHubStatus),
			le16_to_cpu(hubsts->wHubChange));
	dev_dbg(&dev->dev, "local power source is %s\n",
		(le16_to_cpu(hubsts->wHubStatus) & HUB_STATUS_LOCAL_POWER) ? \
		"lost (inactive)" : "good");
	dev_dbg(&dev->dev, "%sover-current condition exists\n",
		(le16_to_cpu(hubsts->wHubStatus) & HUB_STATUS_OVERCURRENT) ? \
		"" : "no ");
	usb_hub_power_on(hub);

	return 0;
}

static int usb_hub_configure_ports(struct usb_device *dev)
{
	struct usb_device_scan *usb_scan;
	int i;

	/*
	 * Only add the connected USB devices, including potential hubs,
	 * to a scanning list. This list will get scanned and devices that
	 * are detected (either via port connected or via port timeout)
	 * will get removed from this list. Scanning of the devices on this
	 * list will continue until all devices are removed.
	 */
	for (i = 0; i < dev->maxchild; i++) {
		usb_scan = calloc(1, sizeof(*usb_scan));
		if (!usb_scan) {
			dev_err(&dev->dev, "Can't allocate memory for \
					USB device scanning!\n");
			return -ENOMEM;
		}
		usb_scan->dev = dev;
		usb_scan->hub = dev->hub;
		usb_scan->port = i;
		list_add_tail(&usb_scan->list, &usb_scan_list);
	}

	/*
	 * And now call the scanning code which loops over the generated list
	 */
	return usb_device_list_scan();
}

static int usb_hub_detect(struct device_d *dev)
{
	struct usb_device *usbdev = container_of(dev, struct usb_device, dev);
	int i;

	usb_hub_configure_ports(usbdev);

	for (i = 0; i < usbdev->maxchild; i++) {
		if (usbdev->children[i])
			device_detect(&usbdev->children[i]->dev);
	}

	return 0;
}

static int usb_hub_probe(struct usb_device *usbdev,
			 const struct usb_device_id *id)
{
	usbdev->dev.detect = usb_hub_detect;

	return usb_hub_configure(usbdev);
}

static void usb_hub_disconnect(struct usb_device *usbdev)
{
	free(usbdev->hub);
}

/* Table with supported devices, most specific first. */
static struct usb_device_id usb_hubage_usb_ids[] = {
	{
		.match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
		.bInterfaceClass = USB_CLASS_HUB,
	},
	{ }
};


/***********************************************************************
 * USB Storage driver initialization and registration
 ***********************************************************************/

static struct usb_driver usb_hubage_driver = {
	.name =		"usb-hub",
	.id_table =	usb_hubage_usb_ids,
	.probe =	usb_hub_probe,
	.disconnect =	usb_hub_disconnect,
};

static int __init usb_hub_init(void)
{
	return usb_driver_register(&usb_hubage_driver);
}
device_initcall(usb_hub_init);
