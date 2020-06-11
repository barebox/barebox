// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2015 Uwe Kleine-König <kernel@pengutronix.de>, Pengutronix

#include <common.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/io.h>
#include <mach/lowlevel.h>
#include <mach/common.h>

extern char __dtb_armada_xp_rn2120_bb_start[];

ENTRY_FUNCTION(start_netgear_rn2120, r0, r1, r2)
{
	void *fdt;
	void __iomem *base = mvebu_get_initial_int_reg_base();

	arm_cpu_lowlevel_init();

	/*
	 * This is necessary to allow the machine to draw more power. Probably
	 * connected to a TI TPS65251. Without this resetting a phy makes the
	 * SoC reset.
	 * This is effectively gpio_direction_output(42, 1);
	 */
	writel((1 << 10) | readl(base + 0x18140), base + 0x18140);
	writel(~(1 << 10) & readl(base + 0x18144), base + 0x18144);

	/*
	 * The vendor binary that initializes RAM doesn't program the SDRAM
	 * Address Decoding registers to match the actual available RAM. But as
	 * barebox later determines the RAM size from these, fix them up here.
	 */

	/* Win 1 Base Address Register: base=0x40000000 */
	writel(0x40000000, base + 0x20188);
	/* Win 1 Control Register: size=0x4000000, wincs=1, en=1*/
	writel(0x3fffffe5, base + 0x2018c);

	/* Win 0 Base Address Register is already 0, base=0x00000000 */
	/* Win 0 Control Register: size=0x4000000, wincs=0, en=1 */
	writel(0x3fffffe1, base + 0x20184);

	fdt = __dtb_armada_xp_rn2120_bb_start +
		get_runtime_offset();

	armada_370_xp_barebox_entry(fdt);
}
