/*
 * (C) Copyright 2000
 * Paolo Scaffardi, AIRVENT SAM s.p.a - RIMINI(ITALY), arsenio@tin.it
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <config.h>
#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <param.h>
#include <console.h>
#include <driver.h>
#include <fs.h>
#include <init.h>
#include <clock.h>
#include <kfifo.h>
#include <module.h>
#include <poller.h>
#include <linux/list.h>
#include <linux/stringify.h>
#include <debug_ll.h>

LIST_HEAD(console_list);
EXPORT_SYMBOL(console_list);

#define CONSOLE_UNINITIALIZED		0
#define CONSOLE_INITIALIZED_BUFFER	1
#define CONSOLE_INIT_FULL		2

#define to_console_dev(d) container_of(d, struct console_device, class_dev)

static int initialized = 0;

#define CONSOLE_BUFFER_SIZE	1024

static char console_input_buffer[CONSOLE_BUFFER_SIZE];
static char console_output_buffer[CONSOLE_BUFFER_SIZE];

static struct kfifo __console_input_fifo;
static struct kfifo __console_output_fifo;
static struct kfifo *console_input_fifo = &__console_input_fifo;
static struct kfifo *console_output_fifo = &__console_output_fifo;

static int console_std_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct console_device *cdev = to_console_dev(dev);
	char active[4];
	unsigned int flag = 0, i = 0;

	if (val) {
		if (strchr(val, 'i') && cdev->getc) {
			active[i++] = 'i';
			flag |= CONSOLE_STDIN;
		}

		if (cdev->putc) {
			if (strchr(val, 'o')) {
				active[i++] = 'o';
				flag |= CONSOLE_STDOUT;
			}

			if (strchr(val, 'e')) {
				active[i++] = 'e';
				flag |= CONSOLE_STDERR;
			}
		}
	}

	if (flag && !cdev->f_active) {
		/* The device is being activated, set its baudrate */
		if (cdev->setbrg)
			cdev->setbrg(cdev, cdev->baudrate);
	}

	active[i] = 0;
	cdev->f_active = flag;

	dev_param_set_generic(dev, param, active);

	if (initialized < CONSOLE_INIT_FULL) {
		char ch;
		initialized = CONSOLE_INIT_FULL;
		puts_ll("Switch to console [");
		puts_ll(dev_name(dev));
		puts_ll("]\n");
		barebox_banner();
		while (kfifo_getc(console_output_fifo, &ch) == 0)
			console_putc(CONSOLE_STDOUT, ch);
	}

	return 0;
}

static int console_baudrate_set(struct param_d *param, void *priv)
{
	struct console_device *cdev = priv;
	unsigned char c;

	/*
	 * If the device is already active, change its baudrate.
	 * The baudrate of an inactive device will be set at activation time.
	 */
	if (cdev->f_active) {
		printf("## Switch baudrate to %d bps and press ENTER ...\n",
			cdev->baudrate);
		mdelay(50);
		cdev->setbrg(cdev, cdev->baudrate);
		mdelay(50);
		do {
			c = getc();
		} while (c != '\r' && c != '\n');
	}

	return 0;
}

static void console_init_early(void)
{
	kfifo_init(console_input_fifo, console_input_buffer,
			CONSOLE_BUFFER_SIZE);
	kfifo_init(console_output_fifo, console_output_buffer,
			CONSOLE_BUFFER_SIZE);

	initialized = CONSOLE_INITIALIZED_BUFFER;
}

int console_register(struct console_device *newcdev)
{
	struct device_d *dev = &newcdev->class_dev;
	int activate = 0;

	if (initialized == CONSOLE_UNINITIALIZED)
		console_init_early();

	dev->id = DEVICE_ID_DYNAMIC;
	strcpy(dev->name, "cs");
	if (newcdev->dev)
		dev->parent = newcdev->dev;
	platform_device_register(dev);

	if (newcdev->setbrg) {
		newcdev->baudrate = CONFIG_BAUDRATE;
		dev_add_param_int(dev, "baudrate", console_baudrate_set,
			NULL, &newcdev->baudrate, "%u", newcdev);
	}

	dev_add_param(dev, "active", console_std_set, NULL, 0);

	if (IS_ENABLED(CONFIG_CONSOLE_ACTIVATE_FIRST)) {
		if (list_empty(&console_list))
			activate = 1;
	} else if (IS_ENABLED(CONFIG_CONSOLE_ACTIVATE_ALL)) {
		activate = 1;
	}

	if (newcdev->dev && of_device_is_stdout_path(newcdev->dev))
		activate = 1;

	list_add_tail(&newcdev->list, &console_list);

	if (activate) {
		if (IS_ENABLED(CONFIG_PARAMETER))
			dev_set_param(dev, "active", "ioe");
		else
			console_std_set(dev, NULL, "ioe");
	}

	return 0;
}
EXPORT_SYMBOL(console_register);

