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

void tegra_maincomplex_entry(void)
{
	uint32_t rambase, ramsize;

	arm_cpu_lowlevel_init();

	switch (tegra_get_chiptype()) {
	case TEGRA20:
		rambase = 0x0;
		ramsize = tegra20_get_ramsize();
		break;
	default:
		/* If we don't know the chiptype, better bail out */
		BUG();
	}

	/*
	 * The standard load address for Tegra systems is 0x10800 which means
	 * the barebox binary will always be below the malloc area for all
	 * reasonable malloc area sizes. We offset the RAM base address by 8MB
	 * to pretend barebox is in another bank.
	 */
	barebox_arm_entry(rambase + SZ_8M, ramsize - SZ_8M, 0);
}
