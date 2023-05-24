/*
 * Copyright (C) 2010 B Labs Ltd,
 * http://l4dev.org
 * Author: Alexey Zaytsev <alexey.zaytsev@gmail.com>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Copyright (C) 1999 - 2003 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <debug_ll.h>
#include <restart.h>
#include <linux/sizes.h>

#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/amba/bus.h>

#include <io.h>
#include <asm/armlinux.h>

#include <mach/versatile/platform.h>

static void __noreturn versatile_reset_soc(struct restart_handler *rst)
{
	u32 val;

	val = __raw_readl(VERSATILE_SYS_RESETCTL) & ~0x7;
	val |= 0x105;

	__raw_writel(0xa05f, VERSATILE_SYS_LOCK);
	__raw_writel(val, VERSATILE_SYS_RESETCTL);
	__raw_writel(0, VERSATILE_SYS_LOCK);

	hang();
}

static int versatile_init(void)
{
	if (!of_machine_is_compatible("arm,versatile-pb") &&
	    !of_machine_is_compatible("arm,versatile-ab"))
		return 0;

	restart_handler_register_fn("soc", versatile_reset_soc);
	return 0;
}
core_initcall(versatile_init);
