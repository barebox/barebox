/*
 * Copyright 2008-2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

/*
 * Generic driver for Freescale DDR2 memory controller.
 * Based on code from spd_sdram.c
 * Author: James Yang [at freescale.com]
 */

#include <common.h>
#include <asm/fsl_ddr_sdram.h>
#include "ddr.h"

static void set_csn_config(int dimm_number, int i,
			   struct fsl_ddr_cfg_regs_s *ddr,
			   const struct memctl_options_s *popts,
			   const struct dimm_params_s *dimm)
{
	uint32_t cs_n_en = 0, ap_n_en = 0, odt_rd_cfg = 0, odt_wr_cfg = 0,
		 ba_bits_cs_n = 0, row_bits_cs_n = 0, col_bits_cs_n = 0,
		 n_banks_per_sdram_device;
	int go_config = 0;

	switch (i) {
	case 0:
		if (dimm[dimm_number].n_ranks > 0)
			go_config = 1;
		break;
	case 1:
		if ((dimm_number == 0 && dimm[0].n_ranks > 1) ||
		    (dimm_number == 1 && dimm[1].n_ranks > 0))
			go_config = 1;
		break;
	case 2:
		if ((dimm_number == 0 && dimm[0].n_ranks > 2) ||
		    (dimm_number >= 1 && dimm[dimm_number].n_ranks > 0))
			go_config = 1;
		break;
	case 3:
		if ((dimm_number == 0 && dimm[0].n_ranks > 3) ||
		    (dimm_number == 1 && dimm[1].n_ranks > 1) ||
		    (dimm_number == 3 && dimm[3].n_ranks > 0))
			go_config = 1;
		break;
	default:
		break;
	}

	if (go_config) {
		/* Chip Select enable */
		cs_n_en = 1;
		/* CSn auto-precharge enable */
		ap_n_en = popts->cs_local_opts[i].auto_precharge;
		/* ODT for reads configuration */
		odt_rd_cfg = popts->cs_local_opts[i].odt_rd_cfg;
		/* ODT for writes configuration */
		odt_wr_cfg = popts->cs_local_opts[i].odt_wr_cfg;
		/* Num of bank bits for SDRAM on CSn */
		n_banks_per_sdram_device =
			dimm[dimm_number].n_banks_per_sdram_device;
		ba_bits_cs_n = __ilog2(n_banks_per_sdram_device) - 2;
		/* Num of row bits for SDRAM on CSn */
		row_bits_cs_n = dimm[dimm_number].n_row_addr - 12;
		/* Num of ocl bits for SDRAM on CSn */
		col_bits_cs_n = dimm[dimm_number].n_col_addr - 8;
	}

	ddr->cs[i].config = (((cs_n_en & 0x1) << 31)
			     | ((ap_n_en & 0x1) << 23)
			     | ((odt_rd_cfg & 0x7) << 20)
			     | ((odt_wr_cfg & 0x7) << 16)
			     | ((ba_bits_cs_n & 0x3) << 14)
			     | ((row_bits_cs_n & 0x7) << 8)
			     | ((col_bits_cs_n & 0x7) << 0));
}

static void set_timing_cfg_0(struct fsl_ddr_cfg_regs_s *ddr,
			     const struct memctl_options_s *popts)
{
	uint32_t trwt_mclk = 0;

	if (popts->trwt_override)
		trwt_mclk = popts->trwt;

	ddr->timing_cfg_0 = (((trwt_mclk & 0x3) << 30)
			     | ((popts->txard & 0x7) << 20)
			     | ((popts->txp & 0xF) << 16)
			     | ((popts->taxpd & 0xf) << 8)
			     | ((popts->tmrd & 0xf) << 0));
}

static void set_timing_cfg_3(struct fsl_ddr_cfg_regs_s *ddr,
			     const struct common_timing_params_s *dimm,
			     uint32_t cas_latency)
{
	uint32_t ext_pretoact, ext_acttopre, ext_acttorw, ext_refrec;

	ext_pretoact = picos_to_mclk(dimm->tRP_ps) >> 4;
	ext_acttopre = picos_to_mclk(dimm->tRAS_ps) >> 4;
	ext_acttorw = picos_to_mclk(dimm->tRCD_ps) >> 4;
	cas_latency = ((cas_latency << 1) - 1) >> 4;
	ext_refrec = (picos_to_mclk(dimm->tRFC_ps) - 8) >> 4;

	ddr->timing_cfg_3 = (((ext_pretoact & 0x1) << 28)
			     | ((ext_acttopre & 0x2) << 24)
			     | ((ext_acttorw & 0x1) << 22)
			     | ((ext_refrec & 0x1F) << 16)
			     | ((cas_latency & 0x3) << 12));
}

