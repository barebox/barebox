/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <asm/fsl_ddr_sdram.h>
#include "ddr.h"
/*
 * Calculate the Density of each Physical Rank.
 * Returned size is in bytes.
 *
 * Table comes from Byte 31 of JEDEC SPD Spec.
 *
 *		DDR II
 *	Bit	Size	Size
 *	---	-----
 *	7 high	512MB
 *	6	256MB
 *	5	128MB
 *	4	 16GB
 *	3	  8GB
 *	2	  4GB
 *	1	  2GB
 *	0 low	  1GB
 *
 * Reorder Table to be linear by stripping the bottom
 * 2 or 5 bits off and shifting them up to the top.
 *
 */
static uint64_t compute_ranksize(uint32_t mem_type, unsigned char row_dens)
{
	uint64_t bsize;

	bsize = ((row_dens >> 5) | ((row_dens & 31) << 3));
	bsize <<= 27ULL;

	return bsize;
}

/*
 * Convert a two-nibble BCD value into a cycle time.
 * While the spec calls for nano-seconds, picos are returned.
 */
static uint32_t convert_bcd_tenths_to_cycle_time_ps(uint32_t spd_val)
{
	uint32_t tenths_ps[16] = {
		0,
		100,
		200,
		300,
		400,
		500,
		600,
		700,
		800,
		900,
		250,
		330,
		660,
		750,
		0,
		0
	};
	uint32_t whole_ns = (spd_val & 0xF0) >> 4;
	uint32_t tenth_ns = spd_val & 0x0F;
	uint32_t ps = (whole_ns * 1000) + tenths_ps[tenth_ns];

	return ps;
}

static uint32_t convert_bcd_hundredths_to_cycle_time_ps(uint32_t spd_val)
{
	uint32_t tenth_ns = (spd_val & 0xF0) >> 4;
	uint32_t hundredth_ns = spd_val & 0x0F;
	uint32_t ps = (tenth_ns * 100) + (hundredth_ns * 10);

	return ps;
}

static uint32_t byte40_table_ps[8] = {
	0,
	250,
	330,
	500,
	660,
	750,
	0,
	0
};

static uint32_t
compute_trfc_ps_from_spd(unsigned char trctrfc_ext, unsigned char trfc)
{
	uint32_t trfc_ps;

	trfc_ps = (((trctrfc_ext & 0x1) * 256) + trfc) * 1000;
	trfc_ps += byte40_table_ps[(trctrfc_ext >> 1) & 0x7];

	return trfc_ps;
}

static uint32_t
compute_trc_ps_from_spd(unsigned char trctrfc_ext, unsigned char trc)
{
	uint32_t trc_ps;

	trc_ps = (trc * 1000);
	trc_ps += byte40_table_ps[(trctrfc_ext >> 4) & 0x7];

	return trc_ps;
}

/*
 * Determine Refresh Rate.
 * Table from SPD Spec, Byte 12, converted to picoseconds and
 * filled in with "default" normal values.
 */
static uint32_t determine_refresh_rate_ps(const uint32_t spd_refresh)
{
	uint32_t refresh_time_ps[8] = {
		15625000,	/* 0 Normal    1.00x */
		3900000,	/* 1 Reduced    .25x */
		7800000,	/* 2 Extended   .50x */
		31300000,	/* 3 Extended  2.00x */
		62500000,	/* 4 Extended  4.00x */
		125000000,	/* 5 Extended  8.00x */
		15625000,	/* 6 Normal    1.00x  filler */
		15625000,	/* 7 Normal    1.00x  filler */
	};

	return refresh_time_ps[spd_refresh & 0x7];
}

/*
 * The purpose of this function is to compute a suitable
 * CAS latency given the DRAM clock period. The SPD only
 * defines at most 3 CAS latencies. Typically the slower in
 * frequency the DIMM runs at, the shorter its CAS latency can.
 * be. If the DIMM is operating at a sufficiently low frequency,
 * it may be able to run at a CAS latency shorter than the
 * shortest SPD-defined CAS latency.
 *
 * If a CAS latency is not found, 0 is returned.
 *
 * Do this by finding in the standard speed table the longest
 * tCKmin that doesn't exceed the value of mclk_ps (tCK).
 *
 * An assumption made is that the SDRAM device allows the
 * CL to be programmed for a value that is lower than those
 * advertised by the SPD. This is not always the case,
 * as those modes not defined in the SPD are optional.
 *
 * CAS latency de-rating based upon values JEDEC Standard No. 79-2C
 * Table 40, "DDR2 SDRAM standard speed bins and tCK, tRCD, tRP, tRAS,
 * and tRC for corresponding bin"
 *
 * ordinal 2, ddr2_speed_bins[1] contains tCK for CL=3
 * Not certain if any good value exists for CL=2
 */
			  /* CL2  CL3   CL4   CL5   CL6   CL7 */
uint16_t ddr2_speed_bins[] = { 0, 5000, 3750, 3000, 2500, 1875 };

uint32_t compute_derated_DDR2_CAS_latency(uint32_t mclk_ps)
{
	const uint32_t num_speed_bins = ARRAY_SIZE(ddr2_speed_bins);
	uint32_t lowest_tCKmin_found = 0, lowest_tCKmin_CL = 0, i, x;

	for (i = 0; i < num_speed_bins; i++) {
		x = ddr2_speed_bins[i];
		if (x && (x <= mclk_ps) && (x >= lowest_tCKmin_found)) {
			lowest_tCKmin_found = x;
			lowest_tCKmin_CL = i + 2;
		}
	}

	return lowest_tCKmin_CL;
}

/*
 * compute_dimm_parameters for DDR2 SPD
 *
 * Compute DIMM parameters based upon the SPD information in SPD.
 * Writes the results to the dimm_params_s structure pointed by pdimm.
 */
