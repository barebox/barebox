/*
 * Copyright (C) 2015 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <linux/sizes.h>
#include <io.h>
#include <debug_ll.h>
#include <asm/sections.h>
#include <asm/mmu.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/generic.h>

static void setup_uart(void)
{
	void __iomem *uart_base = (void *)0x02020000;

	writel(0x1, 0x020e0330);
	writel(0x00000000, uart_base + 0x80);
	writel(0x00004027, uart_base + 0x84);
	writel(0x00000704, uart_base + 0x88);
	writel(0x00000a81, uart_base + 0x90);
	writel(0x0000002b, uart_base + 0x9c);
	writel(0x00013880, uart_base + 0xb0);
	writel(0x0000047f, uart_base + 0xa4);
	writel(0x0000c34f, uart_base + 0xa8);
	writel(0x00000001, uart_base + 0x80);
	putc_ll('>');
}

extern char __dtb_imx6dl_eltec_hipercam_start[];

ENTRY_FUNCTION(start_imx6dl_eltec_hipercam, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00940000 - 8);
	setup_uart();

	fdt = __dtb_imx6dl_eltec_hipercam_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_256M, fdt);
}
