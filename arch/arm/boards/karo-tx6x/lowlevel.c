/*
 * Copyright (C) 2014 Steffen Trumtrar, Pengutronix
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
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <image-metadata.h>
#include <mach/generic.h>
#include <sizes.h>

static inline void setup_uart(void)
{
	void __iomem *ccmbase = (void *)MX6_CCM_BASE_ADDR;
	void __iomem *uartbase = (void *)MX6_UART1_BASE_ADDR;
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x1, iomuxbase + 0x0314);
	writel(0x1, iomuxbase + 0x0318);
	writel(0x1, iomuxbase + 0x0330);
	writel(0x1, iomuxbase + 0x032c);

	writel(0xffffffff, ccmbase + 0x68);
	writel(0xffffffff, ccmbase + 0x6c);
	writel(0xffffffff, ccmbase + 0x70);
	writel(0xffffffff, ccmbase + 0x74);
	writel(0xffffffff, ccmbase + 0x78);
	writel(0xffffffff, ccmbase + 0x7c);
	writel(0xffffffff, ccmbase + 0x80);

	writel(0x00000000, uartbase + 0x80);
	writel(0x00004027, uartbase + 0x84);
	writel(0x00000784, uartbase + 0x88);
	writel(0x00000a81, uartbase + 0x90);
	writel(0x0000002b, uartbase + 0x9c);
	writel(0x0001b0b0, uartbase + 0xb0);
	writel(0x0000047f, uartbase + 0xa4);
	writel(0x0000c34f, uartbase + 0xa8);
	writel(0x00000001, uartbase + 0x80);

	putc_ll('>');
}

extern char __dtb_imx6dl_tx6u_801x_start[];

BAREBOX_IMD_TAG_STRING(tx6x_mx6_memsize_1G, IMD_TYPE_PARAMETER, "memsize=1024", 0);

ENTRY_FUNCTION(start_imx6dl_tx6x_1g, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	IMD_USED(tx6x_mx6_memsize_1G);

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6dl_tx6u_801x_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
