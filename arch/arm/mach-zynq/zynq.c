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
#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/zynq7000-regs.h>
#include <restart.h>

static void __noreturn zynq_restart_soc(struct restart_handler *rst)
{
	/* write unlock key to slcr */
	writel(0xDF0D, ZYNQ_SLCR_UNLOCK);
	/* reset */
	writel(0x1, ZYNQ_PSS_RST_CTRL);

	hang();
}

static enum bootsource zynq_bootsource_get(void)
{
	u32 boot_mode = readl(ZYNQ_SLCR_BOOT_MODE);

	switch (boot_mode & 0x7) {
	case 0x0:
		return BOOTSOURCE_JTAG;
	case 0x1:
		return BOOTSOURCE_SPI;
	case 0x2:
		return BOOTSOURCE_NOR;
	case 0x4:
		return BOOTSOURCE_NAND;
	case 0x5:
		return BOOTSOURCE_MMC;
	default:
		return BOOTSOURCE_UNKNOWN;
	}
}

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

	restart_handler_register_fn(zynq_restart_soc);

	bootsource_set(zynq_bootsource_get());

	return 0;
}
postcore_initcall(zynq_init);
