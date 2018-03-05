/*
 * esdctl-v4.c - i.MX sdram controller functions for i.MX53
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <io.h>
#include <mach/esdctl-v4.h>
#include <mach/imx53-regs.h>
#include <asm/system.h>

void imx_esdctlv4_do_write_leveling(void)
{
	u32 val;
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);

	/* switch RAMs to write-leveling mode */

	val = ESDCTL_V4_DDR3_MR1_ODIC_RZQ7 | ESDCTL_V4_DDR3_MR1_RTTN_DIS |
		ESDCTL_V4_DDR3_MR1_AL_DISABLE | ESDCTL_V4_DDR3_MR1_WL |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_ESDSCR_CMD_CS0 | ESDCTL_V4_ESDSCR_WL_EN |
		ESDCTL_V4_DDR3_REG_MR1;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_DDR3_MR1_ODIC_RZQ7 | ESDCTL_V4_DDR3_MR1_RTTN_DIS |
		ESDCTL_V4_DDR3_MR1_AL_DISABLE |	ESDCTL_V4_DDR3_MR1_WL |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_ESDSCR_CMD_CS1 | ESDCTL_V4_ESDSCR_WL_EN |
		ESDCTL_V4_DDR3_REG_MR1;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* start write leveling */
	writel(ESDCTL_V4_WLGCR_HW_WL_EN, base + ESDCTL_V4_WLGCR);

	do {
		val = readl(base + ESDCTL_V4_WLGCR);
	} while (val & ESDCTL_V4_WLGCR_HW_WL_EN);

	val &= ESDCTL_V4_WLGCR_WL_HW_ERR3 | ESDCTL_V4_WLGCR_WL_HW_ERR2 |
		ESDCTL_V4_WLGCR_WL_HW_ERR1 | ESDCTL_V4_WLGCR_WL_HW_ERR0;

	if (val)
		hang();

	/*
	 * manual still talks about HW_WLx_CYC fields that
	 * must be updated from HW_WLx_RES field by SW, but none
	 * of these fields seem to exist. Closest equivalents
	 * are WL_CYC_DELx-fields but no RES-fields can be found.
	 */

	/* configure RAMs back to normal operation */
	val = ESDCTL_V4_DDR3_MR1_ODIC_RZQ7 | ESDCTL_V4_DDR3_MR1_RTTN_RZQ2 |
		ESDCTL_V4_DDR3_MR1_AL_DISABLE | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_ESDSCR_CMD_CS0 |
		ESDCTL_V4_DDR3_REG_MR1;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_DDR3_MR1_ODIC_RZQ7 | ESDCTL_V4_DDR3_MR1_RTTN_RZQ2 |
		ESDCTL_V4_DDR3_MR1_AL_DISABLE | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_ESDSCR_CMD_CS1 |
		ESDCTL_V4_DDR3_REG_MR1;
	writel(val, base + ESDCTL_V4_ESDSCR);
}

void imx_esdctlv4_do_dqs_gating(void)
{
	u32 val;
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);

	/* configure ESDCTL comparator to use MPR pattern */
	writel(ESDCTL_V4_PDCMPR2_MPR_FULL_CMP | ESDCTL_V4_PDCMPR2_MPR_CMP,
			base + ESDCTL_V4_PDCMPR2);

	/* pre-charge all RAM banks */
	writel(ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_CS0 |
			ESDCTL_V4_ESDSCR_CMD_PRE_ALL, base + ESDCTL_V4_ESDSCR);
	writel(ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_CS1 |
			ESDCTL_V4_ESDSCR_CMD_PRE_ALL, base + ESDCTL_V4_ESDSCR);

	/* configure RAMs to generate MPR pattern */
	writel(ESDCTL_V4_DDR3_MR3_MPR_ENABLE | ESDCTL_V4_DDR3_MR3_MPR_PATTERN |
			ESDCTL_V4_ESDSCR_CMD_CS0 | ESDCTL_V4_ESDSCR_CON_REQ |
			ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_DDR3_REG_MR3,
			base + ESDCTL_V4_ESDSCR);
	writel(ESDCTL_V4_DDR3_MR3_MPR_ENABLE | ESDCTL_V4_DDR3_MR3_MPR_PATTERN |
			ESDCTL_V4_ESDSCR_CMD_CS1 | ESDCTL_V4_ESDSCR_CON_REQ |
			ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_DDR3_REG_MR3,
			base + ESDCTL_V4_ESDSCR);

	/* start HW DQS gating */
	writel(ESDCTL_V4_ESDDGCTRL0_HW_DG_EN, base + ESDCTL_V4_DGCTRL0);

	do {
		val = readl(base + ESDCTL_V4_DGCTRL0);
	} while (val & ESDCTL_V4_ESDDGCTRL0_HW_DG_EN);

	if (val & ESDCTL_V4_ESDDGCTRL0_HW_DG_ERR)
		hang();

	/* configure RAMs back to normal operation */
	val = ESDCTL_V4_DDR3_MR3_MPR_DISABLE | ESDCTL_V4_ESDSCR_CMD_CS0 |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_DDR3_REG_MR3;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_DDR3_MR3_MPR_DISABLE | ESDCTL_V4_ESDSCR_CMD_CS1 |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_DDR3_REG_MR3;
	writel(val, base + ESDCTL_V4_ESDSCR);
}

