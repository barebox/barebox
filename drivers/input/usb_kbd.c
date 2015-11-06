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
#include <kfifo.h>

#define USB_KBD_FIFO_SIZE	50

#define REPEAT_RATE	40		/* 40msec -> 25cps */
#define REPEAT_DELAY	10		/* 10 x REPEAT_RATE = 400msec */

#define NUM_LOCK	0x53
#define CAPS_LOCK	0x39
#define SCROLL_LOCK	0x47

/* Modifier bits */
#define LEFT_CNTR	(1 << 0)
#define LEFT_SHIFT	(1 << 1)
#define LEFT_ALT	(1 << 2)
#define LEFT_GUI	(1 << 3)
#define RIGHT_CNTR	(1 << 4)
#define RIGHT_SHIFT	(1 << 5)
#define RIGHT_ALT	(1 << 6)
#define RIGHT_GUI	(1 << 7)

/* Size of the keyboard buffer */
#define USB_KBD_BUFFER_LEN	0x20

/* Keyboard maps */
static const unsigned char usb_kbd_numkey[] = {
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'\r', 0x1b, '\b', '\t', ' ', '-', '=', '[', ']',
	'\\', '#', ';', '\'', '`', ',', '.', '/'
};
static const unsigned char usb_kbd_numkey_shifted[] = {
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
	'\r', 0x1b, '\b', '\t', ' ', '_', '+', '{', '}',
	'|', '~', ':', '"', '~', '<', '>', '?'
};

static const unsigned char usb_kbd_num_keypad[] = {
	'/', '*', '-', '+', '\r',
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'.', 0, 0, 0, '='
};

/*
 * map arrow keys to ^F/^B ^N/^P, can't really use the proper
 * ANSI sequence for arrow keys because the queuing code breaks
 * when a single keypress expands to 3 queue elements
 */
static const unsigned char usb_kbd_arrow[] = {
	0x6, 0x2, 0xe, 0x10
};

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
	uint64_t	last_report;
	uint64_t	last_poll;
	uint8_t		*new;
	uint8_t		old[USB_KBD_BOOT_REPORT_SIZE];
	uint32_t	repeat_delay;
	uint8_t		flags;
	struct poller_struct	poller;
	struct usb_device	*usbdev;
	struct console_device	cdev;
	struct kfifo		*recv_fifo;
	int		lock;
	unsigned long	intpipe;
	int		intpktsize;
	int		intinterval;
	struct usb_endpoint_descriptor *ep;
	int (*do_poll)(struct usb_kbd_pdata *);
};

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

#define CAPITAL_MASK	0x20
/* Translate the scancode in ASCII */
static int usb_kbd_translate(struct usb_kbd_pdata *data, unsigned char scancode,
				unsigned char modifier, int pressed)
{
	int keycode = 0;

	/* Key released */
	if (pressed == 0) {
		data->repeat_delay = 0;
		return 0;
	}

	if (pressed == 2) {
		data->repeat_delay++;
		if (data->repeat_delay < REPEAT_DELAY)
			return 0;

		data->repeat_delay = REPEAT_DELAY;
	}

	/* Alphanumeric values */
	if ((scancode > 3) && (scancode <= 0x1d)) {
		keycode = scancode - 4 + 'a';

		if (data->flags & USB_KBD_CAPSLOCK)
			keycode &= ~CAPITAL_MASK;

		if (modifier & (LEFT_SHIFT | RIGHT_SHIFT)) {
			/* Handle CAPSLock + Shift pressed simultaneously */
			if (keycode & CAPITAL_MASK)
				keycode &= ~CAPITAL_MASK;
			else
				keycode |= CAPITAL_MASK;
		}
	}

	if ((scancode > 0x1d) && (scancode < 0x3a)) {
		/* Shift pressed */
		if (modifier & (LEFT_SHIFT | RIGHT_SHIFT))
			keycode = usb_kbd_numkey_shifted[scancode - 0x1e];
		else
			keycode = usb_kbd_numkey[scancode - 0x1e];
	}

	/* Arrow keys */
	if ((scancode >= 0x4f) && (scancode <= 0x52))
		keycode = usb_kbd_arrow[scancode - 0x4f];

	/* Numeric keypad */
	if ((scancode >= 0x54) && (scancode <= 0x67))
		keycode = usb_kbd_num_keypad[scancode - 0x54];

	if (data->flags & USB_KBD_CTRL)
		keycode = scancode - 0x3;

	if (pressed == 1) {
		if (scancode == NUM_LOCK) {
			data->flags ^= USB_KBD_NUMLOCK;
			return 1;
		}

		if (scancode == CAPS_LOCK) {
			data->flags ^= USB_KBD_CAPSLOCK;
			return 1;
		}
		if (scancode == SCROLL_LOCK) {
			data->flags ^= USB_KBD_SCROLLLOCK;
			return 1;
		}
	}

	/* Report keycode if any */
	if (keycode) {
		pr_debug("%s: key pressed: '%c'\n", __FUNCTION__, keycode);
		kfifo_put(data->recv_fifo, (u_char*)&keycode, sizeof(keycode));
	}

	return 0;
}

