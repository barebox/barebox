/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
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
#include <linux/clkdev.h>
#include <asm/hardware.h>
#include <mach/hardware.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <linux/amba/bus.h>

#include "clock.h"

static struct clk st8815_clk_48 = {
       .rate = 48 * 1000 * 1000,
};

static struct clk st8815_clk_2_4 = {
       .rate = 2400000,
};

static struct clk st8815_dummy;

void st8815_add_device_sdram(u32 size)
{
	arm_add_mem_device("ram0", 0x00000000, size);
}

static struct clk_lookup clocks_lookups[] = {
	CLKDEV_CON_ID("apb_pclk", &st8815_dummy),
	CLKDEV_CON_ID("nomadik_mtu", &st8815_clk_2_4),
	CLKDEV_DEV_ID("uart-pl0110", &st8815_clk_48),
	CLKDEV_DEV_ID("uart-pl0111", &st8815_clk_48),
};

static int st8815_clkdev_init(void)
{
	clkdev_add_table(clocks_lookups, ARRAY_SIZE(clocks_lookups));

	return 0;
}
postcore_initcall(st8815_clkdev_init);

void st8815_register_uart(unsigned id)
{
	resource_size_t start;

	switch (id) {
	case 0:
		start = NOMADIK_UART1_BASE;
		break;
	case 1:
		start = NOMADIK_UART1_BASE;
		break;
	}
	amba_apb_device_add(NULL, "uart-pl011", id, start, 4096, NULL, 0);
}
