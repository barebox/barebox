/*
 * Copyright 2008-2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <config.h>
#include <asm/fsl_ddr_sdram.h>

#include "ddr.h"

static unsigned int common_burst_length(
		const struct dimm_params_s *dimm_params,
		const unsigned int number_of_dimms)
{
	unsigned int i, temp;

	temp = 0xff;
	for (i = 0; i < number_of_dimms; i++)
		if (dimm_params[i].n_ranks)
			temp &= dimm_params[i].burst_lengths_bitmask;

	return temp;
}

/* Compute a CAS latency suitable for all DIMMs */
static unsigned int compute_lowest_caslat(
		const struct dimm_params_s *dimm_params,
		const unsigned int number_of_dimms)
{
	uint32_t temp1, temp2, i, not_ok, lowest_good_caslat,
		 tCKmin_X_minus_1_ps, tCKmin_X_minus_2_ps;
	const unsigned int mclk_ps = get_memory_clk_period_ps();

	/*
	 * Step 1: find CAS latency common to all DIMMs using bitwise
	 * operation.
	 */
	temp1 = 0xFF;
	for (i = 0; i < number_of_dimms; i++)
		if (dimm_params[i].n_ranks) {
			temp2 = 0;
			temp2 |= 1 << dimm_params[i].caslat_X;
			temp2 |= 1 << dimm_params[i].caslat_X_minus_1;
			temp2 |= 1 << dimm_params[i].caslat_X_minus_2;
			/*
			 * FIXME: If there was no entry for X-2 (X-1) in
			 * the SPD, then caslat_X_minus_2
			 * (caslat_X_minus_1) contains either 255 or
			 * 0xFFFFFFFF because that's what the __ilog2
			 * function returns for an input of 0.
			 * On 32-bit PowerPC, left shift counts with bit
			 * 26 set (that the value of 255 or 0xFFFFFFFF
			 * will have), cause the destination register to
			 * be 0. That is why this works.
			 */
			temp1 &= temp2;
		}

	/*
	 * Step 2: check each common CAS latency against tCK of each
	 * DIMM's SPD.
	 */
	lowest_good_caslat = 0;
	temp2 = 0;
	while (temp1) {
		not_ok = 0;
		temp2 = __ilog2(temp1);

		for (i = 0; i < number_of_dimms; i++) {
			if (!dimm_params[i].n_ranks)
				continue;

			if (dimm_params[i].caslat_X == temp2) {
				if (mclk_ps >= dimm_params[i].tCKmin_X_ps)
					continue;
				else
					not_ok++;
			}

			if (dimm_params[i].caslat_X_minus_1 == temp2) {
				tCKmin_X_minus_1_ps =
					dimm_params[i].tCKmin_X_minus_1_ps;
				if (mclk_ps >= tCKmin_X_minus_1_ps)
					continue;
				else
					not_ok++;
			}

			if (dimm_params[i].caslat_X_minus_2 == temp2) {
				tCKmin_X_minus_2_ps
					= dimm_params[i].tCKmin_X_minus_2_ps;
				if (mclk_ps >= tCKmin_X_minus_2_ps)
					continue;
				else
					not_ok++;
			}
		}

		if (!not_ok)
			lowest_good_caslat = temp2;

		temp1 &= ~(1 << temp2);
	}
	return lowest_good_caslat;
}

/*
 * compute_lowest_common_dimm_parameters()
 *
 * Determine the worst-case DIMM timing parameters from the set of DIMMs
 * whose parameters have been computed into the array pointed to
 * by dimm_params.
 */
