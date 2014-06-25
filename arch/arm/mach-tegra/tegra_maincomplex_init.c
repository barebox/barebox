/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
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
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/errata.h>
#include <mach/lowlevel.h>
#include <mach/tegra20-pmc.h>
#include <mach/tegra20-car.h>

void tegra_maincomplex_entry(void)
{
	uint32_t rambase, ramsize;
	enum tegra_chiptype chiptype;
	u32 reg = 0;

	arm_cpu_lowlevel_init();

	chiptype = tegra_get_chiptype();

	/* enable ARM errata workarounds early */
	switch (chiptype) {
	case TEGRA20:
		enable_arm_errata_716044_war();
		enable_arm_errata_742230_war();
		enable_arm_errata_751472_war();
		break;
	case TEGRA30:
		enable_arm_errata_743622_war();
		enable_arm_errata_751472_war();
		break;
	default:
		break;
	}

	/* switch to PLLX */
	writel(CRC_CCLK_BURST_POLICY_SYS_STATE_RUN <<
	       CRC_CCLK_BURST_POLICY_SYS_STATE_SHIFT |
	       CRC_CCLK_BURST_POLICY_SRC_PLLX_OUT0 <<
	       CRC_CCLK_BURST_POLICY_RUN_SRC_SHIFT,
	       TEGRA_CLK_RESET_BASE + CRC_CCLK_BURST_POLICY);
	writel(CRC_SUPER_CDIV_ENB, TEGRA_CLK_RESET_BASE + CRC_SUPER_CCLK_DIV);

	if (chiptype >= TEGRA114) {
		asm("mrc p15, 1, %0, c9, c0, 2" : : "r" (reg));
		reg &= ~7;
		reg |= 2;
		asm("mcr p15, 1, %0, c9, c0, 2" : : "r" (reg));
	}

	switch (chiptype) {
	case TEGRA20:
		rambase = 0x0;
		ramsize = tegra20_get_ramsize();

		break;
	case TEGRA30:
	case TEGRA124:
		rambase = SZ_2G;
		ramsize = tegra30_get_ramsize();
		break;
	default:
		/* If we don't know the chiptype, better bail out */
		unreachable();
	}

	barebox_arm_entry(rambase, ramsize,
			  (void *)readl(TEGRA_PMC_BASE + PMC_SCRATCH(10)));
}
