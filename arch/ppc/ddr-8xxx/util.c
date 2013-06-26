/*
 * Copyright 2008-2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <asm/fsl_law.h>
#include <asm-generic/div64.h>
#include <mach/clock.h>
#include "ddr.h"

#define ULL_2E12 2000000000000ULL
#define UL_5POW12 244140625UL
#define UL_2POW13 (1UL << 13)
#define ULL_8FS 0xFFFFFFFFULL

/*
 * Round up mclk_ps to nearest 1 ps in memory controller code
 * if the error is 0.5ps or more.
 *
 * If an imprecise data rate is too high due to rounding error
 * propagation, compute a suitably rounded mclk_ps to compute
 * a working memory controller configuration.
 */
uint32_t get_memory_clk_period_ps(void)
{
	uint32_t result, data_rate = fsl_get_ddr_freq(0);
	/* Round to nearest 10ps, being careful about 64-bit multiply/divide */
	uint64_t rem, mclk_ps = ULL_2E12;

	/* Now perform the big divide, the result fits in 32-bits */
	rem = do_div(mclk_ps, data_rate);
	if (rem >= (data_rate >> 1))
		result = mclk_ps + 1;
	else
		result = mclk_ps;

	return result;
}

/* Convert picoseconds into DRAM clock cycles (rounding up if needed). */
uint32_t picos_to_mclk(uint32_t picos)
{
	uint64_t clks, clks_rem;
	uint32_t data_rate = fsl_get_ddr_freq(0);

	if (!picos)
		return 0;

	/* First multiply the time by the data rate (32x32 => 64) */
	clks = picos * (uint64_t)data_rate;
	/*
	 * Now divide by 5^12 and track the 32-bit remainder, then divide
	 * by 2*(2^12) using shifts (and updating the remainder).
	 */
	clks_rem = do_div(clks, UL_5POW12);
	clks_rem += (clks & (UL_2POW13 - 1)) * UL_5POW12;
	clks >>= 13;

	/* If we had a remainder greater than the 1ps error, then round up */
	if (clks_rem > data_rate)
		clks++;

	if (clks > ULL_8FS)
		clks = ULL_8FS;

	return (uint32_t)clks;
}

uint32_t mclk_to_picos(unsigned int mclk)
{
	return get_memory_clk_period_ps() * mclk;
}

int fsl_ddr_set_lawbar(
		const struct common_timing_params_s *params,
		uint32_t law_memctl)
{
	uint64_t base = params->base_address;
	uint64_t size = params->total_mem;

	if (!params->ndimms_present)
		goto out;

	if (base >= MAX_MEM_MAPPED)
		goto error;

	if ((base + size) >= MAX_MEM_MAPPED)
		size = MAX_MEM_MAPPED - base;

	if (fsl_set_ddr_laws(base, size, law_memctl) < 0)
		goto error;
out:
	return 0;
error:
	return 1;
}