void imx_esdctlv4_do_zq_calibration(void)
{
	u32 val;
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);

	/*
	 * configure ZQ parameters
	 * Note: TZQ_CS is only required to be 64cy by Jedec and RAM
	 * manufacturers and i.MX53TO1 provides a 64cy setting, but
	 * TO2 according to official documentation not...
	 */
	val = ESDCTL_V4_ESDZQHWC_ZQ_PARA_EN |
		(ESDCTL_V4_ESDZQHWC_128CYC << ESDCTL_V4_ESDZQHWC_TZQ_CS_SHIFT) |
		(ESDCTL_V4_ESDZQHWC_256CYC << ESDCTL_V4_ESDZQHWC_TZQ_OPER_SHIFT) |
		(ESDCTL_V4_ESDZQHWC_512CYC << ESDCTL_V4_ESDZQHWC_TZQ_INIT_SHIFT) |
		(0 << ESDCTL_V4_ESDZQHWC_ZQ_HW_PD_RES_SHIFT) |
		(0 << ESDCTL_V4_ESDZQHWC_ZQ_HW_PU_RES_SHIFT) |
		(0 << ESDCTL_V4_ESDZQHWC_ZQ_HW_PER_SHIFT) |
		ESDCTL_V4_ESDZQHWC_ZQ_MODE_BOTH_PER |
		ESDCTL_V4_ESDZQHWC_ZQ_HW_FOR;

	/* force ZQ calibration */
	writel(val, base + ESDCTL_V4_ZQHWCTRL);

	while (readl(base + ESDCTL_V4_ZQHWCTRL) & ESDCTL_V4_ESDZQHWC_ZQ_HW_FOR);
}

/*
 * start-up a DDR3 SDRAM chip
 */
void imx_esdctlv4_start_ddr3_sdram(int cs)
{
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);
	u32 val;
	u32 val_cs1;

	if (cs)
		val_cs1 = ESDCTL_V4_ESDSCR_CMD_CS1;
	else
		val_cs1 = ESDCTL_V4_ESDSCR_CMD_CS0;

	/* initialize MR2 */
	val = ESDCTL_V4_DDR3_MR2_RTTWR_OFF | ESDCTL_V4_DDR3_MR2_SRT_NORMAL |
		ESDCTL_V4_DDR3_MR2_CWL_5 | ESDCTL_V4_DDR3_MR2_PASR_1_1 |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_DDR3_REG_MR2 | val_cs1;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* initialize MR3 */
	val =  ESDCTL_V4_DDR3_MR3_MPR_DISABLE | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_DDR3_REG_MR3 | val_cs1;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* initialize MR1 */
	val = ESDCTL_V4_DDR3_MR1_ODIC_RZQ7 | ESDCTL_V4_DDR3_MR1_RTTN_RZQ2 |
		ESDCTL_V4_DDR3_MR1_AL_DISABLE | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_DDR3_REG_MR1 | val_cs1;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* initialize MR0 */
	val = ESDCTL_V4_DDR3_MR0_PPD_SLOW | ESDCTL_V4_DDR3_MR0_WR_6 |
		ESDCTL_V4_DDR3_MR0_CL_6 | ESDCTL_V4_DDR3_MR0_BL_FIXED8 |
		ESDCTL_V4_DDR3_MR0_RBT_NIBBLE | ESDCTL_V4_DDR3_DLL_RESET |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_DDR3_REG_MR0 | val_cs1;

	if (cs)
		val |= ESDCTL_V4_ESDSCR_DLL_RST1;
	else
		val |= ESDCTL_V4_ESDSCR_DLL_RST0;

	writel(val, base + ESDCTL_V4_ESDSCR);

	/* perform ZQ calibration */
	val = 0x04000000 | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_ZQCALIB_OLD;
	val |= val_cs1;
	writel(val, base + ESDCTL_V4_ESDSCR);
}