static void set_timing_cfg_1(struct fsl_ddr_cfg_regs_s *ddr,
			     const struct common_timing_params_s *dimm,
			     uint32_t cas_latency)
{
	uint32_t pretoact_mclk, acttopre_mclk, acttorw_mclk, refrec_ctrl,
		 wrrec_mclk, acttoact_mclk, wrtord_mclk;
	/* DDR_SDRAM_MODE doesn't support 9,11,13,15 */
	static const u8 wrrec_table[] = {
		1, 2, 3, 4, 5, 6, 7, 8, 10, 10, 12, 12, 14, 14, 0, 0
	};

	pretoact_mclk = picos_to_mclk(dimm->tRP_ps);
	acttopre_mclk = picos_to_mclk(dimm->tRAS_ps);
	acttorw_mclk = picos_to_mclk(dimm->tRCD_ps);

	/*
	 * Translate CAS Latency to a DDR controller field value:
	 *
	 *      CAS Lat DDR II  Ctrl
	 *      Clocks  SPD Bit Value
	 *      ------- ------- ------
	 *      1.0             0001
	 *      1.5             0010
	 *      2.0     2       0011
	 *      2.5             0100
	 *      3.0     3       0101
	 *      3.5             0110
	 *      4.0     4       0111
	 *      4.5             1000
	 *      5.0     5       1001
	 */
	cas_latency = (cas_latency << 1) - 1;
	refrec_ctrl = picos_to_mclk(dimm->tRFC_ps) - 8;
	acttoact_mclk = picos_to_mclk(dimm->tRRD_ps);

	wrrec_mclk = picos_to_mclk(dimm->tWR_ps);
	if (wrrec_mclk <= 16)
		wrrec_mclk = wrrec_table[wrrec_mclk - 1];

	wrtord_mclk = picos_to_mclk(dimm->tWTR_ps);
	if (wrtord_mclk < 2)
		wrtord_mclk = 2;

	ddr->timing_cfg_1 = (((pretoact_mclk & 0x0F) << 28)
			     | ((acttopre_mclk & 0x0F) << 24)
			     | ((acttorw_mclk & 0xF) << 20)
			     | ((cas_latency & 0xF) << 16)
			     | ((refrec_ctrl & 0xF) << 12)
			     | ((wrrec_mclk & 0x0F) << 8)
			     | ((acttoact_mclk & 0x0F) << 4)
			     | ((wrtord_mclk & 0x0F) << 0));
}

static void set_timing_cfg_2(struct fsl_ddr_cfg_regs_s *ddr,
			     const struct memctl_options_s *popts,
			     const struct common_timing_params_s *dimm,
			     uint32_t cas_latency, uint32_t additive_latency)
{
	uint32_t cpo, rd_to_pre, wr_data_delay, cke_pls, four_act;

	cpo = popts->cpo_override;
	rd_to_pre = picos_to_mclk(dimm->tRTP_ps);
	if (rd_to_pre < 2)
		rd_to_pre = 2;

	if (additive_latency)
		rd_to_pre += additive_latency;

	wr_data_delay = popts->write_data_delay;
	cke_pls = picos_to_mclk(popts->tCKE_clock_pulse_width_ps);
	four_act = picos_to_mclk(popts->tFAW_window_four_activates_ps);

	ddr->timing_cfg_2 = (((additive_latency & 0xf) << 28)
			     | ((cpo & 0x1f) << 23)
			     | (((cas_latency - 1) & 0xf) << 19)
			     | ((rd_to_pre & 7) << 13)
			     | ((wr_data_delay & 7) << 10)
			     | ((cke_pls & 0x7) << 6)
			     | ((four_act & 0x3f) << 0));
}

