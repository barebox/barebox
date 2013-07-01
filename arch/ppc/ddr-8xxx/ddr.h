/*
 * Copyright 2013 GE Intelligent Platforms, Inc
 * Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef FSL_DDR_MAIN_H
#define FSL_DDR_MAIN_H

#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>
#include <mach/fsl_i2c.h>
#include <mach/clock.h>
#include "common_timing_params.h"

#ifdef CONFIG_MPC85xx
#include <mach/immap_85xx.h>
#endif

/* Record of computed register values. */
struct fsl_ddr_cfg_regs_s {
	struct {
		uint32_t bnds;
		uint32_t config;
		uint32_t config_2;
	} cs[MAX_CHIP_SELECTS_PER_CTRL];
	uint32_t timing_cfg_3;
	uint32_t timing_cfg_0;
	uint32_t timing_cfg_1;
	uint32_t timing_cfg_2;
	uint32_t ddr_sdram_cfg;
	uint32_t ddr_sdram_cfg_2;
	uint32_t ddr_sdram_mode;
	uint32_t ddr_sdram_mode_2;
	uint32_t ddr_sdram_mode_3;
	uint32_t ddr_sdram_mode_4;
	uint32_t ddr_sdram_mode_5;
	uint32_t ddr_sdram_mode_6;
	uint32_t ddr_sdram_mode_7;
	uint32_t ddr_sdram_mode_8;
	uint32_t ddr_sdram_md_cntl;
	uint32_t ddr_sdram_interval;
	uint32_t ddr_data_init;
	uint32_t ddr_sdram_clk_cntl;
	uint32_t ddr_init_addr;
	uint32_t ddr_init_ext_addr;
	uint32_t err_disable;
	uint32_t err_int_en;
	uint32_t debug[32];
};

/*
 * Data Structures
 *
 * All data structures have to be on the stack
 */
struct fsl_ddr_info_s {
	generic_spd_eeprom_t
	spd_installed_dimms[MAX_DIMM_SLOTS_PER_CTLR];
	struct dimm_params_s
		dimm_params[MAX_DIMM_SLOTS_PER_CTLR];
	struct memctl_options_s memctl_opts;
	struct common_timing_params_s common_timing_params;
	struct fsl_ddr_cfg_regs_s fsl_ddr_config_reg;
	struct ddr_board_info_s board_info;
};

uint32_t mclk_to_picos(uint32_t mclk);
uint32_t get_memory_clk_period_ps(void);
uint32_t picos_to_mclk(uint32_t picos);
uint32_t check_fsl_memctl_config_regs(const struct fsl_ddr_cfg_regs_s *ddr);
uint64_t fsl_ddr_compute(struct fsl_ddr_info_s *pinfo);
uint32_t compute_fsl_memctl_config_regs(
		const struct memctl_options_s *popts,
		struct fsl_ddr_cfg_regs_s *ddr,
		const struct common_timing_params_s *common_dimm,
		const struct dimm_params_s *dimm_parameters,
		uint32_t dbw_capacity_adjust);
uint32_t compute_dimm_parameters(
		const generic_spd_eeprom_t *spdin,
		struct dimm_params_s *pdimm);
uint32_t compute_lowest_common_dimm_parameters(
		const struct dimm_params_s *dimm_params,
		struct common_timing_params_s *outpdimm,
		uint32_t number_of_dimms);
uint32_t populate_memctl_options(
		int all_DIMMs_registered,
		struct memctl_options_s *popts,
		struct dimm_params_s *pdimm);
int fsl_ddr_set_lawbar(
		const struct common_timing_params_s *memctl_common_params,
		uint32_t memctl_interleaved);
int fsl_ddr_get_spd(
		generic_spd_eeprom_t *ctrl_dimms_spd,
		struct ddr_board_info_s *binfo);
int fsl_ddr_set_memctl_regs(
		const struct fsl_ddr_info_s *info);
void fsl_ddr_board_options(
		struct memctl_options_s *popts,
		struct dimm_params_s *pdimm);
void fsl_ddr_board_info(struct ddr_board_info_s *info);
#endif
