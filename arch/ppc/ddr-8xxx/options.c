/*
 * Copyright 2008, 2010-2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <common.h>
#include <asm/fsl_ddr_sdram.h>
#include "ddr.h"

uint32_t populate_memctl_options(int all_DIMMs_registered,
		struct memctl_options_s *popts,
		struct dimm_params_s *pdimm)
{
	const struct ddr_board_info_s *binfo = popts->board_info;
	uint32_t i;

	for (i = 0; i < binfo->cs_per_ctrl; i++) {
		popts->cs_local_opts[i].odt_rd_cfg = FSL_DDR_ODT_NEVER;
		popts->cs_local_opts[i].odt_wr_cfg = FSL_DDR_ODT_ALL;
		popts->cs_local_opts[i].odt_rtt_norm = DDR2_RTT_50_OHM;
		popts->cs_local_opts[i].odt_rtt_wr = DDR2_RTT_OFF;
		popts->cs_local_opts[i].auto_precharge = 0;
	}

	/* Memory Organization Parameters */
	popts->registered_dimm_en = all_DIMMs_registered;
	popts->ECC_mode = 0;		  /* 0 = disabled, 1 = enabled */
	popts->ECC_init_using_memctl = 1; /* 0 = use DMA, 1 = use memctl */

	/* Choose DQS config - 1 for DDR2 */
	popts->DQS_config = 1;

	/* Choose self-refresh during sleep. */
	popts->self_refresh_in_sleep = 1;

	/* Choose dynamic power management mode. */
	popts->dynamic_power = 0;

	/*
	 * check first dimm for primary sdram width
	 * assuming all dimms are similar
	 * 0 = 64-bit, 1 = 32-bit, 2 = 16-bit
	 */
	if (pdimm->n_ranks != 0) {
		if ((pdimm->data_width >= 64) && (pdimm->data_width <= 72))
			popts->data_bus_width = 0;
		else if ((pdimm->data_width >= 32) ||
			(pdimm->data_width <= 40))
			popts->data_bus_width = 1;
		else
			panic("data width %u is invalid!\n",
					pdimm->data_width);
	}

	/* Must be a burst length of 4 for DD2 */
	popts->burst_length = DDR_BL4;
	/* Decide whether to use the computed de-rated latency */
	popts->use_derated_caslat = 0;

	/*
	 * 2T_EN setting
	 *
	 * Factors to consider for 2T_EN:
	 *	- number of DIMMs installed
	 *	- number of components, number of active ranks
	 *	- how much time you want to spend playing around
	 */
	popts->twoT_en = 0;

	/*
	 * Default BSTTOPRE precharge interval
	 *
	 * Set the parameter to 0 for global auto precharge in
	 * the board options function.
	 */
	popts->bstopre = 0x100;

	/* Minimum CKE pulse width -- tCKE(MIN) */
	popts->tCKE_clock_pulse_width_ps
		= mclk_to_picos(FSL_DDR_MIN_TCKE_PULSE_WIDTH_DDR);

	/*
	 * Window for four activates -- tFAW
	 *
	 * Set according to specification for the memory used.
	 * The default value below would work for x4/x8 wide memory.
	 *
	 */
	popts->tFAW_window_four_activates_ps = 37500;

	/*
	 * Default powerdown exit timings.
	 * Set according to specifications for the memory used in
	 * the board options function.
	 */
	popts->txard = 3;
	popts->txp = 3;
	popts->taxpd = 11;

	/* Default value for load mode cycle time */
	popts->tmrd = 2;

	/* Specific board override parameters. */
	fsl_ddr_board_options(popts, pdimm);

	return 0;
}