unsigned int
compute_lowest_common_dimm_parameters(const struct dimm_params_s *dimm,
				      struct common_timing_params_s *out,
				      const unsigned int number_of_dimms)
{
	const uint32_t mclk_ps = get_memory_clk_period_ps();
	uint32_t temp1, i;
	struct common_timing_params_s tmp = {0};

	tmp.tCKmax_ps = 0xFFFFFFFF;
	temp1 = 0;
	for (i = 0; i < number_of_dimms; i++) {
		if (dimm[i].n_ranks == 0) {
			temp1++;
			continue;
		}

		/*
		 * Find minimum tCKmax_ps to find fastest slow speed,
		 * i.e., this is the slowest the whole system can go.
		 */
		tmp.tCKmax_ps = min(tmp.tCKmax_ps, dimm[i].tCKmax_ps);

		/* Find maximum value to determine slowest speed, delay, etc */
		tmp.tCKmin_X_ps = max(tmp.tCKmin_X_ps, dimm[i].tCKmin_X_ps);
		tmp.tCKmax_max_ps = max(tmp.tCKmax_max_ps, dimm[i].tCKmax_ps);
		tmp.tRCD_ps = max(tmp.tRCD_ps, dimm[i].tRCD_ps);
		tmp.tRP_ps = max(tmp.tRP_ps, dimm[i].tRP_ps);
		tmp.tRAS_ps = max(tmp.tRAS_ps, dimm[i].tRAS_ps);
		tmp.tWR_ps = max(tmp.tWR_ps, dimm[i].tWR_ps);
		tmp.tWTR_ps = max(tmp.tWTR_ps, dimm[i].tWTR_ps);
		tmp.tRFC_ps = max(tmp.tRFC_ps, dimm[i].tRFC_ps);
		tmp.tRRD_ps = max(tmp.tRRD_ps, dimm[i].tRRD_ps);
		tmp.tRC_ps = max(tmp.tRC_ps, dimm[i].tRC_ps);
		tmp.tIS_ps = max(tmp.tIS_ps, dimm[i].tIS_ps);
		tmp.tIH_ps = max(tmp.tIH_ps, dimm[i].tIH_ps);
		tmp.tDS_ps = max(tmp.tDS_ps, dimm[i].tDS_ps);
		tmp.tDH_ps = max(tmp.tDH_ps, dimm[i].tDH_ps);
		tmp.tRTP_ps = max(tmp.tRTP_ps, dimm[i].tRTP_ps);
		tmp.tQHS_ps = max(tmp.tQHS_ps, dimm[i].tQHS_ps);
		tmp.refresh_rate_ps = max(tmp.refresh_rate_ps,
				dimm[i].refresh_rate_ps);
		/* Find maximum tDQSQ_max_ps to find slowest timing. */
		tmp.tDQSQ_max_ps = max(tmp.tDQSQ_max_ps, dimm[i].tDQSQ_max_ps);
	}
	tmp.ndimms_present = number_of_dimms - temp1;

	if (temp1 == number_of_dimms)
		return 0;

	temp1 = common_burst_length(dimm, number_of_dimms);
	tmp.all_DIMMs_burst_lengths_bitmask = temp1;
	tmp.all_DIMMs_registered = 0;

	tmp.lowest_common_SPD_caslat = compute_lowest_caslat(dimm,
			number_of_dimms);
	/*
	 * Compute a common 'de-rated' CAS latency.
	 *
	 * The strategy here is to find the *highest* de-rated cas latency
	 * with the assumption that all of the DIMMs will support a de-rated
	 * CAS latency higher than or equal to their lowest de-rated value.
	 */
	temp1 = 0;
	for (i = 0; i < number_of_dimms; i++)
		temp1 = max(temp1, dimm[i].caslat_lowest_derated);
	tmp.highest_common_derated_caslat = temp1;

	temp1 = 1;
	for (i = 0; i < number_of_dimms; i++)
		if (dimm[i].n_ranks &&
		    !(dimm[i].edc_config & EDC_ECC)) {
			temp1 = 0;
			break;
		}
	tmp.all_DIMMs_ECC_capable = temp1;

	if (mclk_ps > tmp.tCKmax_max_ps)
		return 1;

	/*
	 * AL must be less or equal to tRCD. Typically, AL would
	 * be AL = tRCD - 1;
	 *
	 * When ODT read or write is enabled the sum of CAS latency +
	 * additive latency must be at least 3 cycles.
	 *
	 */
	if ((tmp.lowest_common_SPD_caslat < 4) && (picos_to_mclk(tmp.tRCD_ps) >
				tmp.lowest_common_SPD_caslat))
		tmp.additive_latency = picos_to_mclk(tmp.tRCD_ps) -
			tmp.lowest_common_SPD_caslat;

	memcpy(out, &tmp, sizeof(struct common_timing_params_s));

	return 0;
}
