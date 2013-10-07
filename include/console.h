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

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <param.h>
#include <linux/list.h>
#include <driver.h>

#define CONSOLE_STDIN           (1 << 0)
#define CONSOLE_STDOUT          (1 << 1)
#define CONSOLE_STDERR          (1 << 2)

enum console_mode {
	CONSOLE_MODE_NORMAL,
	CONSOLE_MODE_RS485,
};

struct console_device {
	struct device_d *dev;
	struct device_d class_dev;

	int (*tstc)(struct console_device *cdev);
	void (*putc)(struct console_device *cdev, char c);
	int  (*getc)(struct console_device *cdev);
	int (*setbrg)(struct console_device *cdev, int baudrate);
	void (*flush)(struct console_device *cdev);
	int (*set_mode)(struct console_device *cdev, enum console_mode mode);

	struct list_head list;

	unsigned char f_active;

	unsigned int baudrate;
};

int console_register(struct console_device *cdev);
int console_unregister(struct console_device *cdev);

struct console_device *console_get_by_dev(struct device_d *dev);

extern struct list_head console_list;
#define for_each_console(console) list_for_each_entry(console, &console_list, list)

#define CFG_PBSIZE (CONFIG_CBSIZE+sizeof(CONFIG_PROMPT)+16)

bool console_is_input_allow(void);
void console_allow_input(bool val);

extern int barebox_loglevel;

struct console_device *console_get_first_active(void);

#endif
