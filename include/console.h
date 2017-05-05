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
	int (*puts)(struct console_device *cdev, const char *s);
	int  (*getc)(struct console_device *cdev);
	int (*setbrg)(struct console_device *cdev, int baudrate);
	void (*flush)(struct console_device *cdev);
	int (*set_mode)(struct console_device *cdev, enum console_mode mode);
	int (*open)(struct console_device *cdev);
	int (*close)(struct console_device *cdev);

	char *devname;
	int devid;

	struct list_head list;

	unsigned char f_active;
	char *active_string;

	unsigned int open_count;

	unsigned int baudrate;
	unsigned int baudrate_param;

	const char *linux_console_name;

	struct cdev devfs;
	struct file_operations fops;
};

int console_register(struct console_device *cdev);
int console_unregister(struct console_device *cdev);

struct console_device *console_get_by_dev(struct device_d *dev);
struct console_device *console_get_by_name(const char *name);

extern struct list_head console_list;
#define for_each_console(console) list_for_each_entry(console, &console_list, list)

#define CFG_PBSIZE (CONFIG_CBSIZE+sizeof(CONFIG_PROMPT)+16)

extern int barebox_loglevel;

struct console_device *console_get_first_active(void);

int console_open(struct console_device *cdev);
int console_close(struct console_device *cdev);
int console_set_active(struct console_device *cdev, unsigned active);
unsigned console_get_active(struct console_device *cdev);
int console_set_baudrate(struct console_device *cdev, unsigned baudrate);
unsigned console_get_baudrate(struct console_device *cdev);

#ifdef CONFIG_PBL_CONSOLE
void pbl_set_putc(void (*putcf)(void *ctx, int c), void *ctx);
#else
static inline void pbl_set_putc(void (*putcf)(void *ctx, int c), void *ctx) {}
#endif

#endif
