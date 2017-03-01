/*
 * Author: Carlo Caione <carlo@carlocaione.org>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
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

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <io.h>
#include <asm/armlinux.h>
#include <linux/sizes.h>

#include <mach/platform.h>
#include <mach/core.h>
#include <linux/amba/bus.h>

static int bcm2835_clk_init(void)
{
	struct clk *clk;

	clk = clk_fixed("apb_pclk", 0);
	clk_register_clkdev(clk, "apb_pclk", NULL);

	clk = clk_fixed("uart0-pl0110", 3 * 1000 * 1000);
	clk_register_clkdev(clk, NULL, "uart0-pl0110");
	clk_register_clkdev(clk, NULL, "20201000.serial");

	clk = clk_fixed("bcm2835-cs", 1 * 1000 * 1000);
	clk_register_clkdev(clk, NULL, "bcm2835-cs");

	return 0;
}
postcore_initcall(bcm2835_clk_init);

void bcm2835_add_device_sdram(u32 size)
{
	if (!size)
		size = get_ram_size((ulong *) BCM2835_SDRAM_BASE, SZ_128M);

	arm_add_mem_device("ram0", BCM2835_SDRAM_BASE, size);
}
