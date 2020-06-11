// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2013 Lucas Stach <l.stach@pengutronix.de>

#include <common.h>
#include <mach/lowlevel.h>

extern char __dtb_tegra20_paz00_start[];

ENTRY_FUNCTION(start_toshiba_ac100, r0, r1, r2)
{
	tegra_cpu_lowlevel_setup(__dtb_tegra20_paz00_start);

	tegra_avp_reset_vector();
}
