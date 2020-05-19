// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Lucas Stach <l.stach@pengutronix.de>

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
