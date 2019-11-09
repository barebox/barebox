/*
 * Copyright (C) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
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
 */

#include <asm/armlinux.h>
#include <common.h>
#include <environment.h>
#include <generated/mach-types.h>
#include <init.h>
#include <mach/zynq7000-regs.h>
#include <linux/sizes.h>


static int zedboard_console_init(void)
{
	barebox_set_hostname("zedboard");

	return 0;
}
console_initcall(zedboard_console_init);
