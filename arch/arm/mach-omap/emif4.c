// SPDX-License-Identifier: GPL-2.0+
/*
 * Author :
 *     Vaibhav Hiremath <hvaibhav@ti.com>
 *
 * Based on mem.c and sdrc.c
 *
 * Copyright (C) 2010
 * Texas Instruments Incorporated - http://www.ti.com/
 */

#include <common.h>
#include <io.h>
#include <mach/emif4.h>

/*
 * AM35xx configuration values
 */
#define EMIF4_TIM1_T_RP		(0x3 << 25)
#define EMIF4_TIM1_T_RCD	(0x3 << 21)
#define EMIF4_TIM1_T_WR		(0x3 << 17)
#define EMIF4_TIM1_T_RAS	(0x7 << 12)
#define EMIF4_TIM1_T_RC		(0xa << 6)
#define EMIF4_TIM1_T_RRD	(0x2 << 3)
#define EMIF4_TIM1_T_WTR	(0x2)

#define EMIF4_TIM2_T_XP		(0x2 << 28)
#define EMIF4_TIM2_T_ODT	(0x0 << 25)
#define EMIF4_TIM2_T_XSNR	(0x1c << 16)
#define EMIF4_TIM2_T_XSRD	(0xc8 << 6)
#define EMIF4_TIM2_T_RTP	(0x1 << 3)
#define EMIF4_TIM2_T_CKE	(0x2)

#define EMIF4_TIM3_T_RFC	(0x15 << 4)
#define EMIF4_TIM3_T_RAS_MAX	(0xf)

#define EMIF4_PWR_IDLE_MODE	(0x2 << 30)
#define EMIF4_PWR_DPD_DIS	(0x0 << 10)
#define EMIF4_PWR_DPD_EN	(0x1 << 10)
#define EMIF4_PWR_LP_MODE	(0x0 << 8)
#define EMIF4_PWR_PM_TIM	(0x0)

#define EMIF4_INITREF_DIS	(0x0 << 31)
#define EMIF4_REFRESH_RATE	(0x257)

#define EMIF4_CFG_SDRAM_TYP	(0x2 << 29)
#define EMIF4_CFG_IBANK_POS	(0x0 << 27)
#define EMIF4_CFG_DDR_TERM	(0x3 << 24)
#define EMIF4_CFG_DDR2_DDQS	(0x1 << 23)
#define EMIF4_CFG_DDR_DIS_DLL	(0x0 << 20)
#define EMIF4_CFG_SDR_DRV	(0x0 << 18)
#define EMIF4_CFG_NARROW_MD	(0x0 << 14)
#define EMIF4_CFG_CL		(0x5 << 10)
#define EMIF4_CFG_ROWSIZE	(0x0 << 7)
#define EMIF4_CFG_IBANK		(0x3 << 4)
#define EMIF4_CFG_EBANK		(0x0 << 3)
#define EMIF4_CFG_PGSIZE	(0x2)

/*
 * EMIF4 PHY Control 1 register configuration
 */
#define EMIF4_DDR1_EXT_STRB_EN	(0x1 << 7)
#define EMIF4_DDR1_EXT_STRB_DIS	(0x0 << 7)
#define EMIF4_DDR1_PWRDN_DIS	(0x0 << 6)
#define EMIF4_DDR1_PWRDN_EN	(0x1 << 6)
#define EMIF4_DDR1_READ_LAT	(0x6 << 0)

/**
 * emif4_sdram_size - read back SDRAM size from sdram_config register
 *
 * @return: The SDRAM size
 */
unsigned long emif4_sdram_size(const void __iomem *emif4)
{
	uint32_t sdram_config = readl(emif4 + EMIF4_SDRAM_CONFIG);
	int rows, cols, width, banks;
	unsigned long size;

	rows = ((sdram_config >> 7) & 0x7) + 9;
	cols = (sdram_config & 0x7) + 8;

	switch ((sdram_config >> 14) & 0x3) {
	case 0:
		width = 4;
		break;
	case 1:
		width = 2;
		break;
	default:
		return 0;
	}

	switch ((sdram_config >> 4) & 0x7) {
	case 0:
		banks = 1;
		break;
	case 1:
		banks = 2;
		break;
	case 2:
		banks = 4;
		break;
	case 3:
		banks = 8;
		break;
	default:
		return 0;
	}

	size = (1 << rows) * (1 << cols) * banks * width;

	debug("SDRAM_CONFIG: 0x%08x, cols: %2d, rows: %2d, width: %2d, banks: %2d, size: 0x%08lx\n",
	      sdram_config, cols, rows, width, banks, size);

	return size;
}

