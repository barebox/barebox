/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <io.h>
#include <mach/cyclone5-clock-manager.h>
#include <mach/cyclone5-regs.h>
#include <mach/generic.h>

static inline void cm_wait_for_lock(void __iomem *cm, uint32_t mask)
{
	while ((readl(cm + CLKMGR_INTER_ADDRESS) & mask) != mask);
}

/* function to poll in the fsm busy bit */
static inline void cm_wait4fsm(void __iomem *cm)
{
	while (readl(cm + CLKMGR_STAT_ADDRESS) & 1);
}

/*
 * function to write the bypass register which requires a poll of the
 * busy bit
 */
static inline void cm_write_bypass(void __iomem *cm, uint32_t val)
{
	writel(val, cm + CLKMGR_BYPASS_ADDRESS);
	cm_wait4fsm(cm);
}

/* function to write the ctrl register which requires a poll of the busy bit */
static inline void cm_write_ctrl(void __iomem *cm, uint32_t val)
{
	writel(val, cm + CLKMGR_CTRL_ADDRESS);
	cm_wait4fsm(cm);
}

/* function to write a clock register that has phase information */
static inline void cm_write_with_phase(uint32_t value,
	void __iomem *reg, uint32_t mask)
{
	/* poll until phase is zero */
	while (readl(reg) & mask);

	writel(value, reg);

	while (readl(reg) & mask);
}

/*
 * Setup clocks while making no assumptions of the
 * previous state of the clocks.
 *
 * - Start by being paranoid and gate all sw managed clocks
 * - Put all plls in bypass
 * - Put all plls VCO registers back to reset value (bgpwr dwn).
 * - Put peripheral and main pll src to reset value to avoid glitch.
 * - Delay 5 us.
 * - Deassert bg pwr dn and set numerator and denominator
 * - Start 7 us timer.
 * - set internal dividers
 * - Wait for 7 us timer.
 * - Enable plls
 * - Set external dividers while plls are locking
 * - Wait for pll lock
 * - Assert/deassert outreset all.
 * - Take all pll's out of bypass
 * - Clear safe mode
 * - set source main and peripheral clocks
 * - Ungate clocks
 */
