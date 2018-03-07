/*
 * Copyright (C) 2014 Sascha Hauer, Pengutronix
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

#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	imx6_ungate_all_peripherals();

	writel(0x0, iomuxbase + 0x24);
	writel(0x1b0b1, iomuxbase + 0x036C);
	writel(0x0, iomuxbase + 0x28);
	writel(0x1b0b1, iomuxbase + 0x0370);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6sx_sdb_start[];

ENTRY_FUNCTION(start_imx6sx_sabresdb, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6sx_sdb_start + get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_1G, fdt);
}
