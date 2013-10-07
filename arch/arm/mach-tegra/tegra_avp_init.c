/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Partly based on code (C) Copyright 2010-2011
 * NVIDIA Corporation <www.nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/lowlevel.h>
#include <mach/tegra20-car.h>
#include <mach/tegra20-pmc.h>

/* instruct the PMIC to enable the CPU power rail */
static void enable_maincomplex_powerrail(void)
{
	u32 reg;

	reg = readl(TEGRA_PMC_BASE + PMC_CNTRL);
	reg |= PMC_CNTRL_CPUPWRREQ_OE;
	writel(reg, TEGRA_PMC_BASE);
}

/* put every core in the main CPU complex into reset state */
static void assert_maincomplex_reset(int num_cores)
{
	u32 mask = 0;
	int i;

	for (i = 0; i < num_cores; i++)
		mask |= 0x1111 << i;

	writel(mask, TEGRA_CLK_RESET_BASE + CRC_RST_CPU_CMPLX_SET);
	writel(CRC_RST_DEV_L_CPU, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_L_SET);
}

/* release reset state of the first core of the main CPU complex */
static void deassert_cpu0_reset(void)
{
	writel(0x1111, TEGRA_CLK_RESET_BASE + CRC_RST_CPU_CMPLX_CLR);
	writel(CRC_RST_DEV_L_CPU, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_L_CLR);
}

/* stop all internal and external clocks to the main CPU complex */
static void stop_maincomplex_clocks(int num_cores)
{
	u32 reg;
	int i;

	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_CPU_CMPLX);
	for (i = 0; i < num_cores; i++)
		reg |= 0x1 << (8 + i);
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_CPU_CMPLX);

	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_L);
	reg &= ~CRC_CLK_OUT_ENB_L_CPU;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_L);
}

struct pll_config {
	u16 divn;
	u16 divm;
	u16 divp;
	u16 cpcon;
};

static struct pll_config pllx_config_table[][4] = {
	{
		{1000, 13, 0, 12},	/* OSC 13.0 MHz */
		{625,  12, 0, 8 },	/* OSC 19.2 MHz */
		{1000, 12, 0, 12},	/* OSC 12.0 MHz */
		{1000, 26, 0, 12},	/* OSC 26.0 MHz */
	}, /* TEGRA 20 */
};

static void init_pllx(void)
{
	struct pll_config *conf;
	enum tegra_chiptype chiptype;
	u8 osc_freq;
	u32 reg;

	/* If PLLX is already enabled, just return */
	if (readl(TEGRA_CLK_RESET_BASE + CRC_PLLX_BASE) & CRC_PLLX_BASE_ENABLE)
		return;

	chiptype = tegra_get_chiptype();

	osc_freq = (readl(TEGRA_CLK_RESET_BASE + CRC_OSC_CTRL) &
		    CRC_OSC_CTRL_OSC_FREQ_MASK) >> CRC_OSC_CTRL_OSC_FREQ_SHIFT;

	conf = &pllx_config_table[chiptype][osc_freq];

	/* set PLL bypass and frequency parameters */
	reg = CRC_PLLX_BASE_BYPASS;
	reg |= (conf->divm << CRC_PLLX_BASE_DIVM_SHIFT) &
		CRC_PLLX_BASE_DIVM_MASK;
	reg |= (conf->divn << CRC_PLLX_BASE_DIVN_SHIFT) &
		CRC_PLLX_BASE_DIVN_MASK;
	reg |= (conf->divp << CRC_PLLX_BASE_DIVP_SHIFT) &
		CRC_PLLX_BASE_DIVP_MASK;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_PLLX_BASE);

	/* set chargepump parameters */
	reg = (conf->cpcon << CRC_PLLX_MISC_CPCON_SHIFT) &
		CRC_PLLX_MISC_CPCON_MASK;
	if (conf->divn > 600)
		reg |= CRC_PLLX_MISC_DCCON;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_PLLX_MISC);

	/* enable PLL and disable bypass */
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_PLLX_BASE);
	reg |= CRC_PLLX_BASE_ENABLE;
	reg &= ~CRC_PLLX_BASE_BYPASS;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_PLLX_BASE);

	/* enable PLL lock */
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_PLLX_MISC);
	reg |= CRC_PLLX_MISC_LOCK_ENABLE;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_PLLX_MISC);
}

/* start internal and external clocks to core 0 of the main CPU complex */
static void start_cpu0_clocks(void)
{
	u32 reg;

	/* setup PLLX */
	init_pllx();

	/* setup super CLK */
	writel(CRC_SCLK_BURST_POLICY_SYS_STATE_RUN <<
	       CRC_SCLK_BURST_POLICY_SYS_STATE_SHIFT,
	       TEGRA_CLK_RESET_BASE + CRC_SCLK_BURST_POLICY);
	writel(CRC_SUPER_SDIV_ENB, TEGRA_CLK_RESET_BASE + CRC_SUPER_SCLK_DIV);

	/* deassert clock stop for cpu 0 */
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_CPU_CMPLX);
	reg &= ~CRC_CLK_CPU_CMPLX_CPU0_CLK_STP;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_CPU_CMPLX);

	/* enable main CPU complex clock */
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_L);
	reg |= CRC_CLK_OUT_ENB_L_CPU;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_L);
}

static void maincomplex_powerup(void)
{
	u32 reg;

	if (!(readl(TEGRA_PMC_BASE + PMC_PWRGATE_STATUS) &
	      PMC_PWRGATE_STATUS_CPU)) {
		writel(PMC_PWRGATE_TOGGLE_START | PMC_PWRGATE_TOGGLE_PARTID_CPU,
		       TEGRA_PMC_BASE + PMC_PWRGATE_TOGGLE);

		while (!(readl(TEGRA_PMC_BASE + PMC_PWRGATE_STATUS) &
			 PMC_PWRGATE_STATUS_CPU));

		reg = readl(TEGRA_PMC_BASE + PMC_REMOVE_CLAMPING_CMD);
		reg |= PMC_REMOVE_CLAMPING_CMD_CPU;
		writel(reg, TEGRA_PMC_BASE + PMC_REMOVE_CLAMPING_CMD);
	}
}
void tegra_avp_reset_vector(uint32_t boarddata)
{
	int num_cores;

	/* get the number of cores in the main CPU complex of the current SoC */
	num_cores = tegra_get_num_cores();

	/* bring down main CPU complex (this may be a warm boot) */
	enable_maincomplex_powerrail();
	assert_maincomplex_reset(num_cores);
	stop_maincomplex_clocks(num_cores);

	/* set start address for the main CPU complex processors */
	writel(tegra_maincomplex_entry - get_runtime_offset(),
	       TEGRA_EXCEPTION_VECTORS_BASE + 0x100);

	/* put boarddata in scratch reg, for main CPU to fetch after startup */
	writel(boarddata, TEGRA_PMC_BASE + PMC_SCRATCH(10));

	/* bring up main CPU complex */
	start_cpu0_clocks();
	maincomplex_powerup();
	deassert_cpu0_reset();

	/* assert AVP reset to stop execution here */
	writel(CRC_RST_DEV_L_COP, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_L_SET);

	unreachable();
}
