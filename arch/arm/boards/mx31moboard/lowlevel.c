/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (c) 2014 EPFL, Philippe Rétornaz <philippe.retornaz@epfl.ch>
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
#include <io.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <asm-generic/sections.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx31-regs.h>
#include <mach/imx-pll.h>
#include <asm/barebox-arm-head.h>
#include <mach/esdctl.h>

static noinline __noreturn void mx31moboard_startup(void)
{
	uint32_t r;
	volatile int c;

	/* Enable IPU Display interface */
	writel(1 << 6, MX31_IPU_CTRL_BASE_ADDR);

	writel(0x074B0BF5, MX31_CCM_BASE_ADDR + MX31_CCM_CCMR);

	for (c = 0; c < 0x4000; c++) ;

	writel(0x074B0BF5 | MX31_CCMR_MPE, MX31_CCM_BASE_ADDR +
			MX31_CCM_CCMR);
	writel((0x074B0BF5 | MX31_CCMR_MPE) & ~MX31_CCMR_MDS,
			MX31_CCM_BASE_ADDR + MX31_CCM_CCMR);

	writel(MX31_PDR0_CSI_PODF(0x1ff) | \
		MX31_PDR0_PER_PODF(7) | \
		MX31_PDR0_HSP_PODF(3) | \
		MX31_PDR0_NFC_PODF(5) | \
		MX31_PDR0_IPG_PODF(1) | \
		MX31_PDR0_MAX_PODF(3) | \
		MX31_PDR0_MCU_PODF(0), \
		MX31_CCM_BASE_ADDR + MX31_CCM_PDR0);

	writel(IMX_PLL_PD(0) | IMX_PLL_MFD(0x33) |
			IMX_PLL_MFI(0xa) | IMX_PLL_MFN(0x0C),
			MX31_CCM_BASE_ADDR + MX31_CCM_MPCTL);
	writel(IMX_PLL_PD(1) | IMX_PLL_MFD(0x43) | IMX_PLL_MFI(12) |
			IMX_PLL_MFN(1), MX31_CCM_BASE_ADDR +
			MX31_CCM_SPCTL);

	/*
	 * Configure IOMUXC
	 * Clears 0x43fa_c26c - 0x43fa_c2dc with 0,
	 * except 0x43fa_c278 (untouched), 0x43fa_c27c (set to 0x1000)
	 * and 0x43fa_c280 (untouched)
	 * (behaviour copied by sha, source unknown)
	 */
	writel(0, 0x43fac26c); /* SDCLK */
	writel(0, 0x43fac270); /* CAS */
	writel(0, 0x43fac274); /* RAS */

	writel(0x1000, 0x43fac27c); /* CSD0 */

	/* DQM3, DQM2, DQM1, DQM0, SD31-SD0, A25-A0, MA10 */
	for (r = 0x43fac284; r <= 0x43fac2dc; r += 4)
		writel(0, r);

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0x80000000 && r < 0xa0000000)
		imx31_barebox_entry(NULL);

	writel(0x00000004, MX31_ESDCTL_BASE_ADDR + IMX_ESDMISC);
	writel(0x00695727, MX31_ESDCTL_BASE_ADDR + IMX_ESDCFG0);
	writel(0x92100000, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0x12344321, MX31_CSD0_BASE_ADDR + 0xf00);
	writel(0xa2100000, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0x12344321, MX31_CSD0_BASE_ADDR);
	writel(0x12344321, MX31_CSD0_BASE_ADDR);
	writel(0xb2100000, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(0xda, MX31_CSD0_BASE_ADDR + 0x33);
	writeb(0xff, MX31_CSD0_BASE_ADDR + 0x01000000);
	writel(0x82226080, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0xDEADBEEF, MX31_CSD0_BASE_ADDR);
	writel(0x0000000c, MX31_ESDCTL_BASE_ADDR + IMX_ESDMISC);

	imx31_barebox_entry(NULL);

}

void __bare_init __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();

	/* Temporary stack location in internal SRAM */
	arm_setup_stack(MX31_IRAM_BASE_ADDR + MX31_IRAM_SIZE - 8);

	mx31moboard_startup();
}
