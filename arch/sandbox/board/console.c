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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <driver.h>
#include <mach/linux.h>
#include <xfuncs.h>

int barebox_register_console(char *name, int stdinfd, int stdoutfd)
{
	struct device_d *dev;
	struct linux_console_data *data;

	dev = xzalloc(sizeof(struct device_d) + sizeof(struct linux_console_data));

	data = (struct linux_console_data *)(dev + 1);

	dev->platform_data = data;
	strcpy(dev->name, name);

	strcpy(dev->name, "console");

	if (stdinfd >= 0)
		data->flags = CONSOLE_STDIN;
	if (stdoutfd >= 0)
		data->flags |= CONSOLE_STDOUT | CONSOLE_STDERR;

	data->stdoutfd = stdoutfd;
	data->stdinfd  = stdinfd;

	return register_device(dev);
}

