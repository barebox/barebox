// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2013 Lucas Stach <l.stach@pengutronix.de>

#include <common.h>
#include <mach/lowlevel.h>

extern char __dtb_tegra20_colibri_iris_start[];

static void common_toradex_colibri_t20_iris_start(void)
{
	tegra_cpu_lowlevel_setup(__dtb_tegra20_colibri_iris_start);

	tegra_avp_reset_vector();
}

ENTRY_FUNCTION(start_colibri_t20_256_usbload, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_256_hsmmc, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_256_v11_nand, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_256_v12_nand, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_512_usbload, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_512_hsmmc, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_512_v11_nand, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}

ENTRY_FUNCTION(start_colibri_t20_512_v12_nand, r0, r1, r2)
{
	common_toradex_colibri_t20_iris_start();
}
