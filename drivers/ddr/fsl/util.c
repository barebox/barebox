// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2008-2014 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <soc/fsl/fsl_immap.h>
#include <io.h>
#include <soc/fsl/immap_lsch2.h>
#include <linux/math64.h>
#include "fsl_ddr.h"

/* To avoid 64-bit full-divides, we factor this here */
#define ULL_2E12 2000000000000ULL
#define UL_5POW12 244140625UL
#define UL_2POW13 (1UL << 13)

#define ULL_8FS 0xFFFFFFFFULL

u32 fsl_ddr_get_version(struct fsl_ddr_controller *c)
{
	struct ccsr_ddr __iomem *ddr = c->base;
	u32 ver_major_minor_errata;

	ver_major_minor_errata = (ddr_in32(&ddr->ip_rev1) & 0xFFFF) << 8;
	ver_major_minor_errata |= (ddr_in32(&ddr->ip_rev2) & 0xFF00) >> 8;

	return ver_major_minor_errata;
}

/*
 * Round up mclk_ps to nearest 1 ps in memory controller code
 * if the error is 0.5ps or more.
 *
 * If an imprecise data rate is too high due to rounding error
 * propagation, compute a suitably rounded mclk_ps to compute
 * a working memory controller configuration.
 */
unsigned int get_memory_clk_period_ps(struct fsl_ddr_controller *c)
{
	unsigned int data_rate = c->ddr_freq;
	unsigned int result;

	/* Round to nearest 10ps, being careful about 64-bit multiply/divide */
	unsigned long long rem, mclk_ps = ULL_2E12;

	/* Now perform the big divide, the result fits in 32-bits */
	rem = do_div(mclk_ps, data_rate);
	result = (rem >= (data_rate >> 1)) ? mclk_ps + 1 : mclk_ps;

	return result;
}

/* Convert picoseconds into DRAM clock cycles (rounding up if needed). */
unsigned int picos_to_mclk(struct fsl_ddr_controller *c, unsigned int picos)
{
	unsigned long long clks, clks_rem;
	unsigned int data_rate = c->ddr_freq;

	/* Short circuit for zero picos */
	if (!picos)
		return 0;

	/* First multiply the time by the data rate (32x32 => 64) */
	clks = picos * (unsigned long long)data_rate;
	/*
	 * Now divide by 5^12 and track the 32-bit remainder, then divide
	 * by 2*(2^12) using shifts (and updating the remainder).
	 */
	clks_rem = do_div(clks, UL_5POW12);
	clks_rem += (clks & (UL_2POW13-1)) * UL_5POW12;
	clks >>= 13;

	/* If we had a remainder greater than the 1ps error, then round up */
	if (clks_rem > data_rate)
		clks++;

	/* Clamp to the maximum representable value */
	if (clks > ULL_8FS)
		clks = ULL_8FS;
	return (unsigned int) clks;
}

unsigned int mclk_to_picos(struct fsl_ddr_controller *c, unsigned int mclk)
{
	return get_memory_clk_period_ps(c) * mclk;
}

void fsl_ddr_set_intl3r(const unsigned int granule_size)
{
}

u32 fsl_ddr_get_intl3r(void)
{
	u32 val = 0;
	return val;
}
