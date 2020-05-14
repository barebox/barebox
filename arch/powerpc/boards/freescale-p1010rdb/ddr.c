/*
 * Copyright 2010 Freescale Semiconductor, Inc.
 * Authors: Srikanth Srinivasan <srikanth.srinivasan@freescale.com>
 *          Timur Tabi <timur@freescale.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <init.h>
#include <mach/fsl_i2c.h>
#include <mach/immap_85xx.h>
#include <mach/clock.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/fsl_lbc.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>
#include "p1010rdb.h"

static const u8 spd_addr = 0x52;

int fsl_ddr_board_info(struct ddr_board_info_s *info)
{
	p1010rdb_early_init();

	info->fsl_ddr_ver = 0;
	info->ddr_base = IOMEM(MPC85xx_DDR_ADDR);
	/* Actual number of chip select used */
	info->cs_per_ctrl = CFG_CHIP_SELECTS_PER_CTRL;
	info->dimm_slots_per_ctrl = 1;
	info->i2c_bus = 1;
	info->i2c_slave = 0x7f;
	info->i2c_speed = 400000;
	info->i2c_base = IOMEM(I2C2_BASE_ADDR);
	info->spd_i2c_addr = &spd_addr;

	return 0;
}

void fsl_ddr_board_options(struct memctl_options_s *popts,
		struct dimm_params_s *pdimm)
{
	popts->cs_local_opts[0].odt_rd_cfg = FSL_DDR_ODT_NEVER;
	popts->cs_local_opts[0].odt_wr_cfg = FSL_DDR_ODT_CS;
	popts->cs_local_opts[0].odt_rtt_norm = DDR3_RTT_40_OHM;
	popts->cs_local_opts[0].odt_rtt_wr = DDR3_RTT_OFF;

	popts->clk_adjust = 6;
	popts->cpo_override = 0x1f;
	popts->write_data_delay = 2;
	/* Write leveling override */
	popts->wrlvl_en = 1;
	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0xf;
	popts->wrlvl_start = 0x8;
	popts->trwt_override = 1;
	popts->trwt = 0;
	popts->dll_rst_dis = 1;
}
