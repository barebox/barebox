/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2008-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP Semiconductor
 */

#ifndef _DDR_DIMMS_H_
#define _DDR_DIMMS_H_

#include <ddr_spd.h>

/* Parameters for a DDR dimm computed from the SPD */
struct dimm_params {

	/* DIMM organization parameters */
	char mpart[19];		/* guaranteed null terminated */

	unsigned int n_ranks;
	unsigned int die_density;
	unsigned long long rank_density;
	unsigned long long capacity;
	unsigned int data_width;
	unsigned int primary_sdram_width;
	unsigned int ec_sdram_width;
	unsigned int registered_dimm;
	unsigned int package_3ds;	/* number of dies in 3DS DIMM */
	unsigned int device_width;	/* x4, x8, x16 components */

	/* SDRAM device parameters */
	unsigned int n_row_addr;
	unsigned int n_col_addr;
	unsigned int edc_config;	/* 0 = none, 1 = parity, 2 = ECC */
	unsigned int bank_addr_bits; /* DDR4 */
	unsigned int bank_group_bits; /* DDR4 */
	unsigned int n_banks_per_sdram_device; /* !DDR4 */
	unsigned int burst_lengths_bitmask;	/* BL=4 bit 2, BL=8 = bit 3 */

	/* used in computing base address of DIMMs */
	unsigned long long base_address;
	/* mirrored DIMMs */
	unsigned int mirrored_dimm;	/* only for ddr3 */

	/* DIMM timing parameters */

	int mtb_ps;	/* medium timebase ps */
	int ftb_10th_ps; /* fine timebase, in 1/10 ps */
	int taa_ps;	/* minimum CAS latency time */
	int tfaw_ps;	/* four active window delay */

	/*
	 * SDRAM clock periods
	 * The range for these are 1000-10000 so a short should be sufficient
	 */
	int tckmin_x_ps;
	int tckmin_x_minus_1_ps;
	int tckmin_x_minus_2_ps;
	int tckmax_ps;

	/* SPD-defined CAS latencies */
	unsigned int caslat_x;
	unsigned int caslat_x_minus_1;
	unsigned int caslat_x_minus_2;

	unsigned int caslat_lowest_derated;	/* Derated CAS latency */

	/* basic timing parameters */
	int trcd_ps;
	int trp_ps;
	int tras_ps;

	int trfc1_ps; /* DDR4 */
	int trfc2_ps; /* DDR4 */
	int trfc4_ps; /* DDR4 */
	int trrds_ps; /* DDR4 */
	int trrdl_ps; /* DDR4 */
	int tccdl_ps; /* DDR4 */
	int trfc_slr_ps; /* DDR4 */
	int twr_ps;	/* !DDR4, maximum = 63750 ps */
	int trfc_ps;	/* max = 255 ns + 256 ns + .75 ns
				       = 511750 ps */
	int trrd_ps;	/* !DDR4, maximum = 63750 ps */
	int twtr_ps;	/* !DDR4, maximum = 63750 ps */
	int trtp_ps;	/* !DDR4, byte 38, spd->trtp */

	int trc_ps;	/* maximum = 254 ns + .75 ns = 254750 ps */

	int refresh_rate_ps;
	int extended_op_srt;

	int tis_ps;	/* DDR1, DDR2, byte 32, spd->ca_setup */
	int tih_ps;	/* DDR1, DDR2, byte 33, spd->ca_hold */
	int tds_ps;	/* DDR1, DDR2, byte 34, spd->data_setup */
	int tdh_ps;	/* DDR1, DDR2, byte 35, spd->data_hold */
	int tdqsq_max_ps;	/* DDR1, DDR2, byte 44, spd->tdqsq */
	int tqhs_ps;	/* DDR1, DDR2, byte 45, spd->tqhs */

	/* DDR3 & DDR4 RDIMM */
	unsigned char rcw[16];	/* Register Control Word 0-15 */
	unsigned int dq_mapping[18]; /* DDR4 */
	unsigned int dq_mapping_ors; /* DDR4 */
};

unsigned int ddr1_compute_dimm_parameters(unsigned int mclk_ps,
					  const struct ddr1_spd_eeprom *spd,
					  struct dimm_params *pdimm);
unsigned int ddr2_compute_dimm_parameters(unsigned int mclk_ps,
					  const struct ddr2_spd_eeprom *spd,
					  struct dimm_params *pdimm);
unsigned int ddr3_compute_dimm_parameters(const struct ddr3_spd_eeprom *spd,
					  struct dimm_params *pdimm);
unsigned int ddr4_compute_dimm_parameters(const struct ddr4_spd_eeprom *spd,
					  struct dimm_params *pdimm);

#endif /* _DDR_DIMMS_H_ */
