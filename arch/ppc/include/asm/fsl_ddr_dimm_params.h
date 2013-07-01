/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#ifndef DDR2_DIMM_PARAMS_H
#define DDR2_DIMM_PARAMS_H

#define EDC_ECC		2

/* Parameters for a DDR2 dimm computed from the SPD */
struct dimm_params_s {
	char mpart[19];
	uint32_t n_ranks;
	uint64_t rank_density;
	uint64_t capacity;
	uint32_t data_width;
	uint32_t primary_sdram_width;
	uint32_t ec_sdram_width;
	uint32_t registered_dimm;
	uint32_t n_row_addr;
	uint32_t n_col_addr;
	uint32_t edc_config;		/* 0 = none, 1 = parity, 2 = ECC */
	uint32_t n_banks_per_sdram_device;
	uint32_t burst_lengths_bitmask;	/* BL=4 bit 2, BL=8 = bit 3 */
	uint32_t row_density;
	uint64_t base_address;
	/* SDRAM clock periods */
	uint32_t tCKmin_X_ps;
	uint32_t tCKmin_X_minus_1_ps;
	uint32_t tCKmin_X_minus_2_ps;
	uint32_t tCKmax_ps;
	/* SPD-defined CAS latencies */
	uint32_t caslat_X;
	uint32_t caslat_X_minus_1;
	uint32_t caslat_X_minus_2;
	uint32_t caslat_lowest_derated;
	/* basic timing parameters */
	uint32_t tRCD_ps;
	uint32_t tRP_ps;
	uint32_t tRAS_ps;
	uint32_t tWR_ps;	/* maximum = 63750 ps */
	uint32_t tWTR_ps;	/* maximum = 63750 ps */
	uint32_t tRFC_ps;	/* max = 255 ns + 256 ns + .75 ns = 511750 ps */
	uint32_t tRRD_ps;	/* maximum = 63750 ps */
	uint32_t tRC_ps;	/* maximum = 254 ns + .75 ns = 254750 ps */
	uint32_t refresh_rate_ps;
	uint32_t tIS_ps;	/* byte 32, spd->ca_setup */
	uint32_t tIH_ps;	/* byte 33, spd->ca_hold */
	uint32_t tDS_ps;	/* byte 34, spd->data_setup */
	uint32_t tDH_ps;	/* byte 35, spd->data_hold */
	uint32_t tRTP_ps;	/* byte 38, spd->trtp */
	uint32_t tDQSQ_max_ps;	/* byte 44, spd->tdqsq */
	uint32_t tQHS_ps;	/* byte 45, spd->tqhs */
};

#endif
