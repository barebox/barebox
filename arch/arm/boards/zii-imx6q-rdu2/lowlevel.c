/*
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <asm/barebox-arm.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	imx6_ungate_all_peripherals();

	writel(0x1b0b1, iomuxbase + 0x0650);
	writel(3, iomuxbase + 0x0280);

	writel(0x1b0b1, iomuxbase + 0x0654);
	writel(3, iomuxbase + 0x0284);
	writel(1, iomuxbase + 0x0920);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6q_zii_rdu2_start[];
extern char __dtb_imx6qp_zii_rdu2_start[];

ENTRY_FUNCTION(start_imx6q_zii_rdu2, r0, r1, r2)
{
	void *fdt = __dtb_imx6q_zii_rdu2_start;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	imx6q_barebox_entry(fdt - get_runtime_offset());
}

ENTRY_FUNCTION(start_imx6qp_zii_rdu2, r0, r1, r2)
{
	void *fdt = __dtb_imx6qp_zii_rdu2_start;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	imx6q_barebox_entry(fdt - get_runtime_offset());
}
