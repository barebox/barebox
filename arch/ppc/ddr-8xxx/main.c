/*
 * Copyright 2008-2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

/*
 * Generic driver for Freescale DDR2 memory controller.
 * Based on code from spd_sdram.c
 * Author: James Yang [at freescale.com]
 */

#include <common.h>
#include <config.h>
#include <asm/fsl_law.h>
#include "ddr.h"

static int get_spd(generic_spd_eeprom_t *spd,
		      struct ddr_board_info_s *bi, u8 i2c_address)
{
	int ret;

	fsl_i2c_init(bi->i2c_base, bi->i2c_speed, bi->i2c_slave);
	ret = fsl_i2c_read(bi->i2c_base, i2c_address, 0, 1, (uchar *) spd,
			sizeof(generic_spd_eeprom_t));
	fsl_i2c_stop(bi->i2c_base);

	return ret;
}

int fsl_ddr_get_spd(generic_spd_eeprom_t *ctrl_dimms_spd,
		struct ddr_board_info_s *binfo)
{
	uint32_t i;
	uint8_t i2c_address;

	for (i = 0; i < binfo->dimm_slots_per_ctrl; i++) {
		if (binfo->spd_i2c_addr == NULL)
			goto error;
		i2c_address = binfo->spd_i2c_addr[i];
		if (get_spd(&(ctrl_dimms_spd[i]), binfo, i2c_address))
				goto error;
	}

	return 0;
error:
	return 1;
}

static uint64_t step_assign_addresses(struct fsl_ddr_info_s *pinfo,
		uint32_t dbw_cap_adj)
{
	uint64_t total_mem, current_mem_base, total_ctlr_mem, cap;
	uint32_t ndimm, dw, j, found = 0;

	ndimm = pinfo->board_info.dimm_slots_per_ctrl;
	/*
	 * If a reduced data width is requested, but the SPD
	 * specifies a physically wider device, adjust the
	 * computed dimm capacities accordingly before
	 * assigning addresses.
	 */
	switch (pinfo->memctl_opts.data_bus_width) {
	case 2:
		/* 16-bit */
		for (j = 0; j < ndimm; j++) {
			if (!pinfo->dimm_params[j].n_ranks)
				continue;
			dw = pinfo->dimm_params[j].primary_sdram_width;
			if ((dw == 72 || dw == 64)) {
				dbw_cap_adj = 2;
				break;
			} else if ((dw == 40 || dw == 32)) {
				dbw_cap_adj = 1;
				break;
			}
		}
		break;
	case 1:
		/* 32-bit */
		for (j = 0; j < ndimm; j++) {
			dw = pinfo->dimm_params[j].data_width;
			if (pinfo->dimm_params[j].n_ranks
			    && (dw == 72 || dw == 64)) {
				/*
				 * FIXME: can't really do it
				 * like this because this just
				 * further reduces the memory
				 */
				found = 1;
				break;
			}
		}
		if (found)
			dbw_cap_adj = 1;
		break;
	case 0:
		/* 64-bit */
		break;
	default:
		return 1;
	}

	current_mem_base = 0ull;
	total_mem = 0;
	total_ctlr_mem = 0;
	pinfo->common_timing_params.base_address = current_mem_base;

	for (j = 0; j < ndimm; j++) {
		cap = pinfo->dimm_params[j].capacity >> dbw_cap_adj;
		pinfo->dimm_params[j].base_address = current_mem_base;
		current_mem_base += cap;
		total_ctlr_mem += cap;

	}

	pinfo->common_timing_params.total_mem = total_ctlr_mem;
	total_mem += total_ctlr_mem;

	return total_mem;
}

static uint32_t retrieve_max_size(struct fsl_ddr_cfg_regs_s *ddr_reg,
				uint32_t ncs)
{
	uint32_t max_end = 0, end, i;

	for (i = 0; i < ncs; i++)
		if (ddr_reg->cs[i].config & 0x80000000) {
			end = ddr_reg->cs[i].bnds & 0xFFF;
			if (end > max_end)
				max_end = end;
		}

	return max_end;
}

static uint32_t compute_dimm_param(struct fsl_ddr_info_s *pinfo, uint32_t ndimm)
{
	struct dimm_params_s *pdimm;
	generic_spd_eeprom_t *spd;
	uint32_t i, retval;

	for (i = 0; i < ndimm; i++) {
		spd = &(pinfo->spd_installed_dimms[i]);
		pdimm = &(pinfo->dimm_params[i]);

		retval = compute_dimm_parameters(spd, pdimm);

		if (retval == 2) /* Fatal error */
			return 1;
	}

	return 0;
}

uint64_t fsl_ddr_compute(struct fsl_ddr_info_s *pinfo)
{
	uint64_t total_mem = 0;
	uint32_t ncs, ndimm, max_end = 0;
	struct fsl_ddr_cfg_regs_s *ddr_reg;
	struct common_timing_params_s *timing_params;
	/* data bus width capacity adjust shift amount */
	uint32_t dbw_capacity_adjust;

	ddr_reg = &pinfo->fsl_ddr_config_reg;
	timing_params = &pinfo->common_timing_params;
	ncs = pinfo->board_info.cs_per_ctrl;
	ndimm = pinfo->board_info.dimm_slots_per_ctrl;
	dbw_capacity_adjust = 0;
	pinfo->memctl_opts.board_info = &pinfo->board_info;

	/* STEP 1:  Gather all DIMM SPD data */
	if (fsl_ddr_get_spd(pinfo->spd_installed_dimms,
			pinfo->memctl_opts.board_info))
		return 0;

	/* STEP 2:  Compute DIMM parameters from SPD data */
	if (compute_dimm_param(pinfo, ndimm))
		return 0;

	/*
	 * STEP 3: Compute a common set of timing parameters
	 * suitable for all of the DIMMs on each memory controller
	 */
	compute_lowest_common_dimm_parameters(pinfo->dimm_params,
			timing_params, ndimm);

	/* STEP 4:  Gather configuration requirements from user */
	populate_memctl_options(timing_params->all_DIMMs_registered,
			&pinfo->memctl_opts,
			pinfo->dimm_params);

	/* STEP 5:  Assign addresses to chip selects */
	total_mem = step_assign_addresses(pinfo, dbw_capacity_adjust);

	/* STEP 6:  compute controller register values */
	if (timing_params->ndimms_present == 0)
		memset(ddr_reg, 0, sizeof(struct fsl_ddr_cfg_regs_s));

	compute_fsl_memctl_config_regs(&pinfo->memctl_opts,
				       ddr_reg, timing_params,
				       pinfo->dimm_params,
				       dbw_capacity_adjust);

	max_end = retrieve_max_size(ddr_reg, ncs);
	total_mem = 1 + (((uint64_t)max_end << 24ULL) | 0xFFFFFFULL);

	return total_mem;
}

/*
 * fsl_ddr_sdram():
 * This is the main function to initialize the memory.
 *
 * It returns amount of memory configured in bytes.
 */
phys_size_t fsl_ddr_sdram(void)
{
	uint64_t total_memory;
	struct fsl_ddr_info_s info;

	memset(&info, 0, sizeof(struct fsl_ddr_info_s));
	/* Gather board information on the  memory controller and I2C bus. */
	fsl_ddr_board_info(&info.board_info);

	total_memory = fsl_ddr_compute(&info);

	if (info.common_timing_params.ndimms_present == 0)
		return 0;

	fsl_ddr_set_memctl_regs(&info);
	fsl_ddr_set_lawbar(&info.common_timing_params, LAW_TRGT_IF_DDR_1);

	return total_memory;
}