void imx_esdctlv4_do_read_delay_line_calibration(void)
{
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);
	u32 val;

	/* configure ESDCTL comparator to use MPR pattern */
	val = ESDCTL_V4_PDCMPR2_MPR_FULL_CMP | ESDCTL_V4_PDCMPR2_MPR_CMP;
	writel(val, base + ESDCTL_V4_PDCMPR2);

	/* pre-charge all RAM banks */
	val = ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_CS0 |
		ESDCTL_V4_ESDSCR_CMD_PRE_ALL;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_CS1 |
		ESDCTL_V4_ESDSCR_CMD_PRE_ALL;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* configure RAMs to generate MPR pattern */
	val = ESDCTL_V4_DDR3_MR3_MPR_ENABLE | ESDCTL_V4_DDR3_MR3_MPR_PATTERN |
		ESDCTL_V4_ESDSCR_CMD_CS0 | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_DDR3_REG_MR3;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_DDR3_MR3_MPR_ENABLE | ESDCTL_V4_DDR3_MR3_MPR_PATTERN |
		ESDCTL_V4_ESDSCR_CMD_CS1 | ESDCTL_V4_ESDSCR_CON_REQ |
		ESDCTL_V4_ESDSCR_CMD_LMR | ESDCTL_V4_DDR3_REG_MR3;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* start read delay-line calibration */
	writel(ESDCTL_V4_RDDLHWCTL_HW_RDL_EN, base + ESDCTL_V4_RDDLHWCTL);

	do {
		val = readl(base + ESDCTL_V4_RDDLHWCTL);
	} while (val & ESDCTL_V4_RDDLHWCTL_HW_RDL_EN);

	val &= ESDCTL_V4_RDDLHWCTL_HW_RDL_ERR3 |
		ESDCTL_V4_RDDLHWCTL_HW_RDL_ERR2 |
		ESDCTL_V4_RDDLHWCTL_HW_RDL_ERR1 |
		ESDCTL_V4_RDDLHWCTL_HW_RDL_ERR0;

	if (val)
		hang();

	/* configure RAMs back to normal operation */
	val = ESDCTL_V4_DDR3_MR3_MPR_DISABLE | ESDCTL_V4_ESDSCR_CMD_CS0 |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_DDR3_REG_MR3;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_DDR3_MR3_MPR_DISABLE | ESDCTL_V4_ESDSCR_CMD_CS1 |
		ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_LMR |
		ESDCTL_V4_DDR3_REG_MR3;
	writel(val, base + ESDCTL_V4_ESDSCR);
}

