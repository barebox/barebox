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
#include <mach/imx35-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <asm/cache-l2x0.h>
#include <io.h>
#include <mach/imx-nand.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/sections.h>
#include <asm-generic/memory_layout.h>
#include <asm/system.h>

/* Assuming 24MHz input clock */
#define MPCTL_PARAM_399     (IMX_PLL_PD(0) | IMX_PLL_MFD(15) | IMX_PLL_MFI(8) | IMX_PLL_MFN(5))
#define MPCTL_PARAM_532     ((1 << 31) | IMX_PLL_PD(0) | IMX_PLL_MFD(11) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))
#define PPCTL_PARAM_300     (IMX_PLL_PD(0) | IMX_PLL_MFD(3) | IMX_PLL_MFI(6) | IMX_PLL_MFN(1))

#define SDRAM_MODE_BL_8	0x0003
#define SDRAM_MODE_BSEQ	0x0000
#define SDRAM_MODE_CL_3	0x0030
#define MDDR_DS_HALF	0x20
#define SDRAM_COMPARE_CONST1	0x55555555
#define SDRAM_COMPARE_CONST2	0xaaaaaaaa

static void __bare_init noinline setup_sdram(u32 memsize, u32 mode, u32 sdram_addr)
{
	volatile int loop;
	void *r9 = (void *)MX35_CSD0_BASE_ADDR;
	u32 r11 = 0xda; /* dummy constant */
	u32 r1, r0;

	/* disable second SDRAM region to save power */
	r1 = readl(MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL1);
	r1 &= ~ESDCTL0_SDE;
	writel(r1, MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL1);

	mode |= ESDMISC_RST | ESDMISC_MDDR_DL_RST;
	writel(mode, MX35_ESDCTL_BASE_ADDR + IMX_ESDMISC);

	mode &= ~(ESDMISC_RST | ESDMISC_MDDR_DL_RST);
	writel(mode, MX35_ESDCTL_BASE_ADDR + IMX_ESDMISC);

	/* wait for esdctl reset */
	for (loop = 0; loop < 0x20000; loop++);

	r1 = ESDCFGx_tXP_4 | ESDCFGx_tWTR_1 |
		ESDCFGx_tRP_3 | ESDCFGx_tMRD_2 |
		ESDCFGx_tWR_1_2 | ESDCFGx_tRAS_6 |
		ESDCFGx_tRRD_2 | ESDCFGx_tCAS_3 |
		ESDCFGx_tRCD_3 | ESDCFGx_tRC_20;

	writel(r1, MX35_ESDCTL_BASE_ADDR + IMX_ESDCFG0);

	/* enable SDRAM controller */
	writel(memsize | ESDCTL0_SMODE_NORMAL,
			MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	/* Micron Datasheet Initialization Step 3: Wait 200us before first command */
	for (loop = 0; loop < 1000; loop++);

	/* Micron Datasheet Initialization Step 4: PRE CHARGE ALL */
	writel(memsize | ESDCTL0_SMODE_PRECHARGE,
			MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(r11, sdram_addr);

	/* Micron Datasheet Initialization Step 5: NOP for tRP (at least 22.5ns)
	 * The CPU is not fast enough to cause a problem here
	 */

	/* Micron Datasheet Initialization Step 6: 2 AUTO REFRESH and tRFC NOP
	 * (at least 140ns)
	 */
	writel(memsize | ESDCTL0_SMODE_AUTO_REFRESH,
			MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(r11, r9); /* AUTO REFRESH #1 */

	for (loop = 0; loop < 3; loop++); /* ~140ns delay at 532MHz */

	writeb(r11, r9); /* AUTO REFRESH #2 */

	for (loop = 0; loop < 3; loop++); /* ~140ns delay at 532MHz */

	/* Micron Datasheet Initialization Step 7: LOAD MODE REGISTER */
	writel(memsize | ESDCTL0_SMODE_LOAD_MODE,
			MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	writeb(r11, r9 + (SDRAM_MODE_BL_8 | SDRAM_MODE_BSEQ | SDRAM_MODE_CL_3));

	/* Micron Datasheet Initialization Step 8: tMRD = 2 tCK NOP
	 * (The memory controller will take care of this delay)
	 */

	/* Micron Datasheet Initialization Step 9: LOAD MODE REGISTER EXTENDED */
	writeb(r11, 0x84000000 | MDDR_DS_HALF);  /*we assume 14 Rows / 10 Cols here */

	/* Micron Datasheet Initialization Step 9: tMRD = 2 tCK NOP
	 * (The memory controller will take care of this delay)
	 */

	/* Now configure SDRAM-Controller and check that it works */
	writel(memsize | ESDCTL0_BL | ESDCTL0_REF4,
			MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);

	/* Freescale asks for first access to be a write to properly
	 * initialize DQS pin-state and keepers
	 */
	writel(0xdeadbeef, r9);

	/* test that the RAM is in fact working */
	writel(SDRAM_COMPARE_CONST1, r9);
	writel(SDRAM_COMPARE_CONST2, r9 + 0x4);

	if (readl(r9) != SDRAM_COMPARE_CONST1)
		while (1);

	/* Verify that the correct row and coloumn is selected */

	/* So far we asssumed that we have 14 rows, verify this */
	writel(SDRAM_COMPARE_CONST1, r9);
	writel(SDRAM_COMPARE_CONST2, r9 + (1 << 25));

	/* if both value are identical, we don't have 14 rows. assume 13 instead */
	if (readl(r9) == readl(r9 + (1 << 25))) {
		r0 = readl(MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
		r0 &= ~ESDCTL0_ROW_MASK;
		r0 |= ESDCTL0_ROW13;
		writel(r0, MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	}

	/* So far we asssumed that we have 10 columns, verify this */
	writel(SDRAM_COMPARE_CONST1, r9);
	writel(SDRAM_COMPARE_CONST2, r9 + (1 << 11));

	/* if both value are identical, we don't have 10 cols. assume 9 instead */
	if (readl(r9) == readl(r9 + (1 << 11))) {
		r0 = readl(MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
		r0 &= ~ESDCTL0_COL_MASK;
		r0 |= ESDCTL0_COL9;
		writel(r0, MX35_ESDCTL_BASE_ADDR + IMX_ESDCTL0);
	}
}

#define BRANCH_PREDICTION_ENABLE
#define UNALIGNED_ACCESS_ENABLE
#define LOW_INT_LATENCY_ENABLE

void __bare_init __naked barebox_arm_reset_vector(void)
{
	u32 r0, r1;
	void *iomuxc_base = (void *)MX35_IOMUXC_BASE_ADDR;
	int i;

	arm_cpu_lowlevel_init();

	arm_setup_stack(0x10000000 + 128 * 1024 - 16);

	/*
	 *       ARM1136 init
	 *       - invalidate I/D cache/TLB and drain write buffer;
	 *       - invalidate L2 cache
	 *       - unaligned access
	 *       - branch predictions
	 */
#ifdef TURN_OFF_IMPRECISE_ABORT
	__asm__ __volatile__("mrs %0, cpsr":"=r"(r0));
	r0 &= ~0x100;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r0));
#endif
	/* ensure L1 caches and MMU are turned-off for now */
	r1 = get_cr();
	r1 &= ~(CR_I | CR_M | CR_C);

	/* setup core features */
	__asm__ __volatile__("mrc p15, 0, r0, c1, c0, 1":"=r"(r0));
#ifdef BRANCH_PREDICTION_ENABLE
	r0 |= 7;
	r1 |= CR_Z;
#else
	r0 &= ~7;
	r1 &= ~CR_Z;
#endif
	__asm__ __volatile__("mcr p15, 0, r0, c1, c0, 1" : : "r"(r0));

#ifdef UNALIGNED_ACCESS_ENABLE
	r1 |= CR_U;
#else
	r1 &= ~CR_U;
#endif

#ifdef LOW_INT_LATENCY_ENABLE
	r1 |= CR_FI;
#else
	r1 &= ~CR_FI;
#endif
	set_cr(r1);

	r0 = 0;
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 6" : : "r"(r0));

	/* invalidate I cache and D cache */
	__asm__ __volatile__("mcr p15, 0, r0, c7, c7, 0" : : "r"(r0));
	/* invalidate TLBs */
	__asm__ __volatile__("mcr p15, 0, r0, c8, c7, 0" : : "r"(r0));
	/* Drain the write buffer */
	__asm__ __volatile__("mcr p15, 0, r0, c7, c10, 4" : : "r"(r0));

	/* Also setup the Peripheral Port Remap register inside the core */
	r0 = 0x40000015; /* start from AIPS 2GB region */
	__asm__ __volatile__("mcr p15, 0, r0, c15, c2, 4" : : "r"(r0));

#define	WDOG_WMCR 0x8
	/* silence reset WDOG */
	writew(0, MX35_WDOG_BASE_ADDR + WDOG_WMCR);

	/* Skip SDRAM initialization if we run from RAM */
	r0 = get_pc();
	if (r0 > 0x80000000 && r0 < 0x90000000)
		goto out;

	/* Configure drive strength */

	/* Configure DDR-pins to correct mode */
	r0 = 0x00001800;
	writel(r0, iomuxc_base + 0x794);
	writel(r0, iomuxc_base + 0x798);
	writel(r0, iomuxc_base + 0x79c);
	writel(r0, iomuxc_base + 0x7a0);
	writel(r0, iomuxc_base + 0x7a4);

	/* Set drive strength for DDR-pins */
	for (i = 0x368; i <= 0x4c8; i += 4) {
		r0 = readl(iomuxc_base + i);
		r0 &= ~0x6;
		r0 |= 0x2;
		writel(r0, iomuxc_base + i);
		if (i == 0x468)
			i = 0x4a4;
	}

	r0 = readl(iomuxc_base + 0x480);
	r0 &= ~0x6;
	r0 |= 0x2;
	writel(r0, iomuxc_base + 0x480);

	r0 = readl(iomuxc_base + 0x4b8);
	r0 &= ~0x6;
	r0 |= 0x2;
	writel(r0, iomuxc_base + 0x4b8);

	/* Configure static chip-selects */
	r0 = readl(iomuxc_base + 0x000);
	r0 &= ~1; /* configure CS2/CSD0 for SDRAM */
	writel(r0, iomuxc_base + 0x000);

	/* start-up code doesn't need any static chip-select.
	 * Leave their initialization to high-level code that
	 * can initialize them depending on the baseboard.
	 */

	/* Configure clocks */

	/* setup cpu/bus clocks */
	writel(0x003f4208, MX35_CCM_BASE_ADDR + MX35_CCM_CCMR);

	/* configure MPLL */
	writel(MPCTL_PARAM_532, MX35_CCM_BASE_ADDR + MX35_CCM_MPCTL);

	/* configure PPLL */
	writel(PPCTL_PARAM_300, MX35_CCM_BASE_ADDR + MX35_CCM_PPCTL);

	/* configure core dividers */
	r0 = MX35_PDR0_CCM_PER_AHB(1) | MX35_PDR0_HSP_PODF(2);

	writel(r0, MX35_CCM_BASE_ADDR + MX35_CCM_PDR0);

	/* configure clock-gates */
	r0 = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR0);
	r0 |= 0x00300000;
	writel(r0, MX35_CCM_BASE_ADDR + MX35_CCM_CGR0);

	r0 = readl(MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);
	r0 |= 0x00000c03;
	writel(r0, MX35_CCM_BASE_ADDR + MX35_CCM_CGR1);

	/* Configure SDRAM */
	/* Try 32-Bit 256 MB DDR memory */
	r0 = ESDCTL0_SDE | ESDCTL0_ROW14 | ESDCTL0_COL10 | ESDCTL0_DSIZ_31_0; /* 1024 MBit DDR-SDRAM */
	setup_sdram(r0, ESDMISC_MDDR_EN, 0x80000f00);

#ifdef CONFIG_NAND_IMX_BOOT
	/* Speed up NAND controller by adjusting the NFC divider */
	r0 = readl(MX35_CCM_BASE_ADDR + MX35_CCM_PDR4);
	r0 &= ~(0xf << 28);
	r0 |= 0x1 << 28;
	writel(r0, MX35_CCM_BASE_ADDR + MX35_CCM_PDR4);

	/* setup a stack to be able to call imx35_barebox_boot_nand_external() */
	arm_setup_stack(MX35_IRAM_BASE_ADDR + MX35_IRAM_SIZE - 8);

	imx35_barebox_boot_nand_external();
#endif
out:
	imx35_barebox_entry(0);
}