int console_unregister(struct console_device *cdev)
{
	struct device_d *dev = &cdev->class_dev;
	int status;

	list_del(&cdev->list);
	if (list_empty(&console_list))
		initialized = CONSOLE_UNINITIALIZED;

	status = unregister_device(dev);
	if (!status)
		memset(cdev, 0, sizeof(*cdev));
	return status;
}
EXPORT_SYMBOL(console_unregister);

static int getc_raw(void)
{
	struct console_device *cdev;
	int active = 0;

	while (1) {
		for_each_console(cdev) {
			if (!(cdev->f_active & CONSOLE_STDIN))
				continue;
			active = 1;
			if (cdev->tstc(cdev))
				return cdev->getc(cdev);
		}
		if (!active)
			/* no active console found. bail out */
			return -1;
	}
}

static int tstc_raw(void)
{
	struct console_device *cdev;

	for_each_console(cdev) {
		if (!(cdev->f_active & CONSOLE_STDIN))
			continue;
		if (cdev->tstc(cdev))
			return 1;
	}

	return 0;
}

int getc(void)
{
	unsigned char ch;
	uint64_t start;

	if (unlikely(!console_is_input_allow()))
		return -EPERM;

	/*
	 * For 100us we read the characters from the serial driver
	 * into a kfifo. This helps us not to lose characters
	 * in small hardware fifos.
	 */
	start = get_time_ns();
	while (1) {
		if (tstc_raw()) {
			kfifo_putc(console_input_fifo, getc_raw());

			start = get_time_ns();
		}
		if (is_timeout(start, 100 * USECOND) &&
				kfifo_len(console_input_fifo))
			break;
	}

	kfifo_getc(console_input_fifo, &ch);
	return ch;
}
EXPORT_SYMBOL(getc);

int fgetc(int fd)
{
	char c;

	if (!fd)
		return getc();
	return read(fd, &c, 1);
}
EXPORT_SYMBOL(fgetc);

int tstc(void)
{
	if (unlikely(!console_is_input_allow()))
		return 0;

	return kfifo_len(console_input_fifo) || tstc_raw();
}
EXPORT_SYMBOL(tstc);

void console_putc(unsigned int ch, char c)
{
	struct console_device *cdev;
	int init = initialized;

	switch (init) {
	case CONSOLE_UNINITIALIZED:
		console_init_early();
		/* fall through */

	case CONSOLE_INITIALIZED_BUFFER:
		kfifo_putc(console_output_fifo, c);
		putc_ll(c);
		return;

	case CONSOLE_INIT_FULL:
		for_each_console(cdev) {
			if (cdev->f_active & ch) {
				if (c == '\n')
					cdev->putc(cdev, '\r');
				cdev->putc(cdev, c);
			}
		}
		return;
	default:
		/* If we have problems inititalizing our data
		 * get them early
		 */
		hang();
	}
}
EXPORT_SYMBOL(console_putc);

int console_puts(unsigned int ch, const char *str)
{
	const char *s = str;
	int n = 0;

	while (*s) {
		if (*s == '\n') {
			console_putc(ch, '\r');
			n++;
		}
		console_putc(ch, *s);
		n++;
		s++;
	}
	return n;
}
EXPORT_SYMBOL(console_puts);

void console_flush(void)
{
	struct console_device *cdev;

	for_each_console(cdev) {
		if (cdev->flush)
			cdev->flush(cdev);
	}
}
EXPORT_SYMBOL(console_flush);

#ifndef ARCH_HAS_CTRLC
/* test if ctrl-c was pressed */
int ctrlc (void)
{
	poller_call();

	if (tstc() && getc() == 3)
		return 1;
	return 0;
}
EXPORT_SYMBOL(ctrlc);
#endif /* ARCH_HAS_CTRC */
