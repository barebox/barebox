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
#include <asm/fsl_lbc.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>
#include "p1022ds.h"

static const u8 spd_addr = 0x51;

int fsl_ddr_board_info(struct ddr_board_info_s *info)
{
	/*
	 * Early mapping is needed to access the clock
	 * parameters in the FPGA.
	 */
	p1022ds_lbc_early_init();

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

struct board_specific_parameters {
	u32 n_ranks;
	u32 datarate_mhz_high;
	u32 clk_adjust;		/* Range: 0-8 */
	u32 cpo;		/* Range: 2-31 */
	u32 write_data_delay;	/* Range: 0-6 */
	u32 force_2t;
};

/*
 * This table contains all valid speeds we want to override with board
 * specific parameters. datarate_mhz_high values need to be in ascending order
 * for each n_ranks group.
 */
static const struct board_specific_parameters dimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi|  clk| cpo|wrdata|2T
	 * ranks| mhz|adjst|    | delay|
	 */
	{ 1, 549, 5, 31, 3, 0 },
	{ 1, 850, 5, 31, 5, 0 },
	{ 2, 549, 5, 31, 3, 0 },
	{ 2, 850, 5, 31, 5, 0 },
	{ }
};

void fsl_ddr_board_options(struct memctl_options_s *popts,
		struct dimm_params_s *pdimm)
{
	const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
	unsigned long ddr_freq;
	uint32_t i;

	for (i = 0; i < popts->board_info->cs_per_ctrl; i++) {
		popts->cs_local_opts[i].odt_rd_cfg = 0;
		popts->cs_local_opts[i].odt_wr_cfg = 1;
		popts->cs_local_opts[i].odt_rtt_wr = DDR3_RTT_OFF;
	}
	popts->cs_local_opts[0].odt_rtt_norm = DDR3_RTT_40_OHM;
	popts->cs_local_opts[1].odt_rtt_norm = DDR3_RTT_OFF;

	pbsp = dimm0;

	ddr_freq = fsl_get_ddr_freq(0) / 1000000;
	/*
	 * To have optimal parameters specific to the board, do a fine
	 * adjustment of DDR parameters depending on the DDR data rate.
	 */
	while (pbsp->datarate_mhz_high) {
		if (pbsp->n_ranks == pdimm->n_ranks) {
			if (ddr_freq <= pbsp->datarate_mhz_high) {
				popts->clk_adjust = pbsp->clk_adjust;
				popts->cpo_override = pbsp->cpo;
				popts->write_data_delay =
					pbsp->write_data_delay;
				popts->twoT_en = pbsp->force_2t;
				goto found;
			}
			pbsp_highest = pbsp;
		}
		pbsp++;
	}

	/* Use highest parameters if none were found */
	if (pbsp_highest) {
		popts->clk_adjust = pbsp->clk_adjust;
		popts->cpo_override = pbsp->cpo;
		popts->write_data_delay = pbsp->write_data_delay;
		popts->twoT_en = pbsp->force_2t;
	}

found:
	popts->half_strength_driver_enable = 1;

	/* Per AN4039, enable ZQ calibration. */
	popts->zq_en = 1;

	popts->auto_self_refresh_en = 1;
	popts->sr_it = 0xb;

	popts->dll_rst_dis = 1;
}
