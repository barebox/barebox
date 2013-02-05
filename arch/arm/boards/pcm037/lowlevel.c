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
#include <io.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <asm-generic/sections.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx31-regs.h>
#include <mach/imx-pll.h>
#include <asm/barebox-arm-head.h>
#include <mach/esdctl.h>

#define ESDCTL0_VAL (ESDCTL0_SDE | ESDCTL0_ROW13 | ESDCTL0_COL10)

void __bare_init __naked barebox_arm_reset_vector(void)
{
	uint32_t r;
	volatile int v;

	arm_cpu_lowlevel_init();

	writel(1 << 6, MX31_IPU_CTRL_BASE_ADDR);

	writel(0x074B0BF5, MX31_CCM_BASE_ADDR + MX31_CCM_CCMR);

	for (v = 0; v < 0x4000; v++);

	writel(0x074B0BF5 | MX31_CCMR_MPE, MX31_CCM_BASE_ADDR +
			MX31_CCM_CCMR);
	writel((0x074B0BF5 | MX31_CCMR_MPE) & ~MX31_CCMR_MDS,
			MX31_CCM_BASE_ADDR + MX31_CCM_CCMR);

	writel(MX31_PDR0_CSI_PODF(0xff1) | \
		MX31_PDR0_PER_PODF(7) | \
		MX31_PDR0_HSP_PODF(3) | \
		MX31_PDR0_NFC_PODF(5) | \
		MX31_PDR0_IPG_PODF(1) | \
		MX31_PDR0_MAX_PODF(3) | \
		MX31_PDR0_MCU_PODF(0), \
		MX31_CCM_BASE_ADDR + MX31_CCM_PDR0);

	writel(IMX_PLL_PD(0) | IMX_PLL_MFD(0xe) |
			IMX_PLL_MFI(9) | IMX_PLL_MFN(0xd),
			MX31_CCM_BASE_ADDR + MX31_CCM_MPCTL);
	writel(IMX_PLL_PD(1) | IMX_PLL_MFD(0x43) | IMX_PLL_MFI(12) |
			IMX_PLL_MFN(1), MX31_CCM_BASE_ADDR +
			MX31_CCM_SPCTL);

	/*
	 * Configure IOMUXC
	 * Clears 0x43fa_c26c - 0x43fa_c2dc with 0, except 0x43fa_c278 (untouched),
	 * 0x43fa_c27c (set to 0x1000) and 0x43fa_c280 (untouched)
	 * (behaviour copied by sha, source unknown)
	 */
	writel(0, 0x43fac26c);
	writel(0, 0x43fac270);
	writel(0, 0x43fac274);

	writel(0x1000, 0x43fac27c);

	for (r = 0x43fac284; r <= 0x43fac2dc; r += 4)
		writel(0, r);

	/* Skip SDRAM initialization if we run from RAM */
	r = get_pc();
	if (r > 0x80000000 && r < 0xa0000000)
		imx31_barebox_entry(0);

#if defined CONFIG_PCM037_SDRAM_BANK0_128MB
#define ROWS0	ESDCTL0_ROW13
#elif defined CONFIG_PCM037_SDRAM_BANK0_256MB
#define ROWS0	ESDCTL0_ROW14
#endif
	writel(0x00000004, MX31_ESDCTL_BASE_ADDR + IMX_ESDMISC);
	writel(0x006ac73a, MX31_ESDCTL_BASE_ADDR + IMX_ESDCFG0);
	writel(0x90100000 | ROWS0, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0x12344321, MX31_CSD0_BASE_ADDR + 0xf00);
	writel(0xa0100000 | ROWS0, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0x12344321, MX31_CSD0_BASE_ADDR);
	writel(0x12344321, MX31_CSD0_BASE_ADDR);
	writel(0xb0100000 | ROWS0, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(0xda, MX31_CSD0_BASE_ADDR + 0x33);
	writeb(0xff, MX31_CSD0_BASE_ADDR + 0x01000000);
	writel(0x80226080 | ROWS0, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writel(0xDEADBEEF, MX31_CSD0_BASE_ADDR);
	writel(0x0000000c, MX31_ESDCTL_BASE_ADDR + IMX_ESDMISC);

#ifndef CONFIG_PCM037_SDRAM_BANK1_NONE
#if defined CONFIG_PCM037_SDRAM_BANK1_128MB
#define ROWS1	ESDCTL0_ROW13
#elif defined CONFIG_PCM037_SDRAM_BANK1_256MB
#define ROWS1	ESDCTL0_ROW14
#endif
	writel(0x006ac73a, MX31_ESDCTL_BASE_ADDR + IMX_ESDCFG1);
	writel(0x90100000 | ROWS1, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL1);
	writel(0x12344321, MX31_CSD1_BASE_ADDR + 0xf00);
	writel(0xa0100000 | ROWS1, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL1);
	writel(0x12344321, MX31_CSD1_BASE_ADDR);
	writel(0x12344321, MX31_CSD1_BASE_ADDR);
	writel(0xb0100000 | ROWS1, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL1);
	writeb(0xda, MX31_CSD1_BASE_ADDR + 0x33);
	writeb(0xff, MX31_CSD1_BASE_ADDR + 0x01000000);
	writel(0x80226080 | ROWS1, MX31_ESDCTL_BASE_ADDR + IMX_ESDCTL1);
	writel(0xDEADBEEF, MX31_CSD1_BASE_ADDR);
	writel(0x0000000c, MX31_ESDCTL_BASE_ADDR + IMX_ESDMISC);
#endif

#ifdef CONFIG_NAND_IMX_BOOT
	/* setup a stack to be able to call imx31_barebox_boot_nand_external() */
	arm_setup_stack(MX31_IRAM_BASE_ADDR + MX31_IRAM_SIZE - 12);

	imx31_barebox_boot_nand_external();
#else
	imx31_barebox_entry(0);
#endif
}
