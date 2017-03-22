/*
 * USB keyboard driver for barebox
 *
 * (C) Copyright 2001 Denis Peter, MPL AG Switzerland
 * (C) Copyright 2015 Peter Mamonov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <poller.h>
#include <usb/usb.h>
#include <string.h>
#include <dma.h>
#include <input/input.h>

/*
 * NOTE: It's important for the NUM, CAPS, SCROLL-lock bits to be in this
 *       order. See usb_kbd_setled() function!
 */
#define USB_KBD_NUMLOCK		(1 << 0)
#define USB_KBD_CAPSLOCK	(1 << 1)
#define USB_KBD_SCROLLLOCK	(1 << 2)
#define USB_KBD_CTRL		(1 << 3)

#define USB_KBD_LEDMASK		\
	(USB_KBD_NUMLOCK | USB_KBD_CAPSLOCK | USB_KBD_SCROLLLOCK)

/*
 * USB Keyboard reports are 8 bytes in boot protocol.
 * Appendix B of HID Device Class Definition 1.11
 */
#define USB_KBD_BOOT_REPORT_SIZE 8

struct usb_kbd_pdata;

struct usb_kbd_pdata {
	uint8_t		*new;
	uint8_t		old[USB_KBD_BOOT_REPORT_SIZE];
	struct poller_async	poller;
	struct usb_device	*usbdev;
	unsigned long	intpipe;
	int		intpktsize;
	int		intinterval;
	struct usb_endpoint_descriptor *ep;
	int (*do_poll)(struct usb_kbd_pdata *);
	struct input_device input;
};

static void usb_kbd_release_all_keys(struct usb_kbd_pdata *data)
{
	int i;

	for (i = 0; i <= KEY_MAX; i++)
		input_report_key_event(&data->input, i, 0);
}

static int usb_kbd_int_poll(struct usb_kbd_pdata *data)
{
	return usb_submit_int_msg(data->usbdev, data->intpipe, data->new,
				  data->intpktsize, data->intinterval);
}

static int usb_kbd_cnt_poll(struct usb_kbd_pdata *data)
{
	struct usb_interface *iface = &data->usbdev->config.interface[0];

	return usb_get_report(data->usbdev, iface->desc.bInterfaceNumber,
			      1, 0, data->new, USB_KBD_BOOT_REPORT_SIZE);
}

static const unsigned char usb_kbd_keycode[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
	191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
	122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140
};

static void usb_kbd_poll(void *arg)
{
	struct usb_kbd_pdata *data = arg;
	struct usb_device *usbdev = data->usbdev;
	int ret, i;

	ret = data->do_poll(data);
	if (ret < 0)
		usb_kbd_release_all_keys(data);
	if (ret == -EAGAIN)
		goto exit;
	if (ret < 0) {
		/* exit with noreturn */
		dev_err(&usbdev->dev,
			"usb_submit_int_msg() failed. Keyboard disconnect?\n");
		return;
	}

	if (!memcmp(data->old, data->new, USB_KBD_BOOT_REPORT_SIZE))
		goto exit;

	pr_debug("%s: new report: %016llx\n", __func__, *((volatile uint64_t *)data->new));

	for (i = 0; i < 8; i++)
		input_report_key_event(&data->input, usb_kbd_keycode[i + 224], (data->new[0] >> i) & 1);

	for (i = 2; i < 8; i++) {
		if (data->old[i] > 3 && memscan(data->new + 2, data->old[i], 6) == data->new + 8) {
			if (usb_kbd_keycode[data->old[i]])
				input_report_key_event(&data->input, usb_kbd_keycode[data->old[i]], 0);
			else
				dev_err(&usbdev->dev,
					 "Unknown key (scancode %#x) released.\n",
					 data->old[i]);
		}

		if (data->new[i] > 3 && memscan(data->old + 2, data->new[i], 6) == data->old + 8) {
			if (usb_kbd_keycode[data->new[i]])
				input_report_key_event(&data->input, usb_kbd_keycode[data->new[i]], 1);
			else
				dev_err(&usbdev->dev,
					 "Unknown key (scancode %#x) pressed.\n",
					 data->new[i]);
		}
	}

	memcpy(data->old, data->new, USB_KBD_BOOT_REPORT_SIZE);

exit:
	poller_call_async(&data->poller, data->intinterval * MSECOND, usb_kbd_poll, data);
}

static int usb_kbd_probe(struct usb_device *usbdev,
			 const struct usb_device_id *id)
{
	int ret;
	struct usb_interface *iface = &usbdev->config.interface[0];
	struct usb_kbd_pdata *data;

	dev_info(&usbdev->dev, "USB keyboard found\n");
	dev_dbg(&usbdev->dev, "Debug enabled\n");
	ret = usb_set_protocol(usbdev, iface->desc.bInterfaceNumber, 0);
	if (ret < 0)
		return ret;

	ret = usb_set_idle(usbdev, iface->desc.bInterfaceNumber, 1, 0);
	if (ret < 0)
		return ret;

	data = xzalloc(sizeof(struct usb_kbd_pdata));
	usbdev->drv_data = data;
	data->new = dma_alloc(USB_KBD_BOOT_REPORT_SIZE);

	data->usbdev = usbdev;

	data->ep = &iface->ep_desc[0];
	data->intpipe = usb_rcvintpipe(usbdev, data->ep->bEndpointAddress);
	data->intpktsize = min(usb_maxpacket(usbdev, data->intpipe),
			       USB_KBD_BOOT_REPORT_SIZE);
	data->intinterval = data->ep->bInterval;
	/* test polling via interrupt endpoint */
	data->do_poll = usb_kbd_int_poll;
	ret = data->do_poll(data);
	if (ret < 0) {
		/* fall back to polling via control enpoint */
		data->do_poll = usb_kbd_cnt_poll;
		usb_set_idle(usbdev,
			     iface->desc.bInterfaceNumber, 0, 0);
		ret = data->do_poll(data);
		if (ret < 0) {
			/* no luck */
			dma_free(data->new);
			free(data);
			return ret;
		} else
			dev_dbg(&usbdev->dev, "poll keyboard via cont ep\n");
	} else
		dev_dbg(&usbdev->dev, "poll keyboard via int ep\n");

	ret = input_device_register(&data->input);
	if (ret) {
		dev_err(&usbdev->dev, "can't register input\n");
		return ret;
	}

	ret = poller_async_register(&data->poller);
	if (ret) {
		dev_err(&usbdev->dev, "can't setup poller\n");
		return ret;
	}

	poller_call_async(&data->poller, data->intinterval * MSECOND, usb_kbd_poll, data);

	return 0;
}

static void usb_kbd_disconnect(struct usb_device *usbdev)
{
	struct usb_kbd_pdata *data = usbdev->drv_data;

	usb_kbd_release_all_keys(data);
	poller_async_unregister(&data->poller);
	input_device_unregister(&data->input);
	dma_free(data->new);
	free(data);
}

static struct usb_device_id usb_kbd_usb_ids[] = {
	{ USB_INTERFACE_INFO(3, 1, 1) }, // usb keyboard
	{ }
};

static struct usb_driver usb_kbd_driver = {
	.name =		"usb-keyboard",
	.id_table =	usb_kbd_usb_ids,
	.probe =	usb_kbd_probe,
	.disconnect =	usb_kbd_disconnect,
};

static int __init usb_kbd_init(void)
{
	return usb_driver_register(&usb_kbd_driver);
}
device_initcall(usb_kbd_init);