static void set_ddr_sdram_cfg(struct fsl_ddr_cfg_regs_s *ddr,
			      const struct memctl_options_s *popts,
			      const struct common_timing_params_s *dimm)
{
	uint32_t mem_en, sren, ecc_en, sdram_type, dyn_pwr, dbw, twoT_en, hse;

	mem_en = 1;
	sren = popts->self_refresh_in_sleep;
	if (dimm->all_DIMMs_ECC_capable)
		ecc_en = popts->ECC_mode;
	else
		ecc_en = 0;

	if (popts->sdram_type)
		sdram_type = popts->sdram_type;
	else
		sdram_type = FSL_SDRAM_TYPE;

	twoT_en = popts->twoT_en;
	dyn_pwr = popts->dynamic_power;
	dbw = popts->data_bus_width;
	hse = popts->half_strength_driver_enable;

	ddr->ddr_sdram_cfg = (((mem_en & 0x1) << 31)
			      | ((sren & 0x1) << 30)
			      | ((ecc_en & 0x1) << 29)
			      | ((sdram_type & 0x7) << 24)
			      | ((dyn_pwr & 0x1) << 21)
			      | ((dbw & 0x3) << 19)
			      | ((twoT_en & 0x1) << 15)
			      | ((hse & 0x1) << 3));
}

static void set_ddr_sdram_cfg_2(struct fsl_ddr_cfg_regs_s *ddr,
				const struct memctl_options_s *popts)
{
	struct ddr_board_info_s *bi = popts->board_info;
	uint32_t i, dll_rst_dis, dqs_cfg, odt_cfg = 0, num_pr, d_init = 0;

	dll_rst_dis = popts->dll_rst_dis;
	dqs_cfg = popts->DQS_config;

	/*
	 * Check for On-Die Termination options and
	 * assert ODT only during reads to DRAM.
	 */
	for (i = 0; i < bi->cs_per_ctrl; i++)
		if (popts->cs_local_opts[i].odt_rd_cfg ||
				popts->cs_local_opts[i].odt_wr_cfg) {
			odt_cfg = SDRAM_CFG2_ODT_ONLY_READ;
			break;
		}

	/* Default number of posted refresh */
	num_pr = 1;

	if (popts->ECC_init_using_memctl) {
		d_init = 1;
		ddr->ddr_data_init = popts->data_init;
	}

	ddr->ddr_sdram_cfg_2 = (((dll_rst_dis & 0x1) << 29)
				| ((dqs_cfg & 0x3) << 26)
				| ((odt_cfg & 0x3) << 21)
				| ((num_pr & 0xf) << 12)
				| ((d_init & 0x1) << 4));
}

static void
set_ddr_sdram_interval(struct fsl_ddr_cfg_regs_s *ddr,
		const struct memctl_options_s *popts,
		const struct common_timing_params_s *dimm)
{
	uint32_t refint, bstopre;

	refint = picos_to_mclk(dimm->refresh_rate_ps);
	/* Precharge interval */
	bstopre = popts->bstopre;

	ddr->ddr_sdram_interval = (((refint & 0xFFFF) << 16)
				   | ((bstopre & 0x3FFF) << 0));
}

static void set_ddr_sdram_mode(struct fsl_ddr_cfg_regs_s *ddr,
			       const struct memctl_options_s *popts,
			       const struct common_timing_params_s *dimm,
			       uint32_t cas_latency,
			       uint32_t additive_latency)
{
	uint16_t esdmode, sdmode;
	uint32_t dqs_en, rtt, al, wr, bl;
	const uint32_t mclk_ps = get_memory_clk_period_ps();

	/* DQS# Enable: 0=enable, 1=disable */
	dqs_en = !popts->DQS_config;
	/* Posted CAS# additive latency (AL) */
	al = additive_latency;
	/* Internal Termination Resistor */
	if (popts->rtt_override)
		rtt = popts->rtt_override_value;
	else
		rtt = popts->cs_local_opts[0].odt_rtt_norm;

	/*
	 * Extended SDRAM mode.
	 * The variable also selects:
	 * - OCD set to exit mode
	 * - all outputs bit i.e DQ, DQS, RDQS output enabled
	 * - RDQS ball disabled
	 * - DQS ball enabled
	 * - DLL enabled
	 * - Output drive strength: full strength.
	 */
	esdmode = (((dqs_en & 0x1) << 10)
		   | ((rtt & 0x2) << 5)
		   | ((al & 0x7) << 3)
		   | ((rtt & 0x1) << 2));

	/* Write recovery */
	wr = ((dimm->tWR_ps + mclk_ps - 1) / mclk_ps) - 1;

	switch (popts->burst_length) {
	case DDR_BL4:
		bl = 2;
		break;
	case DDR_BL8:
		bl = 3;
		break;
	default:
		bl = 2;
		break;
	}

	/* SDRAM mode
	 * The variable also selects:
	 * - power down mode: fast exit (normal)
	 * - DLL reset disabled.
	 * - burst type: sequential
	 */
	sdmode = (((wr & 0x7) << 9)
		  | ((cas_latency & 0x7) << 4)
		  | ((bl & 0x7) << 0));

	ddr->ddr_sdram_mode = (((esdmode & 0xFFFF) << 16)
			       | ((sdmode & 0xFFFF) << 0));
}