void socfpga_cm_basic_init(const struct socfpga_cm_config *cfg)
{
	uint32_t mainvco, periphvco, val;
	void *cm = (void *)CYCLONE5_CLKMGR_ADDRESS;

	/* Start by being paranoid and gate all sw managed clocks */

	/*
	 * We need to disable nandclk
	 * and then do another apb access before disabling
	 * gatting off the rest of the periperal clocks.
	 */
	val = readl(cm + CLKMGR_PERPLLGRP_EN_ADDRESS);
	val &= ~CLKMGR_PERPLLGRP_EN_NANDCLK_MASK;
	writel(val, cm + CLKMGR_PERPLLGRP_EN_ADDRESS);

	/* DO NOT GATE OFF DEBUG CLOCKS & BRIDGE CLOCKS */
	writel(CLKMGR_MAINPLLGRP_EN_DBGTIMERCLK_MASK |
		CLKMGR_MAINPLLGRP_EN_DBGTRACECLK_MASK |
		CLKMGR_MAINPLLGRP_EN_DBGCLK_MASK |
		CLKMGR_MAINPLLGRP_EN_DBGATCLK_MASK |
		CLKMGR_MAINPLLGRP_EN_S2FUSER0CLK_MASK |
		CLKMGR_MAINPLLGRP_EN_L4MPCLK_MASK,
		cm + CLKMGR_MAINPLLGRP_EN_ADDRESS);

	writel(0, cm + CLKMGR_SDRPLLGRP_EN_ADDRESS);

	/* now we can gate off the rest of the peripheral clocks */
	writel(0, cm + CLKMGR_PERPLLGRP_EN_ADDRESS);

	/* Put all plls in bypass */
	cm_write_bypass(cm,
		CLKMGR_BYPASS_PERPLL_SET(1) |
		CLKMGR_BYPASS_SDRPLL_SET(1) |
		CLKMGR_BYPASS_MAINPLL_SET(1));

	/*
	 * Put all plls VCO registers back to reset value.
	 * Some code might have messed with them.
	 */
	writel(CLKMGR_MAINPLLGRP_VCO_RESET_VALUE &
		~CLKMGR_MAINPLLGRP_VCO_REGEXTSEL_MASK,
		cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);
	writel(CLKMGR_PERPLLGRP_VCO_RESET_VALUE &
		~CLKMGR_PERPLLGRP_VCO_REGEXTSEL_MASK,
		cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);
	writel(CLKMGR_SDRPLLGRP_VCO_RESET_VALUE &
		~CLKMGR_SDRPLLGRP_VCO_REGEXTSEL_MASK,
		cm + CLKMGR_SDRPLLGRP_VCO_ADDRESS);

	/*
	 * The clocks to the flash devices and the L4_MAIN clocks can
	 * glitch when coming out of safe mode if their source values
	 * are different from their reset value.  So the trick it to
	 * put them back to their reset state, and change input
	 * after exiting safe mode but before ungating the clocks.
	 */
	writel(CLKMGR_PERPLLGRP_SRC_RESET_VALUE,
		cm + CLKMGR_PERPLLGRP_SRC_ADDRESS);
	writel(CLKMGR_MAINPLLGRP_L4SRC_RESET_VALUE,
		cm + CLKMGR_MAINPLLGRP_L4SRC_ADDRESS);

	/* read back for the required 5 us delay. */
	readl(cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);
	readl(cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);
	readl(cm + CLKMGR_SDRPLLGRP_VCO_ADDRESS);

	/*
	 * We made sure bgpwr down was assert for 5 us. Now deassert BG PWR DN
	 * with numerator and denominator.
	 */
	writel(cfg->main_vco_base | CLEAR_BGP_EN_PWRDN,
		cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);
	writel(cfg->peri_vco_base | CLEAR_BGP_EN_PWRDN,
		cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);
	writel(cfg->sdram_vco_base |
	       CLKMGR_SDRPLLGRP_VCO_OUTRESET_SET(0) |
	       CLKMGR_SDRPLLGRP_VCO_OUTRESETALL_SET(0) |
	       CLEAR_BGP_EN_PWRDN,
	       cm + CLKMGR_SDRPLLGRP_VCO_ADDRESS);

	writel(cfg->mpuclk, cm + CLKMGR_MAINPLLGRP_MPUCLK_ADDRESS);
	writel(cfg->mainclk, cm + CLKMGR_MAINPLLGRP_MAINCLK_ADDRESS);
	writel(cfg->alteragrp_mpu, cm + CLKMGR_ALTERAGRP_MPUCLK);
	writel(cfg->dbgatclk, cm + CLKMGR_MAINPLLGRP_DBGATCLK_ADDRESS);
	writel(cfg->alteregrp_main, cm + CLKMGR_ALTERAGRP_MAINCLK);
	writel(cfg->cfg2fuser0clk, cm + CLKMGR_MAINPLLGRP_CFGS2FUSER0CLK_ADDRESS);
	writel(cfg->emac0clk, cm + CLKMGR_PERPLLGRP_EMAC0CLK_ADDRESS);
	writel(cfg->emac1clk, cm + CLKMGR_PERPLLGRP_EMAC1CLK_ADDRESS);
	writel(cfg->mainqspiclk, cm + CLKMGR_MAINPLLGRP_MAINQSPICLK_ADDRESS);
	writel(cfg->perqspiclk, cm + CLKMGR_PERPLLGRP_PERQSPICLK_ADDRESS);
	writel(cfg->mainnandsdmmcclk, cm + CLKMGR_MAINPLLGRP_MAINNANDSDMMCCLK_ADDRESS);
	writel(cfg->pernandsdmmcclk, cm + CLKMGR_PERPLLGRP_PERNANDSDMMCCLK_ADDRESS);
	writel(cfg->perbaseclk, cm + CLKMGR_PERPLLGRP_PERBASECLK_ADDRESS);
	writel(cfg->s2fuser1clk, cm + CLKMGR_PERPLLGRP_S2FUSER1CLK_ADDRESS);

	/* 7 us must have elapsed before we can enable the VCO */
	__udelay(7);

	/* Enable vco */
	writel(cfg->main_vco_base | CLKMGR_MAINPLLGRP_VCO_EN_SET(1),
			cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);
	writel(cfg->peri_vco_base | CLKMGR_MAINPLLGRP_VCO_EN_SET(1),
			cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);
	writel(cfg->sdram_vco_base | CLKMGR_MAINPLLGRP_VCO_EN_SET(1),
			cm + CLKMGR_SDRPLLGRP_VCO_ADDRESS);

	/* setup dividers while plls are locking */

	/* L3 MP and L3 SP */
	writel(cfg->maindiv, cm + CLKMGR_MAINPLLGRP_MAINDIV_ADDRESS);
	writel(cfg->dbgdiv, cm + CLKMGR_MAINPLLGRP_DBGDIV_ADDRESS);
	writel(cfg->tracediv, cm + CLKMGR_MAINPLLGRP_TRACEDIV_ADDRESS);

	/* L4 MP, L4 SP, can0, and can1 */
	writel(cfg->perdiv, cm + CLKMGR_PERPLLGRP_DIV_ADDRESS);
	writel(cfg->gpiodiv, cm + CLKMGR_PERPLLGRP_GPIODIV_ADDRESS);

	cm_wait_for_lock(cm, CLKMGR_INTER_SDRPLLLOCKED_MASK |
			CLKMGR_INTER_PERPLLLOCKED_MASK  |
			CLKMGR_INTER_MAINPLLLOCKED_MASK);

	/* write the sdram clock counters before toggling outreset all */
	writel(cfg->ddrdqsclk & CLKMGR_SDRPLLGRP_DDRDQSCLK_CNT_MASK,
		cm + CLKMGR_SDRPLLGRP_DDRDQSCLK_ADDRESS);

	writel(cfg->ddr2xdqsclk & CLKMGR_SDRPLLGRP_DDR2XDQSCLK_CNT_MASK,
		cm + CLKMGR_SDRPLLGRP_DDR2XDQSCLK_ADDRESS);

	writel(cfg->ddrdqclk & CLKMGR_SDRPLLGRP_DDRDQCLK_CNT_MASK,
		cm + CLKMGR_SDRPLLGRP_DDRDQCLK_ADDRESS);

	writel(cfg->s2fuser2clk & CLKMGR_SDRPLLGRP_S2FUSER2CLK_CNT_MASK,
		cm + CLKMGR_SDRPLLGRP_S2FUSER2CLK_ADDRESS);

	/*
	 * after locking, but before taking out of bypass
	 * assert/deassert outresetall
	 */
	mainvco = readl(cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);

	/* assert main outresetall */
	writel(mainvco | CLKMGR_MAINPLLGRP_VCO_OUTRESETALL_MASK,
		cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);

	periphvco = readl(cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);

	/* assert pheriph outresetall */
	writel(periphvco | CLKMGR_PERPLLGRP_VCO_OUTRESETALL_MASK,
		cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);

	/* assert sdram outresetall */
	writel(cfg->sdram_vco_base | CLKMGR_MAINPLLGRP_VCO_EN_SET(1) |
		CLKMGR_SDRPLLGRP_VCO_OUTRESETALL_SET(1),
		cm + CLKMGR_SDRPLLGRP_VCO_ADDRESS);

	/* deassert main outresetall */
	writel(mainvco & ~CLKMGR_MAINPLLGRP_VCO_OUTRESETALL_MASK,
		cm + CLKMGR_MAINPLLGRP_VCO_ADDRESS);

	/* deassert pheriph outresetall */
	writel(periphvco & ~CLKMGR_PERPLLGRP_VCO_OUTRESETALL_MASK,
		cm + CLKMGR_PERPLLGRP_VCO_ADDRESS);

	/* deassert sdram outresetall */
	writel(cfg->sdram_vco_base | CLKMGR_MAINPLLGRP_VCO_EN_SET(1),
		cm + CLKMGR_SDRPLLGRP_VCO_ADDRESS);

	/*
	 * now that we've toggled outreset all, all the clocks
	 * are aligned nicely; so we can change any phase.
	 */
	cm_write_with_phase(cfg->ddrdqsclk,
		cm + CLKMGR_SDRPLLGRP_DDRDQSCLK_ADDRESS,
		CLKMGR_SDRPLLGRP_DDRDQSCLK_PHASE_MASK);

	/* SDRAM DDR2XDQSCLK */
	cm_write_with_phase(cfg->ddr2xdqsclk,
		cm + CLKMGR_SDRPLLGRP_DDR2XDQSCLK_ADDRESS,
		CLKMGR_SDRPLLGRP_DDR2XDQSCLK_PHASE_MASK);

	cm_write_with_phase(cfg->ddrdqclk,
		cm + CLKMGR_SDRPLLGRP_DDRDQCLK_ADDRESS,
		CLKMGR_SDRPLLGRP_DDRDQCLK_PHASE_MASK);

	cm_write_with_phase(cfg->s2fuser2clk,
		cm + CLKMGR_SDRPLLGRP_S2FUSER2CLK_ADDRESS,
		CLKMGR_SDRPLLGRP_S2FUSER2CLK_PHASE_MASK);

	/* Take all three PLLs out of bypass when safe mode is cleared. */
	cm_write_bypass(cm, 0);

	/* clear safe mode */
	val = readl(cm + CLKMGR_CTRL_ADDRESS);
	val |= CLKMGR_CTRL_SAFEMODE_SET(CLKMGR_CTRL_SAFEMODE_MASK);
	cm_write_ctrl(cm, val);

	/*
	 * now that safe mode is clear with clocks gated
	 * it safe to change the source mux for the flashes the the L4_MAIN
	 */
	writel(cfg->persrc, cm + CLKMGR_PERPLLGRP_SRC_ADDRESS);
	writel(cfg->l4src, cm + CLKMGR_MAINPLLGRP_L4SRC_ADDRESS);

	/* Now ungate non-hw-managed clocks */
	writel(~0, cm + CLKMGR_MAINPLLGRP_EN_ADDRESS);
	writel(~0, cm + CLKMGR_PERPLLGRP_EN_ADDRESS);
	writel(~0, cm + CLKMGR_SDRPLLGRP_EN_ADDRESS);

	val = readl(cm + CLKMGR_DBCTRL_ADDRESS);
	val |= CLKMGR_DBCTRL_STAYOSC1_MASK;
	writel(val, cm + CLKMGR_DBCTRL_ADDRESS);
}
