/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
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
#include <mach/lowlevel.h>
#include <mach/lowlevel-dvc.h>

extern char __dtb_tegra124_jetson_tk1_start[];

ENTRY_FUNCTION(start_nvidia_jetson, r0, r1, r2)
{
	tegra_cpu_lowlevel_setup(__dtb_tegra124_jetson_tk1_start);

	tegra_dvc_init();
	tegra124_dvc_pinmux();
	tegra124_as3722_enable_essential_rails(0x3c00);

	tegra_avp_reset_vector();
}