void imx_esdctlv4_do_write_delay_line_calibration(void)
{
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);
	void __iomem *adr;
	u32 val;

	/*
	 * configure ESCTL to normal operation so we can
	 * write the compare values
	 */
	writel(ESDCTL_V4_ESDSCR_CMD_NOP, base + ESDCTL_V4_ESDSCR);

	/* write test-pattern to RAM */

	/* ESCTL uses this address for calibration */
	adr = IOMEM(MX53_CSD0_BASE_ADDR) + 0x10000000;
	writel(0, adr + 0x00);
	writel(0, adr + 0x0c);
	writel(0, adr + 0x10);
	writel(0, adr + 0x1c);
	writel(0, adr + 0x20);
	writel(0, adr + 0x2c);
	writel(0, adr + 0x30);
	writel(0, adr + 0x4c);
	writel(0xffffffff, adr + 0x04);
	writel(0xffffffff, adr + 0x08);
	writel(0xffffffff, adr + 0x14);
	writel(0xffffffff, adr + 0x18);
	writel(0xffffffff, adr + 0x24);
	writel(0xffffffff, adr + 0x28);
	writel(0xffffffff, adr + 0x34);
	writel(0xffffffff, adr + 0x48);

	/* pre-charge all RAM banks */
	val = ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_CS0 |
		ESDCTL_V4_ESDSCR_CMD_PRE_ALL;
	writel(val, base + ESDCTL_V4_ESDSCR);

	val = ESDCTL_V4_ESDSCR_CON_REQ | ESDCTL_V4_ESDSCR_CMD_CS1 |
		ESDCTL_V4_ESDSCR_CMD_PRE_ALL;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* write test-pattern to ESCTL */
	writel(0x00ffff00, base + ESDCTL_V4_PDCMPR1);
	writel(0, base + ESDCTL_V4_PDCMPR2);

	/* start write delay-line calibration */
	writel(ESDCTL_V4_WRDLHWCTL_HW_WDL_EN, base + ESDCTL_V4_WRDLHWCTL);

	do {
		val = readl(base + ESDCTL_V4_WRDLHWCTL);
	} while (val & ESDCTL_V4_WRDLHWCTL_HW_WDL_EN);

	val &= ESDCTL_V4_WRDLHWCTL_HW_WDL_ERR3 |
		ESDCTL_V4_WRDLHWCTL_HW_WDL_ERR2 |
		ESDCTL_V4_WRDLHWCTL_HW_WDL_ERR1 |
		ESDCTL_V4_WRDLHWCTL_HW_WDL_ERR0;

	if (val)
		hang();
}

#define SDRAM_COMPARE_CONST1 0x12345678
#define SDRAM_COMPARE_CONST2 0xAAAAAAAA

/*
 * write magic values to RAM for testing purposes
 */
static void imx_esdctlv4_write_magic_values(void __iomem *adr)
{
	/*
	 * Freescale asks for first access to be a write to properly
	 * initialize DQS pin-state and keepers
	 */
	writel(SDRAM_COMPARE_CONST1, adr);

	/*
	 * write two magic constants to RAM, to test for bus-size and
	 * row-/column-configuraion
	 */
	writel(SDRAM_COMPARE_CONST1, adr);
	writel(SDRAM_COMPARE_CONST2, adr + 4);
	dsb();
}

/*
 * check if given DRAM addresses match expected values for row/col configuration
 */
static u32 check_ram_address_line(void __iomem *adr, u32 compare, u32 mask)
{
	u32 val;

	val = readl(adr);	/* load data from RAM-address to check */
	val &= mask;		/* mask data for our bus-size */

	if (compare == val)	/* compare read data with expected value */
		return 1;	/* if data is identical, update ESDCTL configuration */
	else
		return 0;
}

/*
 * determine RAM chip-density and configure tRFC and tXS accordingly
 */
