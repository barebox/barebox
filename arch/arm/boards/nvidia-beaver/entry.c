// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Lucas Stach <l.stach@pengutronix.de>

#include <common.h>
#include <mach/lowlevel.h>
#include <mach/lowlevel-dvc.h>

extern char __dtb_tegra30_beaver_start[];

ENTRY_FUNCTION(start_nvidia_beaver, r0, r1, r2)
{
	tegra_cpu_lowlevel_setup(__dtb_tegra30_beaver_start);

	tegra_dvc_init();
	tegra30_tps62366a_ramp_vddcore();
	tegra30_tps65911_cpu_rail_enable();

	tegra_avp_reset_vector();
}
