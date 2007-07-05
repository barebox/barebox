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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <param.h>

#define CONSOLE_STDIN           (1 << 0)
#define CONSOLE_STDOUT          (1 << 1)
#define CONSOLE_STDERR          (1 << 2)

struct console_device {
	struct device_d *dev;

	int (*tstc)(struct console_device *cdev);
	void (*putc)(struct console_device *cdev, char c);
	int  (*getc)(struct console_device *cdev);
	struct console_device *next;

	unsigned char f_caps;
	unsigned char f_active;

	struct param_d active_param;
	char active[4];
};

int console_register(struct console_device *cdev);

#endif