void imx_esdctlv4_set_tRFC_timing(void)
{
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);
	u32 val, trfc, r2, esdcfg;

	/* determine chip-density */
	val = readl(base + ESDCTL_V4_ESDMISC);
	if ((val & ESDCTL_V4_ESDMISC_BANKS_MASK) == ESDCTL_V4_ESDMISC_BANKS_4)
		r2 = 2;
	else
		r2 = 3;

	val = readl(base + ESDCTL_V4_ESDCTL0);
	if ((val & ESDCTL_V4_ESDCTLx_DSIZ_MASK) == ESDCTL_V4_ESDCTLx_DSIZ_32B)
		r2 += 1;

	switch (val & ESDCTL_V4_ESDCTLx_ROW_MASK) {
		case ESDCTL_V4_ESDCTLx_ROW_11: r2 += 1; break;
		case ESDCTL_V4_ESDCTLx_ROW_12: r2 += 2; break;
		case ESDCTL_V4_ESDCTLx_ROW_13: r2 += 3; break;
		case ESDCTL_V4_ESDCTLx_ROW_14: r2 += 4; break;
		case ESDCTL_V4_ESDCTLx_ROW_15: r2 += 5; break;
		case ESDCTL_V4_ESDCTLx_ROW_16: r2 += 6; break;
	}

	switch (val & ESDCTL_V4_ESDCTLx_COL_MASK) {
		case ESDCTL_V4_ESDCTLx_COL_8: r2 += 8; break;
		case ESDCTL_V4_ESDCTLx_COL_9: r2 += 9; break;
		case ESDCTL_V4_ESDCTLx_COL_10: r2 += 10; break;
		case ESDCTL_V4_ESDCTLx_COL_11: r2 += 11; break;
		case ESDCTL_V4_ESDCTLx_COL_12: r2 += 12; break;
	}

	/* save current tRFC timing */
	esdcfg = readl(base + ESDCTL_V4_ESDCFG0);

	trfc = (esdcfg & ESDCTL_V4_ESDCFG0_tRFC_MASK) >> ESDCTL_V4_ESDCFG0_tRFC_SHIFT;

	/* clear tRFC and tXS timings */
	esdcfg &= ~(ESDCTL_V4_ESDCFG0_tRFC_MASK | ESDCTL_V4_ESDCFG0_tXS_MASK);

	/*
	 * determine tRFC depending on density
	 * (the timings and density-associations are taken
	 * from JEDEC JESD79-3E DDR3-RAM specification)
	 */
	if (r2 >= 16)
		trfc = 36 - 1;
	if (r2 >= 17)
		trfc = 44 - 1;
	if (r2 >= 18)
		trfc = 64 - 1;
	if (r2 >= 19)
		trfc = 120 - 1;
	if (r2 >= 20)
		trfc = 140 - 1;

	/* calculate new tRFC and tXS timings */
	esdcfg |= (trfc << ESDCTL_V4_ESDCFG0_tRFC_SHIFT) |
		(trfc + 4) << ESDCTL_V4_ESDCFG0_tXS_SHIFT;

	writel(esdcfg, base + ESDCTL_V4_ESDCFG0);
}

/*
 * disable chip-selects that are not equipped
 */
void imx_esdctlv4_detect_sdrams(void)
{
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);
	u32 esdctl0;

	esdctl0 = readl(base + ESDCTL_V4_ESDCTL0);

	writel(MX53_CSD0_BASE_ADDR, MX53_CSD0_BASE_ADDR);
	writel(MX53_CSD1_BASE_ADDR, MX53_CSD1_BASE_ADDR);

	if (readl(MX53_CSD0_BASE_ADDR) != MX53_CSD0_BASE_ADDR)
		esdctl0 &= ~ESDCTL_V4_ESDCTLx_SDE0;
	if (readl(MX53_CSD1_BASE_ADDR) != MX53_CSD1_BASE_ADDR)
		esdctl0 &= ~ESDCTL_V4_ESDCTLx_SDE1;

	writel(esdctl0, base + ESDCTL_V4_ESDCTL0);
}