/*
 *  - Init the emif4 module for DDR access
 *  - Early init routines, called from flash or SRAM.
 */
void am35xx_emif4_init(const void __iomem *emif4)
{
	unsigned int regval;

	/* Set the DDR PHY parameters in PHY ctrl registers */
	regval = (EMIF4_DDR1_READ_LAT | EMIF4_DDR1_PWRDN_DIS |
		EMIF4_DDR1_EXT_STRB_DIS);
	writel(regval, emif4 + EMIF4_DDR_PHY_CTRL_1);
	writel(regval, emif4 + EMIF4_DDR_PHY_CTRL_1_SHADOW);
	writel(0, emif4 + EMIF4_DDR_PHY_CTRL_2);

	/* Reset the DDR PHY and wait till completed */
	regval = readl(emif4 + EMIF4_IODFT_TLGC);
	regval |= (1 << 10);
	writel(regval, emif4 + EMIF4_IODFT_TLGC);

	/* Wait till that bit clears*/
	while (readl(emif4 + EMIF4_IODFT_TLGC) & (1 << 10));

	/* Re-verify the DDR PHY status*/
	while ((readl(emif4 + EMIF4_STATUS) & (1 << 2)) == 0x0);

	regval |= (1 << 0);
	writel(regval, emif4 + EMIF4_IODFT_TLGC);

	/* Set SDR timing registers */
	regval = (EMIF4_TIM1_T_WTR | EMIF4_TIM1_T_RRD |
		EMIF4_TIM1_T_RC | EMIF4_TIM1_T_RAS |
		EMIF4_TIM1_T_WR | EMIF4_TIM1_T_RCD |
		EMIF4_TIM1_T_RP);
	writel(regval, emif4 + EMIF4_SDRAM_TIM_1);
	writel(regval, emif4 + EMIF4_SDRAM_TIM_1_SHADOW);

	regval = (EMIF4_TIM2_T_CKE | EMIF4_TIM2_T_RTP |
		EMIF4_TIM2_T_XSRD | EMIF4_TIM2_T_XSNR |
		EMIF4_TIM2_T_ODT | EMIF4_TIM2_T_XP);
	writel(regval, emif4 + EMIF4_SDRAM_TIM_2);
	writel(regval, emif4 + EMIF4_SDRAM_TIM_2_SHADOW);

	regval = (EMIF4_TIM3_T_RAS_MAX | EMIF4_TIM3_T_RFC);
	writel(regval, emif4 + EMIF4_SDRAM_TIM_3);
	writel(regval, emif4 + EMIF4_SDRAM_TIM_3_SHADOW);

	/* Set the PWR control register */
	regval = (EMIF4_PWR_PM_TIM | EMIF4_PWR_LP_MODE |
		EMIF4_PWR_DPD_DIS | EMIF4_PWR_IDLE_MODE);
	writel(regval, emif4 + EMIF4_POWER_MANAGEMENT_CTRL);
	writel(regval, emif4 + EMIF4_POWER_MANAGEMENT_CTRL_SHADOW);

	/* Set the DDR refresh rate control register */
	regval = (EMIF4_REFRESH_RATE | EMIF4_INITREF_DIS);
	writel(regval, emif4 + EMIF4_SDRAM_REF_CTRL);
	writel(regval, emif4 + EMIF4_SDRAM_REF_CTRL_SHADOW);

	/* set the SDRAM configuration register */
	regval = (EMIF4_CFG_PGSIZE | EMIF4_CFG_EBANK |
		EMIF4_CFG_IBANK | EMIF4_CFG_ROWSIZE |
		EMIF4_CFG_CL | EMIF4_CFG_NARROW_MD |
		EMIF4_CFG_SDR_DRV | EMIF4_CFG_DDR_DIS_DLL |
		EMIF4_CFG_DDR2_DDQS | EMIF4_CFG_DDR_TERM |
		EMIF4_CFG_IBANK_POS | EMIF4_CFG_SDRAM_TYP);
	writel(regval, emif4 + EMIF4_SDRAM_CONFIG);
}
