/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <kfifo.h>
#include <poller.h>
#include <clock.h>
#include <input/input.h>
#include <linux/bitmap.h>
#include <input/keyboard.h>
#include <dt-bindings/input/linux-event-codes.h>
#include <readkey.h>

static LIST_HEAD(input_consumers);

int input_register_notfier(struct input_notifier *in)
{
	list_add_tail(&in->list, &input_consumers);

	return 0;
}

void input_unregister_notfier(struct input_notifier *in)
{
	list_del(&in->list);
}

void input_report_key_event(struct input_device *idev, unsigned int code, int value)
{
	struct input_event event;
	struct input_notifier *in;

	if (code > KEY_MAX)
		return;

	if (value)
		set_bit(code, idev->keys);
	else
		clear_bit(code, idev->keys);

	event.code = code;
	event.value = value;

	list_for_each_entry(in, &input_consumers, list)
		in->notify(in, &event);
}

static LIST_HEAD(input_devices);

int input_device_register(struct input_device *idev)
{
	list_add_tail(&idev->list, &input_devices);

	return 0;
}

void input_device_unregister(struct input_device *idev)
{
	list_del(&idev->list);
}

void input_key_get_status(unsigned long *keys, int bits)
{
	struct input_device *idev;

	bitmap_zero(keys, bits);

	if (bits > KEY_MAX)
		bits = KEY_MAX;

	list_for_each_entry(idev, &input_devices, list)
		bitmap_or(keys, keys, idev->keys, bits);
}

struct input_console {
	struct console_device console;
	struct input_notifier notifier;
	struct kfifo *fifo;
	struct poller_async poller;
	uint8_t current_key;
	uint8_t modstate[6];
};

static int input_console_tstc(struct console_device *cdev)
{
	struct input_console *ic = container_of(cdev, struct input_console, console);

	return kfifo_len(ic->fifo) ? 1 : 0;
}

static int input_console_getc(struct console_device *cdev)
{
	struct input_console *ic = container_of(cdev, struct input_console, console);
	uint8_t c;

	kfifo_getc(ic->fifo, &c);

	return c;
}

static void input_console_repeat(void *ctx)
{
	struct input_console *ic = ctx;

	if (ic->current_key) {
		kfifo_putc(ic->fifo, ic->current_key);
		poller_call_async(&ic->poller, 40 * MSECOND,
				  input_console_repeat, ic);
	}
}

struct keycode {
	unsigned char key;
	unsigned char ascii;
};

static void input_console_notify(struct input_notifier *in,
				 struct input_event *ev)
{
	struct input_console *ic = container_of(in, struct input_console, notifier);
	uint8_t modstate = 0;
	unsigned char ascii;

	switch (ev->code) {
	case KEY_LEFTSHIFT:
		ic->modstate[0] = ev->value;
		return;
	case KEY_RIGHTSHIFT:
		ic->modstate[1] = ev->value;
		return;
	case KEY_LEFTCTRL:
		ic->modstate[2] = ev->value;
		return;
	case KEY_RIGHTCTRL:
		ic->modstate[3] = ev->value;
		return;
	case KEY_LEFTALT:
		ic->modstate[4] = ev->value;
		return;
	case KEY_RIGHTALT:
		ic->modstate[5] = ev->value;
		return;
	case KEY_LEFTMETA:
	case KEY_RIGHTMETA:
		return;
	default:
		break;
	}

	if (ic->modstate[0] || ic->modstate[1])
		modstate |= 1 << 0;

	if (ic->modstate[2] || ic->modstate[3])
		modstate |= 1 << 1;

	if (ic->modstate[4] || ic->modstate[5])
		modstate |= 1 << 2;

	if (modstate & (1 << 1)) {
		ascii = keycode_bb_keys[ev->code];
		ascii = ascii >= 'a' ? CTL_CH(ascii) : 0;
	} else if (modstate & (1 << 0))
		ascii = keycode_bb_shift_keys[ev->code];
	else
		ascii = keycode_bb_keys[ev->code];

	pr_debug("map %02x KEY: 0x%04x code: %d\n", modstate, ascii, ev->code);

	if (ev->value) {
		kfifo_putc(ic->fifo, ascii);
		ic->current_key = ascii;
		poller_call_async(&ic->poller, 400 * MSECOND,
				  input_console_repeat, ic);
	} else {
		ic->current_key = 0;
		poller_async_cancel(&ic->poller);
	}
}

static struct input_console input_console;

static int input_init(void)
{
	struct input_console *ic = &input_console;

	ic->console.tstc = input_console_tstc;
	ic->console.getc = input_console_getc;
	ic->console.f_active = CONSOLE_STDIN;
	ic->console.devid = DEVICE_ID_DYNAMIC;
	ic->console.devname = "input";

	ic->fifo = kfifo_alloc(32);
	ic->notifier.notify = input_console_notify;
	input_register_notfier(&ic->notifier);
	poller_async_register(&ic->poller);

	return console_register(&ic->console);
}
console_initcall(input_init);
