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
#include <mach/omap3-silicon.h>

/*
 * do_pac200_emif4_init -
 *  - Init the emif4 module for DDR access
 *  - Early init routines, called from flash or SRAM.
 */
void am35xx_emif4_init(void)
{
	unsigned int regval;
	struct emif4 *emif4_base = IOMEM(OMAP3_SDRC_BASE);

	/* Set the DDR PHY parameters in PHY ctrl registers */
	regval = (EMIF4_DDR1_READ_LAT | EMIF4_DDR1_PWRDN_DIS |
		EMIF4_DDR1_EXT_STRB_DIS);
	writel(regval, &emif4_base->ddr_phyctrl1);
	writel(regval, &emif4_base->ddr_phyctrl1_shdw);
	writel(0, &emif4_base->ddr_phyctrl2);

	/* Reset the DDR PHY and wait till completed */
	regval = readl(&emif4_base->sdram_iodft_tlgc);
	regval |= (1 << 10);
	writel(regval, &emif4_base->sdram_iodft_tlgc);

	/* Wait till that bit clears*/
	while ((readl(&emif4_base->sdram_iodft_tlgc) & (1 << 10)) == 0x1);

	/* Re-verify the DDR PHY status*/
	while ((readl(&emif4_base->sdram_sts) & (1 << 2)) == 0x0);

	regval |= (1 << 0);
	writel(regval, &emif4_base->sdram_iodft_tlgc);

	/* Set SDR timing registers */
	regval = (EMIF4_TIM1_T_WTR | EMIF4_TIM1_T_RRD |
		EMIF4_TIM1_T_RC | EMIF4_TIM1_T_RAS |
		EMIF4_TIM1_T_WR | EMIF4_TIM1_T_RCD |
		EMIF4_TIM1_T_RP);
	writel(regval, &emif4_base->sdram_time1);
	writel(regval, &emif4_base->sdram_time1_shdw);

	regval = (EMIF4_TIM2_T_CKE | EMIF4_TIM2_T_RTP |
		EMIF4_TIM2_T_XSRD | EMIF4_TIM2_T_XSNR |
		EMIF4_TIM2_T_ODT | EMIF4_TIM2_T_XP);
	writel(regval, &emif4_base->sdram_time2);
	writel(regval, &emif4_base->sdram_time2_shdw);

	regval = (EMIF4_TIM3_T_RAS_MAX | EMIF4_TIM3_T_RFC);
	writel(regval, &emif4_base->sdram_time3);
	writel(regval, &emif4_base->sdram_time3_shdw);

	/* Set the PWR control register */
	regval = (EMIF4_PWR_PM_TIM | EMIF4_PWR_LP_MODE |
		EMIF4_PWR_DPD_DIS | EMIF4_PWR_IDLE_MODE);
	writel(regval, &emif4_base->sdram_pwr_mgmt);
	writel(regval, &emif4_base->sdram_pwr_mgmt_shdw);

	/* Set the DDR refresh rate control register */
	regval = (EMIF4_REFRESH_RATE | EMIF4_INITREF_DIS);
	writel(regval, &emif4_base->sdram_refresh_ctrl);
	writel(regval, &emif4_base->sdram_refresh_ctrl_shdw);

	/* set the SDRAM configuration register */
	regval = (EMIF4_CFG_PGSIZE | EMIF4_CFG_EBANK |
		EMIF4_CFG_IBANK | EMIF4_CFG_ROWSIZE |
		EMIF4_CFG_CL | EMIF4_CFG_NARROW_MD |
		EMIF4_CFG_SDR_DRV | EMIF4_CFG_DDR_DIS_DLL |
		EMIF4_CFG_DDR2_DDQS | EMIF4_CFG_DDR_TERM |
		EMIF4_CFG_IBANK_POS | EMIF4_CFG_SDRAM_TYP);
	writel(regval, &emif4_base->sdram_config);
}
