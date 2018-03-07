/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
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
#include <mach/tegra30-car.h>
#include <mach/tegra30-flow.h>
#include <mach/tegra124-car.h>

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

	for (i = 0; i < num_cores; i++) {
		if (tegra_get_chiptype() >= TEGRA114)
			mask |= 0x111001 << i;
		else
			mask |= 0x1111 << i;
	}

	writel(mask, TEGRA_CLK_RESET_BASE + CRC_RST_CPU_CMPLX_SET);
	writel(CRC_RST_DEV_L_CPU, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_L_SET);
}

/* release reset state of the first core of the main CPU complex */
static void deassert_cpu0_reset(void)
{
	u32 reg;

	if (tegra_get_chiptype() >= TEGRA114)
		reg = 0x21fff00f;
	else
		reg = 0x1111;

	writel(reg, TEGRA_CLK_RESET_BASE + CRC_RST_CPU_CMPLX_CLR);
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
	{
		{600,  13, 0, 8 },
		{500,  16, 0, 8 },
		{600,  12, 0, 8 },
		{600,  26, 0, 8 },
	}, /* TEGRA 30 */
	{
	}, /* TEGRA 114 */
	{
		{ 108,  1, 1, 0 },
		{  73,  1, 1, 0 },
		{ 116,  1, 1, 0 },
		{ 108,  2, 1, 0 },
	}, /* TEGRA 124 */
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

	/* disable IDDQ on T124 */
	if (chiptype == TEGRA124) {
		reg = readl(TEGRA_CLK_RESET_BASE + CRC_PLLX_MISC_3);
		reg &= ~CRC_PLLX_MISC_3_IDDQ;
		writel(reg, TEGRA_CLK_RESET_BASE + CRC_PLLX_MISC_3);
		tegra_ll_delay_usec(2);
	}

	osc_freq = (readl(TEGRA_CLK_RESET_BASE + CRC_OSC_CTRL) &
		    CRC_OSC_CTRL_OSC_FREQ_MASK) >> CRC_OSC_CTRL_OSC_FREQ_SHIFT;

	conf = &pllx_config_table[chiptype][osc_freq];
	/* we are not relocated yet - globals are a bit more tricky here */
	conf = (struct pll_config *)((char *)conf + get_runtime_offset());

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

	writel(1 << CRC_CLK_SYSTEM_RATE_AHB_SHIFT,
	       TEGRA_CLK_RESET_BASE + CRC_CLK_SYSTEM_RATE);

	if (tegra_get_chiptype() >= TEGRA30) {
		/* init MSELECT */
		writel(CRC_RST_DEV_V_MSELECT,
		       TEGRA_CLK_RESET_BASE + CRC_RST_DEV_V_SET);
		writel((CRC_CLK_SOURCE_MSEL_SRC_CLKM <<
		       CRC_CLK_SOURCE_MSEL_SRC_SHIFT),
		       TEGRA_CLK_RESET_BASE + CRC_CLK_SOURCE_MSEL);
		writel(CRC_CLK_OUT_ENB_V_MSELECT,
		       TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_V_SET);
		tegra_ll_delay_usec(3);
		writel(CRC_RST_DEV_V_MSELECT,
		       TEGRA_CLK_RESET_BASE + CRC_RST_DEV_V_CLR);
	}

	/* deassert clock stop for cpu 0 */
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_CPU_CMPLX);
	reg &= ~(0xf << 8);
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_CPU_CMPLX);

	/* enable main CPU complex clock */
	reg = readl(TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_L);
	reg |= CRC_CLK_OUT_ENB_L_CPU;
	writel(reg, TEGRA_CLK_RESET_BASE + CRC_CLK_OUT_ENB_L);

	/* give clocks some time to settle */
	tegra_ll_delay_usec(300);
}

static void power_up_partition(u32 partid)
{
	u32 reg;

	if (!(readl(TEGRA_PMC_BASE + PMC_PWRGATE_STATUS) & (1 << partid))) {
		writel(PMC_PWRGATE_TOGGLE_START | partid,
		       TEGRA_PMC_BASE + PMC_PWRGATE_TOGGLE);

		while (!(readl(TEGRA_PMC_BASE + PMC_PWRGATE_STATUS) &
			(1 << partid)));

		reg = readl(TEGRA_PMC_BASE + PMC_REMOVE_CLAMPING_CMD);
		reg |= (1 << partid);
		writel(reg, TEGRA_PMC_BASE + PMC_REMOVE_CLAMPING_CMD);

		tegra_ll_delay_usec(1000);
	}
}

static void maincomplex_powerup(void)
{
	/* main cpu rail */
	power_up_partition(PMC_PARTID_CRAIL);

	if (tegra_get_chiptype() >= TEGRA114) {
		/* fast cluster uncore part */
		power_up_partition(PMC_PARTID_C0NC);
		/* fast cluster cpu0 part */
		power_up_partition(PMC_PARTID_CE0);
	}
}

static void tegra_cluster_switch_hp(void)
{
	u32 reg;

	reg = readl(TEGRA_FLOW_CTRL_BASE + FLOW_CLUSTER_CONTROL);
	reg &= ~FLOW_CLUSTER_CONTROL_ACTIVE_LP;
	writel(reg, TEGRA_FLOW_CTRL_BASE + FLOW_CLUSTER_CONTROL);
}

void tegra_avp_reset_vector(void)
{
	int num_cores;
	unsigned int entry_address = 0;

	/* we want to bring up the high performance CPU complex */
	if (tegra_get_chiptype() >= TEGRA30)
		tegra_cluster_switch_hp();

	/* get the number of cores in the main CPU complex of the current SoC */
	num_cores = tegra_get_num_cores();

	/* bring down main CPU complex (this may be a warm boot) */
	enable_maincomplex_powerrail();
	assert_maincomplex_reset(num_cores);
	stop_maincomplex_clocks(num_cores);

	/* set start address for the main CPU complex processors */
	switch (tegra_get_chiptype()) {
	case TEGRA20:
		entry_address = 0x108000;
		break;
	case TEGRA30:
	case TEGRA124:
		entry_address = 0x80108000;
		break;
	default:
		break;
	}
	writel(entry_address, TEGRA_EXCEPTION_VECTORS_BASE + 0x100);

	/* bring up main CPU complex */
	start_cpu0_clocks();
	maincomplex_powerup();
	deassert_cpu0_reset();

	/* assert AVP reset to stop execution here */
	writel(CRC_RST_DEV_L_COP, TEGRA_CLK_RESET_BASE + CRC_RST_DEV_L_SET);

	unreachable();
}
