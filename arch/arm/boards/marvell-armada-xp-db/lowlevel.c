// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright
 * (C) 2019 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/lowlevel.h>
#include <io.h>

extern char __dtb_armada_xp_db_bb_start[];

ENTRY_FUNCTION(start_marvell_armada_xp_db, r0, r1, r2)
{
	void *fdt;
	uint32_t reg;
	void __iomem *base = mvebu_get_initial_int_reg_base();

	arm_cpu_lowlevel_init();

	/* enable L2 parity and ECC */
#define L2_AUX_CONTRL_ADDRESS (base + 0x8104)
	reg = readl(L2_AUX_CONTRL_ADDRESS);
	reg |= 3 << 20;
	writel(0x1a09ef00, L2_AUX_CONTRL_ADDRESS);

	fdt = __dtb_armada_xp_db_bb_start + get_runtime_offset();

	armada_370_xp_barebox_entry(fdt);
}
