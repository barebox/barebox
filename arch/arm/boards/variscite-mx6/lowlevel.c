/*
 *
 * Copyright (C) 2013 Michael Burkey
 * Based on code by Sascha Hauer <s.hauer@pengutronix.de>
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
#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx6.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x03, iomuxbase + 0x0280);
	writel(0x03, iomuxbase + 0x0284);

	imx6_ungate_all_peripherals();
	imx6_uart_setup_ll();

	putc_ll('>');
}
extern char __dtb_imx6q_var_custom_start[];

ENTRY_FUNCTION(start_variscite_custom, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	if (IS_ENABLED(CONFIG_DEBUG_LL))
	    setup_uart();

	fdt = __dtb_imx6q_var_custom_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