uint32_t check_fsl_memctl_config_regs(const struct fsl_ddr_cfg_regs_s *ddr)
{
	/*
	 * DDR_SDRAM_CFG[RD_EN] and DDR_SDRAM_CFG[2T_EN should not
	 * be set at the same time.
	 */
	if ((ddr->ddr_sdram_cfg & 0x10000000) &&
			(ddr->ddr_sdram_cfg & 0x00008000))
		return 1;

	return 0;
}

uint32_t
compute_fsl_memctl_config_regs(const struct memctl_options_s *popts,
			       struct fsl_ddr_cfg_regs_s *ddr,
			       const struct common_timing_params_s *dimm,
			       const struct dimm_params_s *dimmp,
			       uint32_t dbw_cap_adj)
{
	struct ddr_board_info_s *binfo = popts->board_info;
	uint32_t cas_latency, additive_latency, i, cs_per_dimm,
		 dimm_number;
	uint64_t ea, sa, rank_density;

	if (dimm == NULL)
		return 1;

	memset(ddr, 0, sizeof(struct fsl_ddr_cfg_regs_s));

	/* Process overrides first.  */
	if (popts->cas_latency_override)
		cas_latency = popts->cas_latency_override_value;
	else
		cas_latency =  dimm->lowest_common_SPD_caslat;

	if (popts->additive_latency_override)
		additive_latency = popts->additive_latency_override_value;
	else
		additive_latency = dimm->additive_latency;

	/* Chip Select Memory Bounds (CSn_BNDS) */
	for (i = 0; i < binfo->cs_per_ctrl; i++) {
		cs_per_dimm = binfo->cs_per_ctrl / binfo->dimm_slots_per_ctrl;
		dimm_number = i / cs_per_dimm;
		rank_density =
			dimmp[dimm_number].rank_density >> dbw_cap_adj;

		if (dimmp[dimm_number].n_ranks == 0)
			continue;

		sa = dimmp[dimm_number].base_address;
		ea = sa + rank_density - 1;
		if (dimmp[dimm_number].n_ranks > (i % cs_per_dimm)) {
			sa += (i % cs_per_dimm) * rank_density;
			ea += (i % cs_per_dimm) * rank_density;
		} else {
			sa = 0;
			ea = 0;
		}
		sa >>= 24;
		ea >>= 24;

		ddr->cs[i].bnds = (((sa & 0xFFF) << 16) | ((ea & 0xFFF) << 0));
		set_csn_config(dimm_number, i, ddr, popts, dimmp);
	}

	set_timing_cfg_0(ddr, popts);
	set_timing_cfg_3(ddr, dimm, cas_latency);
	set_timing_cfg_1(ddr, dimm, cas_latency);
	set_timing_cfg_2(ddr, popts, dimm, cas_latency, additive_latency);
	set_ddr_sdram_cfg(ddr, popts, dimm);
	set_ddr_sdram_cfg_2(ddr, popts);
	set_ddr_sdram_mode(ddr, popts, dimm, cas_latency, additive_latency);
	set_ddr_sdram_interval(ddr, popts, dimm);

	ddr->ddr_data_init = popts->data_init;
	ddr->ddr_sdram_clk_cntl = (popts->clk_adjust & 0xF) << 23;

	return check_fsl_memctl_config_regs(ddr);
}