uint32_t
compute_dimm_parameters(const generic_spd_eeprom_t *spdin,
		struct dimm_params_s *pdimm)
{
	const struct ddr2_spd_eeprom_s *spd = spdin;
	uint32_t retval;

	if (!spd->mem_type) {
		memset(pdimm, 0, sizeof(struct dimm_params_s));
		goto error;
	}

	if (spd->mem_type != SPD_MEMTYPE_DDR2)
		goto error;

	retval = ddr2_spd_checksum_pass(spd);
	if (retval)
		goto spd_err;

	/*
	 * The part name in ASCII in the SPD EEPROM is not null terminated.
	 * Guarantee null termination here by presetting all bytes to 0
	 * and copying the part name in ASCII from the SPD onto it
	 */
	memset(pdimm->mpart, 0, sizeof(pdimm->mpart));
	memcpy(pdimm->mpart, spd->mpart, sizeof(pdimm->mpart) - 1);

	/* DIMM organization parameters */
	pdimm->n_ranks = (spd->mod_ranks & 0x7) + 1;
	pdimm->rank_density = compute_ranksize(spd->mem_type, spd->rank_dens);
	pdimm->capacity = pdimm->n_ranks * pdimm->rank_density;
	pdimm->data_width = spd->dataw;
	pdimm->primary_sdram_width = spd->primw;
	pdimm->ec_sdram_width = spd->ecw;

	/* These are all the types defined by the JEDEC DDR2 SPD 1.3 spec */
	switch (spd->dimm_type) {
	case DDR2_SPD_DIMMTYPE_RDIMM:
	case DDR2_SPD_DIMMTYPE_72B_SO_RDIMM:
	case DDR2_SPD_DIMMTYPE_MINI_RDIMM:
		/* Registered/buffered DIMMs */
		pdimm->registered_dimm = 1;
		break;

	case DDR2_SPD_DIMMTYPE_UDIMM:
	case DDR2_SPD_DIMMTYPE_SO_DIMM:
	case DDR2_SPD_DIMMTYPE_MICRO_DIMM:
	case DDR2_SPD_DIMMTYPE_MINI_UDIMM:
		/* Unbuffered DIMMs */
		pdimm->registered_dimm = 0;
		break;

	case DDR2_SPD_DIMMTYPE_72B_SO_CDIMM:
	default:
		goto error;
	}

	pdimm->n_row_addr = spd->nrow_addr;
	pdimm->n_col_addr = spd->ncol_addr;
	pdimm->n_banks_per_sdram_device = spd->nbanks;
	pdimm->edc_config = spd->config;
	pdimm->burst_lengths_bitmask = spd->burstl;
	pdimm->row_density = spd->rank_dens;

	/*
	 * Calculate the Maximum Data Rate based on the Minimum Cycle time.
	 * The SPD clk_cycle field (tCKmin) is measured in tenths of
	 * nanoseconds and represented as BCD.
	 */
	pdimm->tCKmin_X_ps
	    = convert_bcd_tenths_to_cycle_time_ps(spd->clk_cycle);
	pdimm->tCKmin_X_minus_1_ps
	    = convert_bcd_tenths_to_cycle_time_ps(spd->clk_cycle2);
	pdimm->tCKmin_X_minus_2_ps
	    = convert_bcd_tenths_to_cycle_time_ps(spd->clk_cycle3);
	pdimm->tCKmax_ps = convert_bcd_tenths_to_cycle_time_ps(spd->tckmax);

	/*
	 * Compute CAS latencies defined by SPD
	 * The SPD caslat_X should have at least 1 and at most 3 bits set.
	 *
	 * If cas_lat after masking is 0, the __ilog2 function returns
	 * 255 into the variable. This behavior is abused once.
	 */
	pdimm->caslat_X = __ilog2(spd->cas_lat);
	pdimm->caslat_X_minus_1 = __ilog2(spd->cas_lat
					  & ~(1 << pdimm->caslat_X));
	pdimm->caslat_X_minus_2 = __ilog2(spd->cas_lat & ~(1 << pdimm->caslat_X)
					  & ~(1 << pdimm->caslat_X_minus_1));
	pdimm->caslat_lowest_derated
	    = compute_derated_DDR2_CAS_latency(get_memory_clk_period_ps());
	pdimm->tRCD_ps = spd->trcd * 250;
	pdimm->tRP_ps = spd->trp * 250;
	pdimm->tRAS_ps = spd->tras * 1000;
	pdimm->tWR_ps = spd->twr * 250;
	pdimm->tWTR_ps = spd->twtr * 250;
	pdimm->tRFC_ps = compute_trfc_ps_from_spd(spd->trctrfc_ext, spd->trfc);
	pdimm->tRRD_ps = spd->trrd * 250;
	pdimm->tRC_ps = compute_trc_ps_from_spd(spd->trctrfc_ext, spd->trc);
	pdimm->refresh_rate_ps = determine_refresh_rate_ps(spd->refresh);
	pdimm->tIS_ps = convert_bcd_hundredths_to_cycle_time_ps(spd->ca_setup);
	pdimm->tIH_ps = convert_bcd_hundredths_to_cycle_time_ps(spd->ca_hold);
	pdimm->tDS_ps
	    = convert_bcd_hundredths_to_cycle_time_ps(spd->data_setup);
	pdimm->tDH_ps = convert_bcd_hundredths_to_cycle_time_ps(spd->data_hold);
	pdimm->tRTP_ps = spd->trtp * 250;
	pdimm->tDQSQ_max_ps = spd->tdqsq * 10;
	pdimm->tQHS_ps = spd->tqhs * 10;

	return 0;
error:
	return 1;
spd_err:
	return 2;
}
