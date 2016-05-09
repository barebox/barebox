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
#include <init.h>
#include <asm/memory.h>
#include <mach/iomap.h>
#include <mach/lowlevel.h>
#include <mach/tegra114-sysctr.h>

static int tegra20_mem_init(void)
{
	if (!of_machine_is_compatible("nvidia,tegra20"))
		return 0;

	arm_add_mem_device("ram0", 0x0, tegra20_get_ramsize());

	return 0;
}
mem_initcall(tegra20_mem_init);

static int tegra30_mem_init(void)
{
	if (!of_machine_is_compatible("nvidia,tegra30") &&
	    !of_machine_is_compatible("nvidia,tegra124"))
		return 0;

	arm_add_mem_device("ram0", SZ_2G, tegra30_get_ramsize());

	return 0;
}
mem_initcall(tegra30_mem_init);

static int tegra114_architected_timer_init(void)
{
	u32 freq, reg;

	if (!of_machine_is_compatible("nvidia,tegra114") &&
	    !of_machine_is_compatible("nvidia,tegra124"))
		return 0;

	freq = tegra_get_osc_clock();

	/* ARM CNTFRQ */
	asm("mcr p15, 0, %0, c14, c0, 0\n" : : "r" (freq));

	/* Tegra specific SYSCTR */
	writel(freq, TEGRA_SYSCTR0_BASE + TEGRA_SYSCTR0_CNTFID0);

	reg = readl(TEGRA_SYSCTR0_BASE + TEGRA_SYSCTR0_CNTCR);
	reg |= TEGRA_SYSCTR0_CNTCR_ENABLE | TEGRA_SYSCTR0_CNTCR_HDBG;
	writel(reg, TEGRA_SYSCTR0_BASE + TEGRA_SYSCTR0_CNTCR);

	return 0;
}
coredevice_initcall(tegra114_architected_timer_init);
