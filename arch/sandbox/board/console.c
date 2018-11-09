/*
 * console.c - register a console device
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <driver.h>
#include <mach/linux.h>
#include <xfuncs.h>

int barebox_register_console(int stdinfd, int stdoutfd)
{
	struct device_d *dev;
	struct linux_console_data *data;

	dev = xzalloc(sizeof(struct device_d) + sizeof(struct linux_console_data));

	data = (struct linux_console_data *)(dev + 1);

	dev->platform_data = data;
	dev_set_name(dev, "console");
	dev->id = DEVICE_ID_DYNAMIC;

	data->stdoutfd = stdoutfd;
	data->stdinfd  = stdinfd;

	return sandbox_add_device(dev);
}

