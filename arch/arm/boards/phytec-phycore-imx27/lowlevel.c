/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <init.h>
#include <mach/imx27-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/barebox-arm-head.h>

#include "pll.h"

#define ESDCTL0_VAL (ESDCTL0_SDE | ESDCTL0_ROW13 | ESDCTL0_COL10)

static void __bare_init __naked noinline phytec_phycorce_imx27_common_init(void *fdt)
{
	uint32_t r;
	int i;

	arm_cpu_lowlevel_init();

	/* ahb lite ip interface */
	writel(0x20040304, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR0);
	writel(0xDFFBFCFB, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR1);
	writel(0x00000000, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR0);
	writel(0xFFFFFFFF, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR1);

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0xa0000000 && r < 0xb0000000)
		goto out;

	/* re-program the PLL prior(!) starting the SDRAM controller */
	writel(MPCTL0_VAL, MX27_CCM_BASE_ADDR + MX27_MPCTL0);
	writel(SPCTL0_VAL, MX27_CCM_BASE_ADDR + MX27_SPCTL0);
	writel(CSCR_VAL | MX27_CSCR_UPDATE_DIS | MX27_CSCR_MPLL_RESTART |
		MX27_CSCR_SPLL_RESTART, MX27_CCM_BASE_ADDR + MX27_CSCR);

	/*
	 * DDR on CSD0
	 */
	/* Enable DDR SDRAM operation */
	writel(0x00000008, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC);

	/* Set the driving strength   */
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(3));
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(5));
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(6));
	writel(0x00005005, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(7));
	writel(0x15555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(8));

	/* Initial reset */
	writel(0x00000004, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC);
	writel(0x006ac73a, MX27_ESDCTL_BASE_ADDR + IMX_ESDCFG0);

	/* precharge CSD0 all banks */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0x00000000, 0xA0000F00);	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	for (i = 0; i < 8; i++)
		writel(0, 0xa0000f00);

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	writeb(0xda, 0xa0000033);
	writeb(0xff, 0xa1000000);
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 |
			ESDCTL0_BL | ESDCTL0_SMODE_NORMAL,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	if (IS_ENABLED(CONFIG_ARCH_IMX_EXTERNAL_BOOT_NAND))
		imx27_barebox_boot_nand_external(fdt);

out:
	imx27_barebox_entry(fdt);
}

extern char __dtb_imx27_phytec_phycore_rdk_start[];

ENTRY_FUNCTION(start_phytec_phycore_imx27, r0, r1, r2)
{
	void *fdt;

	arm_setup_stack(MX27_IRAM_BASE_ADDR + MX27_IRAM_SIZE - 12);

	fdt = __dtb_imx27_phytec_phycore_rdk_start + get_runtime_offset();

	phytec_phycorce_imx27_common_init(fdt);
}