static uint32_t usb_kbd_service_key(struct usb_kbd_pdata *data, int i, int up)
{
	uint32_t res = 0;
	uint8_t *new;
	uint8_t *old;

	if (up) {
		new = data->old;
		old = data->new;
	} else {
		new = data->new;
		old = data->old;
	}

	if ((old[i] > 3) &&
	    (memscan(new + 2, old[i], USB_KBD_BOOT_REPORT_SIZE - 2) ==
			new + USB_KBD_BOOT_REPORT_SIZE)) {
		res |= usb_kbd_translate(data, old[i], data->new[0], up);
	}

	return res;
}

static void usb_kbd_setled(struct usb_kbd_pdata *data)
{
	struct usb_device *usbdev = data->usbdev;
	struct usb_interface *iface = &usbdev->config.interface[0];
	uint8_t leds = (uint8_t)(data->flags & USB_KBD_LEDMASK);

	usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0),
		USB_REQ_SET_REPORT, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		0x200, iface->desc.bInterfaceNumber, &leds, 1, USB_CNTL_TIMEOUT);
}


static int usb_kbd_process(struct usb_kbd_pdata *data)
{
	int i, res = 0;

	/* No combo key pressed */
	if (data->new[0] == 0x00)
		data->flags &= ~USB_KBD_CTRL;
	/* Left or Right Ctrl pressed */
	else if ((data->new[0] == LEFT_CNTR) || (data->new[0] == RIGHT_CNTR))
		data->flags |= USB_KBD_CTRL;

	for (i = 2; i < USB_KBD_BOOT_REPORT_SIZE; i++) {
		res |= usb_kbd_service_key(data, i, 0);
		res |= usb_kbd_service_key(data, i, 1);
	}

	/* Key is still pressed */
	if ((data->new[2] > 3) && (data->old[2] == data->new[2]))
		res |= usb_kbd_translate(data, data->new[2], data->new[0], 2);

	if (res == 1)
		usb_kbd_setled(data);

	return 1;
}

static void usb_kbd_poll(struct poller_struct *poller)
{
	struct usb_kbd_pdata *data = container_of(poller,
						  struct usb_kbd_pdata, poller);
	struct usb_device *usbdev = data->usbdev;
	int diff, tout, ret;

	if (data->lock)
		return;
	data->lock = 1;

	if (!is_timeout_non_interruptible(data->last_poll, REPEAT_RATE * MSECOND / 2))
		goto exit;
	data->last_poll = get_time_ns();

	ret = data->do_poll(data);
	if (ret == -EAGAIN)
		goto exit;
	if (ret < 0) {
		/* exit and lock forever */
		dev_err(&usbdev->dev,
			"usb_submit_int_msg() failed. Keyboard disconnect?\n");
		return;
	}
	diff = memcmp(data->old, data->new, USB_KBD_BOOT_REPORT_SIZE);
	tout = get_time_ns() > data->last_report + REPEAT_RATE * MSECOND;
	if (diff || tout) {
		data->last_report = get_time_ns();
		if (diff) {
			pr_debug("%s: old report: %016llx\n",
				__func__,
				*((volatile uint64_t *)data->old));
			pr_debug("%s: new report: %016llx\n\n",
				__func__,
				*((volatile uint64_t *)data->new));
		}
		usb_kbd_process(data);
		memcpy(data->old, data->new, USB_KBD_BOOT_REPORT_SIZE);
	}

exit:
	data->lock = 0;
}

static int usb_kbd_getc(struct console_device *cdev)
{
	int code = 0;
	struct usb_kbd_pdata *data = container_of(cdev, struct usb_kbd_pdata, cdev);

	kfifo_get(data->recv_fifo, (u_char*)&code, sizeof(int));
	return code;
}

static int usb_kbd_tstc(struct console_device *cdev)
{
	struct usb_kbd_pdata *data = container_of(cdev, struct usb_kbd_pdata, cdev);

	return (kfifo_len(data->recv_fifo) == 0) ? 0 : 1;
}

static int usb_kbd_probe(struct usb_device *usbdev,
			 const struct usb_device_id *id)
{
	int ret;
	struct usb_interface *iface = &usbdev->config.interface[0];
	struct usb_kbd_pdata *data;
	struct console_device *cdev;

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
	data->recv_fifo = kfifo_alloc(USB_KBD_FIFO_SIZE);
	data->new = dma_alloc(USB_KBD_BOOT_REPORT_SIZE);

	data->usbdev = usbdev;
	data->last_report = get_time_ns();

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
			kfifo_free(data->recv_fifo);
			dma_free(data->new);
			free(data);
			return ret;
		} else
			dev_dbg(&usbdev->dev, "poll keyboard via cont ep\n");
	} else
		dev_dbg(&usbdev->dev, "poll keyboard via int ep\n");

	cdev = &data->cdev;
	usbdev->dev.type_data = cdev;
	cdev->dev = &usbdev->dev;
	cdev->tstc = usb_kbd_tstc;
	cdev->getc = usb_kbd_getc;

	console_register(cdev);
	console_set_active(cdev, CONSOLE_STDIN);

	data->poller.func = usb_kbd_poll;
	return poller_register(&data->poller);
}

static void usb_kbd_disconnect(struct usb_device *usbdev)
{
	struct usb_kbd_pdata *data = usbdev->drv_data;

	poller_unregister(&data->poller);
	console_unregister(&data->cdev);
	kfifo_free(data->recv_fifo);
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
