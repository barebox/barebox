// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 */

#include <common.h>
#include <linux/sizes.h>
#include <soc/imx8m/ddr.h>
#include <soc/imx8m/lpddr4_define.h>

#define USE_ECC
#include "lpddr4-timing-prt8ml.h"

/* ddr timing config params */
struct dram_timing_info prt8ml_dram_ecc_timing = {
	.ddrc_cfg = ddr_ddrc_cfg,
	.ddrc_cfg_num = ARRAY_SIZE(ddr_ddrc_cfg),
	.ddrphy_cfg = ddr_ddrphy_cfg,
	.ddrphy_cfg_num = ARRAY_SIZE(ddr_ddrphy_cfg),
	.fsp_msg = ddr_dram_fsp_msg,
	.fsp_msg_num = ARRAY_SIZE(ddr_dram_fsp_msg),
	.ddrphy_pie = ddr_phy_pie,
	.ddrphy_pie_num = ARRAY_SIZE(ddr_phy_pie),
	.fsp_table = { 4000, 400, 100, },
	.ecc_full_size = SZ_4G + SZ_2G,
};
