/*
 * (c) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>
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

#include <asm/system.h>
#include <asm-generic/io.h>
#include <common.h>
#include <init.h>
#include <mach/zynq7000-regs.h>

static int zynq_init(void)
{
	u32 val;

	dsb();
	isb();
	writel(0xDF0D, ZYNQ_SLCR_UNLOCK);
	/* remap ocm high */
	writel(0x0000000F, 0xf8000910);
	/* mpcore.filtering_start_address */
	writel(0x00000000, 0xf8f00040);
	/* mpcore.filtering_end_address */
	writel(0xffe00000, 0xf8f00044);
	val = readl(0xf8f00000);
	val |= 0x2;
	writel(val, 0xf8f00000);
	dmb();

	add_generic_device("zynq-clock", 0, NULL, ZYNQ_SLCR_BASE, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("smp_twd", 0, NULL, CORTEXA9_SCU_TIMER_BASE_ADDR,
				0x4000, IORESOURCE_MEM, NULL);
	return 0;
}
postcore_initcall(zynq_init);

void __noreturn reset_cpu(unsigned long addr)
{
	/* write unlock key to slcr */
	writel(0xDF0D, ZYNQ_SLCR_UNLOCK);
	/* reset */
	writel(0x1, ZYNQ_PSS_RST_CTRL);

	while (1)
		;
}
