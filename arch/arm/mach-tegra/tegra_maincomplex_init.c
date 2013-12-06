/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
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
#include <mach/lowlevel.h>
#include <mach/tegra20-pmc.h>
#include <mach/tegra20-car.h>

void tegra_maincomplex_entry(void)
{
	uint32_t rambase, ramsize;

	arm_cpu_lowlevel_init();

	/* switch to PLLX */
	writel(CRC_CCLK_BURST_POLICY_SYS_STATE_RUN <<
	       CRC_CCLK_BURST_POLICY_SYS_STATE_SHIFT |
	       CRC_CCLK_BURST_POLICY_SRC_PLLX_OUT0 <<
	       CRC_CCLK_BURST_POLICY_RUN_SRC_SHIFT,
	       TEGRA_CLK_RESET_BASE + CRC_CCLK_BURST_POLICY);
	writel(CRC_SUPER_CDIV_ENB, TEGRA_CLK_RESET_BASE + CRC_SUPER_CCLK_DIV);

	switch (tegra_get_chiptype()) {
	case TEGRA20:
		rambase = 0x0;
		ramsize = tegra20_get_ramsize();
		break;
	default:
		/* If we don't know the chiptype, better bail out */
		unreachable();
	}

	barebox_arm_entry(rambase, ramsize,
			  readl(TEGRA_PMC_BASE + PMC_SCRATCH(10)));
}