void imx_esdctlv4_init(void)
{
	void __iomem *base = IOMEM(MX53_ESDCTL_BASE_ADDR);
	u32 val, r1, esdctl0, mask, rows, cols;

	/*
	 * assume worst-case here: 4Gb chips. this will be optimized
	 * further down, when we can determine the actual chip density
	 * in imx_esdctlv4_set_tRFC_timing()
	 */
	val = (((140 - 1) << ESDCTL_V4_ESDCFG0_tRFC_SHIFT) |
				  ((144 - 1) << ESDCTL_V4_ESDCFG0_tXS_SHIFT) |
				  ((3 - 1) << ESDCTL_V4_ESDCFG0_tXP_SHIFT) |
				  ((10 - 1) << ESDCTL_V4_ESDCFG0_tXPDLL_SHIFT) |
				  ((20 - 1) << ESDCTL_V4_ESDCFG0_tFAW_SHIFT) |
				  ((6 - 3) << ESDCTL_V4_ESDCFG0_tCL_SHIFT));
	writel(val, base + ESDCTL_V4_ESDCFG0);

	val = (((6 - 1) << ESDCTL_V4_ESDCFG1_tRCD_SHIFT) |
				  ((6 - 1) << ESDCTL_V4_ESDCFG1_tRP_SHIFT) |
				  ((21 - 1) << ESDCTL_V4_ESDCFG1_tRC_SHIFT) |
				  ((15 - 1) << ESDCTL_V4_ESDCFG1_tRAS_SHIFT) |
				  (1 << ESDCTL_V4_ESDCFG1_tRPA_SHIFT) |
				  ((6 - 1) << ESDCTL_V4_ESDCFG1_tWR_SHIFT) |
				  ((4 - 1) << ESDCTL_V4_ESDCFG1_tMRD_SHIFT) |
				  ((5 - 2) << ESDCTL_V4_ESDCFG1_tCWL_SHIFT));
	writel(val, base + ESDCTL_V4_ESDCFG1);

	val = (((512 - 1) << ESDCTL_V4_ESDCFG2_tDLLK_SHIFT) |
				  ((4 - 1) << ESDCTL_V4_ESDCFG2_tRTP_SHIFT) |
				  ((4 - 1) << ESDCTL_V4_ESDCFG2_tWTR_SHIFT) |
				  ((4 - 1) << ESDCTL_V4_ESDCFG2_tRRD_SHIFT));
	writel(val, base + ESDCTL_V4_ESDCFG2);

	/*
	 * we don't touch the ESDRWD register as documentation claims
	 * the power-on defaults are minimum required values and we don't
	 * want to interfere with changes of these in new chip or
	 * BootROM revisions.
	 */

	val = (((3 - 1) << ESDCTL_V4_ESDOTC_tAOFPD_SHIFT) |
				  ((3 - 1) << ESDCTL_V4_ESDOTC_tAONPD_SHIFT) |
				  ((4 - 1) << ESDCTL_V4_ESDOTC_tANPD_SHIFT) |
				  ((4 - 1) << ESDCTL_V4_ESDOTC_tAXPD_SHIFT) |
				  (3 << ESDCTL_V4_ESDOTC_tODTLon_SHIFT) |
				  (3 << ESDCTL_V4_ESDOTC_tODT_idle_off_SHIFT));
	writel(val, base + ESDCTL_V4_ESDOTC);

	/* currently we only support DDR3 RAM, which always has 8 banks */
	val = ESDCTL_V4_ESDMISC_WALAT_0 | ESDCTL_V4_ESDMISC_RALAT_2 |
				  ESDCTL_V4_ESDMISC_MIF3_MODE_EFAM |
				  ESDCTL_V4_ESDMISC_DDR_DDR3 |
				  ESDCTL_V4_ESDMISC_BANKS_8;
	writel(val, base + ESDCTL_V4_ESDMISC);

	val = (((144 - 1) << ESDCTL_V4_ESDOR_tXPR_SHIFT) |
			((13 + 1) << ESDCTL_V4_ESDOR_SDE_to_RST_SHIFT) |
			((32 + 1) << ESDCTL_V4_ESDOR_RST_to_CKE_SHIFT));
	writel(val, base + ESDCTL_V4_ESDOR);

	/*
	 * we assume maximum address line configuration here (32b, 16rows, 12cols,
	 * both chips) once the RAM is initialized, we determine actual configuration
	 */
	val = ESDCTL_V4_ESDCTLx_SDE0 | ESDCTL_V4_ESDCTLx_SDE1 |
			ESDCTL_V4_ESDCTLx_ROW_16 | ESDCTL_V4_ESDCTLx_COL_8 |
			ESDCTL_V4_ESDCTLx_BL_8_8 | ESDCTL_V4_ESDCTLx_DSIZ_32B;
	writel(val, base + ESDCTL_V4_ESDCTL0);

	imx_esdctlv4_start_ddr3_sdram(0);
	imx_esdctlv4_start_ddr3_sdram(1);

	val = (ESDCTL_V4_ESDPDC_PRCT_DISABLE << ESDCTL_V4_ESDPDC_PRCT1_SHIFT) |
		(ESDCTL_V4_ESDPDC_PRCT_DISABLE << ESDCTL_V4_ESDPDC_PRCT0_SHIFT) |
		((3 - 1) << ESDCTL_V4_ESDPDC_tCKE_SHIFT) |
		(ESDCTL_V4_ESDPDC_PWDT_DISABLE << ESDCTL_V4_ESDPDC_PWDT1_SHIFT) |
		(ESDCTL_V4_ESDPDC_PWDT_DISABLE << ESDCTL_V4_ESDPDC_PWDT0_SHIFT) |
		(5 << ESDCTL_V4_ESDPDC_tCKSRX_SHIFT) |
		(5 << ESDCTL_V4_ESDPDC_tCKSRE_SHIFT);
	writel(val, base + ESDCTL_V4_ESDPDC);

	/* configure ODT */
	val = (ESDCTL_V4_ESDODTC_RTT_120 << ESDCTL_V4_ESDODTC_ODT3_INT_RES_SHIFT) |
		(ESDCTL_V4_ESDODTC_RTT_120 << ESDCTL_V4_ESDODTC_ODT2_INT_RES_SHIFT) |
		(ESDCTL_V4_ESDODTC_RTT_120 << ESDCTL_V4_ESDODTC_ODT1_INT_RES_SHIFT) |
		(ESDCTL_V4_ESDODTC_RTT_120 << ESDCTL_V4_ESDODTC_ODT0_INT_RES_SHIFT) |
		ESDCTL_V4_ESDODTC_ODT_RD_PAS_EN |
		ESDCTL_V4_ESDODTC_ODT_WR_ACT_EN |
		ESDCTL_V4_ESDODTC_ODT_WR_PAS_EN;
	writel(val, base + ESDCTL_V4_ODTCTRL);

	/*
	 * ensure refresh and ZQ calibration are turned off as
	 * nothing may interfere with the first few calibrations
	 */
	writel(ESDCTL_V4_ESDREF_REF_SEL_MASK, base + ESDCTL_V4_ESDREF);

	writel(ESDCTL_V4_ESDZQHWC_ZQ_MODE_NO_CAL, base + ESDCTL_V4_ZQHWCTRL);

	/*
	 * ensure that read/write delays are configured to
	 * (working) 1/4 cycle default delay, proper delays
	 * will be calibrated below.
	 */
	writel(0x30303030, base + ESDCTL_V4_RDDLCTL);
	writel(0x40404040, base + ESDCTL_V4_WRDLCTL);

	/* DQS Gating */
	imx_esdctlv4_do_dqs_gating();

	/* Write Leveling */
	imx_esdctlv4_do_write_leveling();

	/*
	 * ZQ calibration (this will also enable regular
	 * automatic ZQ calibration again)
	 */
	imx_esdctlv4_do_zq_calibration();

	/* configure and enable refresh */
	val = (0 << ESDCTL_V4_ESDREF_REF_CNT_SHIFT) |
		ESDCTL_V4_ESDREF_REF_SEL_64K |
		((2 - 1) << ESDCTL_V4_ESDREF_REFR_SHIFT);
	writel(val, base + ESDCTL_V4_ESDREF);


	/* Now do proper Delay Line Calibration */
	imx_esdctlv4_do_read_delay_line_calibration();
	imx_esdctlv4_do_write_delay_line_calibration();

	/* enable regular operation of ESDCTL */
	writel(ESDCTL_V4_ESDSCR_CMD_NOP, base + ESDCTL_V4_ESDSCR);

	dsb();

	/*
	 * detect RAM configuration (Note: both CSDx must be
	 * equipped with identical RAMs, so we only need to detect
	 * the configuration of CSD0 and anything on CSD1)
	 */
	esdctl0 = readl(base + ESDCTL_V4_ESDCTL0);

	imx_esdctlv4_write_magic_values((void *)MX53_CSD0_BASE_ADDR);

	/* check for bus-configuration first */
	esdctl0 &= ~ESDCTL_V4_ESDCTLx_DSIZ_MASK;

	/* assume we're on a 32b bus */
	mask = 0xffffffff;

	/* data correct? */
	if (readl(MX53_CSD0_BASE_ADDR) == SDRAM_COMPARE_CONST1) {
		esdctl0 |= ESDCTL_V4_ESDCTLx_DSIZ_32B; /* yep, indeed 32b bus */
		goto sdram_bussize_found;
	}

	/*
	 * ok, last possibility is 16b bus on low data-lines, check that
	 * (i.MX25 also suports 16b on high data-lines, but i.MX53 doesn't)
	 */
	if ((readl(MX53_CSD0_BASE_ADDR) & 0xffff) == (SDRAM_COMPARE_CONST1 & 0xffff)) {
		esdctl0 |= ESDCTL_V4_ESDCTLx_DSIZ_16B_LOW;
		mask >>= 16;
		goto sdram_bussize_found;
	}

	/* nope, no working SDRAM, leave. */
	hang();

sdram_bussize_found:

	/* Now determine actual row/column configuration of the RAMs */

	/* mask unused bits from our compare constant */
	r1 = SDRAM_COMPARE_CONST1 & mask;
	/*
	 * So far we asssumed that we have 16 rows, check for copies of our
	 * SDRAM_COMPARE_CONST1 due to missing row lines...
	 */

	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 25), r1, mask))
		rows = ESDCTL_V4_ESDCTLx_ROW_15;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 24), r1, mask))
		rows = ESDCTL_V4_ESDCTLx_ROW_14;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 23), r1, mask))
		rows = ESDCTL_V4_ESDCTLx_ROW_13;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 22), r1, mask))
		rows = ESDCTL_V4_ESDCTLx_ROW_12;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 21), r1, mask))
		rows = ESDCTL_V4_ESDCTLx_ROW_11;

	esdctl0 &= ~ESDCTL_V4_ESDCTLx_ROW_MASK;
	esdctl0 |= rows;

	/*
	 * To detect columns we have to switch from the (max rows, min cols)
	 * configuration we used so far to a (min rows, max cols) configuration
	 */

	/* switch ESDCTL to configuration mode */
	val = ESDCTL_V4_ESDSCR_CMD_NOP | ESDCTL_V4_ESDSCR_CON_REQ;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* reconfigure row-/column-lines */
	val = readl(base + ESDCTL_V4_ESDCTL0);
	val &= ~(ESDCTL_V4_ESDCTLx_ROW_MASK | ESDCTL_V4_ESDCTLx_COL_MASK);
	val |= ESDCTL_V4_ESDCTLx_ROW_11 | ESDCTL_V4_ESDCTLx_COL_12;
	writel(val, base + ESDCTL_V4_ESDCTL0);

	dsb();

	/* switch ESDCTL back to normal operation */
	writel(ESDCTL_V4_ESDSCR_CMD_NOP, base + ESDCTL_V4_ESDSCR);

	/*
	 * not quite sure why, but the row-/col-reconfiguration destroys the
	 * contents of the RAM so we have to write our magic values back
	 * (maybe because refresh is suspended during that time)
	 */
	imx_esdctlv4_write_magic_values((void *)MX53_CSD0_BASE_ADDR);

	/*
	 * So far we asssumed that we have 12 columns, check for copies of our
	 * SDRAM_COMPARE_CONST1 due to missing column lines...
	 */

	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 13), r1, mask))
		cols = ESDCTL_V4_ESDCTLx_COL_11;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 12), r1, mask))
		cols = ESDCTL_V4_ESDCTLx_COL_10;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 11), r1, mask))
		cols = ESDCTL_V4_ESDCTLx_COL_9;
	if (check_ram_address_line((void *)MX53_CSD0_BASE_ADDR + (1 << 10), r1, mask))
		cols = ESDCTL_V4_ESDCTLx_COL_8;

	esdctl0 &= ~ESDCTL_V4_ESDCTLx_COL_MASK;
	esdctl0 |= cols;

	/* setup proper row-/column-configuration */

	/* switch ESDCTL to configuration mode */
	val = ESDCTL_V4_ESDSCR_CMD_NOP | ESDCTL_V4_ESDSCR_CON_REQ;
	writel(val, base + ESDCTL_V4_ESDSCR);

	/* reconfigure row-/column-lines */
	writel(esdctl0, base + ESDCTL_V4_ESDCTL0);

	/* configure densitiy dependent timing parameters */
	imx_esdctlv4_set_tRFC_timing();

	dsb();

	/* switch ESDCTL back to normal operation */
	writel(ESDCTL_V4_ESDSCR_CMD_NOP, base + ESDCTL_V4_ESDSCR);

	/* see at which CSx we actually have working RAM */
	imx_esdctlv4_detect_sdrams();
}
