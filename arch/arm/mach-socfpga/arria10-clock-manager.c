/*
 * Copyright (C) 2014 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <mach/generic.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-clock-manager.h>

static const struct arria10_clock_manager *arria10_clkmgr_base =
	(void *)ARRIA10_CLKMGR_ADDR;

static uint32_t eosc1_hz;
static uint32_t cb_intosc_hz;
static uint32_t f2s_free_hz;
#define LOCKED_MASK	(ARRIA10_CLKMGR_CLKMGR_STAT_MAINPLLLOCKED_SET_MSK | \
			 ARRIA10_CLKMGR_CLKMGR_STAT_PERPLLLOCKED_SET_MSK)

static inline void arria10_cm_wait_for_lock(uint32_t mask)
{
	register uint32_t inter_val;

	do {
		inter_val = readl(&arria10_clkmgr_base->stat) & mask;
	} while (inter_val != mask);
}

/* function to poll in the fsm busy bit */
static inline void arria10_cm_wait4fsm(void)
{
	register uint32_t inter_val;

	do {
		inter_val = readl(&arria10_clkmgr_base->stat) &
			ARRIA10_CLKMGR_CLKMGR_STAT_BUSY_SET_MSK;
	} while (inter_val);
}

static uint32_t arria10_cm_get_main_vco(void)
{
	uint32_t vco1, src_hz, numer, denom, vco;
	uint32_t clk_src = readl(&arria10_clkmgr_base->main_pll_vco0);

	clk_src = (clk_src >> ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_LSB) &
		ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_MSK;

	switch (clk_src) {
	case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_EOSC:
		src_hz = eosc1_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_E_INTOSC:
		src_hz = cb_intosc_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_F2S:
		src_hz = f2s_free_hz;
		break;
	default:
		pr_err("arria10_cm_get_main_vco invalid clk_src %d\n", clk_src);
		return 0;
	}

	vco1 = readl(&arria10_clkmgr_base->main_pll_vco1);
	numer = vco1 & ARRIA10_CLKMGR_MAINPLL_VCO1_NUMER_MSK;
	denom = (vco1 >> ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_LSB) &
		 ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_MSK;
	vco = src_hz * (1 + numer);
	vco /= 1 + denom;

	return vco;
}

static uint32_t arria10_cm_get_peri_vco(void)
{
	uint32_t vco1, src_hz, numer, denom, vco;
	uint32_t clk_src = readl(&arria10_clkmgr_base->per_pll_vco0);

	clk_src = (clk_src >> ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_LSB) &
		ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_MSK;

	switch (clk_src) {
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_EOSC:
		src_hz = eosc1_hz;
		break;
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_E_INTOSC:
		src_hz = cb_intosc_hz;
		break;
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_F2S:
		src_hz = f2s_free_hz;
		break;
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_MAIN:
		src_hz = arria10_cm_get_main_vco();
		src_hz /= (readl(&arria10_clkmgr_base->main_pll_cntr15clk) &
			   ARRIA10_CLKMGR_MAINPLL_CNTRCLK_MSK) + 1;
		break;
	default:
		pr_err("arria10_cm_get_peri_vco invalid clk_src %d\n", clk_src);
		return 0;
	}

	vco1 = readl(&arria10_clkmgr_base->per_pll_vco1);
	numer = vco1 & ARRIA10_CLKMGR_PERPLL_VCO1_NUMER_MSK;
	denom = (vco1 >> ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_LSB) &
		ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_MSK;
	vco = src_hz * (1 + numer);
	vco /= 1 + denom;

	return vco;
}

unsigned int arria10_cm_get_mmc_controller_clk_hz(void)
{
	uint32_t clk_hz = 0;
	uint32_t clk_input = readl(&arria10_clkmgr_base->per_pll_cntr6clk);
	clk_input = (clk_input >> ARRIA10_CLKMGR_PERPLL_CNTR6CLK_SRC_LSB) &
		ARRIA10_CLKMGR_PERPLLGRP_SRC_MSK;

	switch (clk_input) {
	case ARRIA10_CLKMGR_PERPLLGRP_SRC_MAIN:
		clk_hz = arria10_cm_get_main_vco();
		clk_hz /= 1 + (readl(&arria10_clkmgr_base->main_pll_cntr6clk) &
			       ARRIA10_CLKMGR_MAINPLL_CNTRCLK_MSK);
		break;

	case ARRIA10_CLKMGR_PERPLLGRP_SRC_PERI:
		clk_hz = arria10_cm_get_peri_vco();
		clk_hz /= 1 + (readl(&arria10_clkmgr_base->per_pll_cntr6clk) &
			       ARRIA10_CLKMGR_PERPLL_CNTRCLK_MSK);
		break;

	case ARRIA10_CLKMGR_PERPLLGRP_SRC_OSC1:
		clk_hz = eosc1_hz;
		break;

	case ARRIA10_CLKMGR_PERPLLGRP_SRC_INTOSC:
		clk_hz = cb_intosc_hz;
		break;

	case ARRIA10_CLKMGR_PERPLLGRP_SRC_FPGA:
		clk_hz = f2s_free_hz;
		break;
	}

	return clk_hz/4;
}

/* calculate the intended main VCO frequency based on handoff */
static uint32_t arria10_cm_calc_handoff_main_vco_clk_hz(struct arria10_mainpll_cfg *main_cfg)
{
	uint32_t clk_hz;

	/* Check main VCO clock source: eosc, intosc or f2s? */
	switch (main_cfg->vco0_psrc) {
	case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_EOSC:
		clk_hz = eosc1_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_E_INTOSC:
		clk_hz = cb_intosc_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_F2S:
		clk_hz = f2s_free_hz;
		break;
	default:
		return 0;
	}

	/* calculate the VCO frequency */
	clk_hz *= 1 + main_cfg->vco1_numer;
	clk_hz /= 1 + main_cfg->vco1_denom;

	return clk_hz;
}

/* calculate the intended periph VCO frequency based on handoff */
static uint32_t arria10_cm_calc_handoff_periph_vco_clk_hz(struct arria10_mainpll_cfg *main_cfg,
							  struct arria10_perpll_cfg *per_cfg)
{
	uint32_t clk_hz;

	/* Check periph VCO clock source: eosc, intosc, f2s or mainpll? */
	switch (per_cfg->vco0_psrc) {
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_EOSC:
		clk_hz = eosc1_hz;
		break;
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_E_INTOSC:
		clk_hz = cb_intosc_hz;
		break;
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_F2S:
		clk_hz = f2s_free_hz;
		break;
	case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_MAIN:
		clk_hz = arria10_cm_calc_handoff_main_vco_clk_hz(main_cfg);
		clk_hz /= main_cfg->cntr15clk_cnt;
		break;
	default:
		return 0;
	}

	/* calculate the VCO frequency */
	clk_hz *= 1 + per_cfg->vco1_numer;
	clk_hz /= 1 + per_cfg->vco1_denom;

	return clk_hz;
}

/* calculate the intended MPU clock frequency based on handoff */
static uint32_t arria10_cm_calc_handoff_mpu_clk_hz(struct arria10_mainpll_cfg *main_cfg,
						   struct arria10_perpll_cfg *per_cfg)
{
	uint32_t clk_hz;

	/* Check MPU clock source: main, periph, osc1, intosc or f2s? */
	switch (main_cfg->mpuclk_src) {
	case ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN:
		clk_hz = arria10_cm_calc_handoff_main_vco_clk_hz(main_cfg);
		clk_hz /= ((main_cfg->mpuclk & ARRIA10_CLKMGR_MAINPLL_MPUCLK_CNT_MSK)
			   + 1);
		break;
	case ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI:
		clk_hz = arria10_cm_calc_handoff_periph_vco_clk_hz(main_cfg, per_cfg);
		clk_hz /= (((main_cfg->mpuclk >>
			   ARRIA10_CLKMGR_MAINPLL_MPUCLK_PERICNT_LSB) &
			   ARRIA10_CLKMGR_MAINPLL_MPUCLK_CNT_MSK) + 1);
		break;
	case ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_OSC1:
		clk_hz = eosc1_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_INTOSC:
		clk_hz = cb_intosc_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_FPGA:
		clk_hz = f2s_free_hz;
		break;
	default:
		return 0;
	}

	clk_hz /= (main_cfg->mpuclk_cnt + 1);

	return clk_hz;
}

/* calculate the intended NOC clock frequency based on handoff */
static uint32_t arria10_cm_calc_handoff_noc_clk_hz(struct arria10_mainpll_cfg *main_cfg,
						   struct arria10_perpll_cfg *per_cfg)
{
	uint32_t clk_hz;

	/* Check MPU clock source: main, periph, osc1, intosc or f2s? */
	switch (main_cfg->nocclk_src) {
	case ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN:
		clk_hz = arria10_cm_calc_handoff_main_vco_clk_hz(main_cfg);
		clk_hz /= ((main_cfg->nocclk & ARRIA10_CLKMGR_MAINPLL_NOCCLK_CNT_MSK)
			   + 1);
		break;
	case ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI:
		clk_hz = arria10_cm_calc_handoff_periph_vco_clk_hz(main_cfg, per_cfg);
		clk_hz /= (((main_cfg->nocclk >>
			   ARRIA10_CLKMGR_MAINPLL_NOCCLK_PERICNT_LSB) &
			   ARRIA10_CLKMGR_MAINPLL_NOCCLK_CNT_MSK) + 1);
		break;
	case ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_OSC1:
		clk_hz = eosc1_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_INTOSC:
		clk_hz = cb_intosc_hz;
		break;
	case ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_FPGA:
		clk_hz = f2s_free_hz;
		break;
	default:
		return 0;
	}

	clk_hz /= (main_cfg->nocclk_cnt + 1);

	return clk_hz;
}

/* return 1 if PLL ramp is required */
static int arria10_cm_is_pll_ramp_required(int main0periph1,
					   struct arria10_mainpll_cfg *main_cfg,
					   struct arria10_perpll_cfg *per_cfg)
{

	/* Check for main PLL */
	if (main0periph1 == 0) {
		/*
		 * PLL ramp is not required if both MPU clock and NOC clock are
		 * not sourced from main PLL
		 */
		if (main_cfg->mpuclk_src != ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN &&
		    main_cfg->nocclk_src != ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN)
			return 0;

		/*
		 * PLL ramp is required if MPU clock is sourced from main PLL
		 * and MPU clock is over 900MHz (as advised by HW team)
		 */
		if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN &&
		    (arria10_cm_calc_handoff_mpu_clk_hz(main_cfg, per_cfg) >
		     ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_THRESHOLD_HZ))
			return 1;

		/*
		 * PLL ramp is required if NOC clock is sourced from main PLL
		 * and NOC clock is over 300MHz (as advised by HW team)
		 */
		if (main_cfg->nocclk_src == ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN &&
		    (arria10_cm_calc_handoff_noc_clk_hz(main_cfg, per_cfg) >
		     ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_THRESHOLD_HZ))
			return 1;

	} else if (main0periph1 == 1) {
		/*
		 * PLL ramp is not required if both MPU clock and NOC clock are
		 * not sourced from periph PLL
		 */
		if (main_cfg->mpuclk_src != ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI &&
		    main_cfg->nocclk_src != ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI)
			return 0;

		/*
		 * PLL ramp is required if MPU clock are source from periph PLL
		 * and MPU clock is over 900MHz (as advised by HW team)
		 */
		if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI &&
		    (arria10_cm_calc_handoff_mpu_clk_hz(main_cfg, per_cfg) >
		     ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_THRESHOLD_HZ))
			return 1;

		/*
		 * PLL ramp is required if NOC clock are source from periph PLL
		 * and NOC clock is over 300MHz (as advised by HW team)
		 */
		if (main_cfg->nocclk_src == ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI &&
		    (arria10_cm_calc_handoff_noc_clk_hz(main_cfg, per_cfg) >
		     ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_THRESHOLD_HZ))
			return 1;
	}

	return 0;
}

/*
 * Calculate the new PLL numerator which is based on existing DTS hand off and
 * intended safe frequency (safe_hz). Note that PLL ramp is only modifying the
 * numerator while maintaining denominator as denominator will influence the
 * jitter condition. Please refer A10 HPS TRM for the jitter guide. Note final
 * value for numerator is minus with 1 to cater our register value
 * representation.
 */
static uint32_t arria10_cm_calc_safe_pll_numer(int main0periph1,
					       struct arria10_mainpll_cfg *main_cfg,
					       struct arria10_perpll_cfg *per_cfg,
					       uint32_t safe_hz)
{
	uint32_t clk_hz = 0;

	/* Check for main PLL */
	if (main0periph1 == 0) {
		/* Check main VCO clock source: eosc, intosc or f2s? */
		switch (main_cfg->vco0_psrc) {
		case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_EOSC:
			clk_hz = eosc1_hz;
			break;
		case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_E_INTOSC:
			clk_hz = cb_intosc_hz;
			break;
		case ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_F2S:
			clk_hz = f2s_free_hz;
			break;
		default:
			return 0;
		}

		/* Applicable if MPU clock is from main PLL */
		if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN) {
			/* calculate the safe numer value */
			clk_hz = (safe_hz / clk_hz) *
				(main_cfg->mpuclk_cnt + 1) *
				((main_cfg->mpuclk &
				  ARRIA10_CLKMGR_MAINPLL_MPUCLK_CNT_MSK) + 1) *
				(1 + main_cfg->vco1_denom) - 1;
		}
		/* Reach here if MPU clk not from main PLL but NOC clk is */
		else if (main_cfg->nocclk_src ==
			 ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN) {
			/* calculate the safe numer value */
			clk_hz = (safe_hz / clk_hz) *
				(main_cfg->nocclk_cnt + 1) *
				((main_cfg->nocclk &
				  ARRIA10_CLKMGR_MAINPLL_NOCCLK_CNT_MSK) + 1) *
				(1 + main_cfg->vco1_denom) - 1;
		} else {
			clk_hz = 0;
		}

	} else if (main0periph1 == 1) {
		/* Check periph VCO clock source: eosc, intosc, f2s, mainpll */
		switch (per_cfg->vco0_psrc) {
		case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_EOSC:
			clk_hz = eosc1_hz;
			break;
		case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_E_INTOSC:
			clk_hz = cb_intosc_hz;
			break;
		case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_F2S:
			clk_hz = f2s_free_hz;
			break;
		case ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_MAIN:
			clk_hz = arria10_cm_calc_handoff_main_vco_clk_hz(
				 main_cfg);
			clk_hz /= main_cfg->cntr15clk_cnt;
			break;
		default:
			return 0;
		}
		/* Applicable if MPU clock is from periph PLL */
		if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI) {
			/* calculate the safe numer value */
			clk_hz = (safe_hz / clk_hz) *
				(main_cfg->mpuclk_cnt + 1) *
				(((main_cfg->mpuclk >>
				  ARRIA10_CLKMGR_MAINPLL_MPUCLK_PERICNT_LSB) &
				  ARRIA10_CLKMGR_MAINPLL_MPUCLK_CNT_MSK) + 1) *
				(1 + per_cfg->vco1_denom) - 1;
		}
		/* Reach here if MPU clk not from periph PLL but NOC clk is */
		else if (main_cfg->nocclk_src ==
			 ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI) {
			/* calculate the safe numer value */
			clk_hz = (safe_hz / clk_hz) *
				(main_cfg->nocclk_cnt + 1) *
				(((main_cfg->nocclk >>
				  ARRIA10_CLKMGR_MAINPLL_NOCCLK_PERICNT_LSB) &
				  ARRIA10_CLKMGR_MAINPLL_NOCCLK_CNT_MSK) + 1) *
				(1 + per_cfg->vco1_denom) - 1;
		} else {
			clk_hz = 0;
		}
	}

	return clk_hz;
}

/* ramping the main PLL to final value */
static void arria10_cm_pll_ramp_main(struct arria10_mainpll_cfg *main_cfg,
				     struct arria10_perpll_cfg *per_cfg,
				     uint32_t pll_ramp_main_hz)
{
	uint32_t clk_hz = 0;
	uint32_t clk_incr_hz = 0;
	uint32_t clk_final_hz = 0;

	/* find out the increment value */
	if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN) {
		clk_incr_hz = ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_INCREMENT_HZ;
		clk_final_hz = arria10_cm_calc_handoff_mpu_clk_hz(main_cfg, per_cfg);
	} else if (main_cfg->nocclk_src == ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN) {
		clk_incr_hz = ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_INCREMENT_HZ;
		clk_final_hz = arria10_cm_calc_handoff_noc_clk_hz(main_cfg, per_cfg);
	}

	/* execute the ramping here */
	for (clk_hz = pll_ramp_main_hz + clk_incr_hz;
	     clk_hz < clk_final_hz; clk_hz += clk_incr_hz) {
		writel((main_cfg->vco1_denom << ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_LSB) |
		       arria10_cm_calc_safe_pll_numer(0, main_cfg, per_cfg, clk_hz),
		       &arria10_clkmgr_base->main_pll_vco1);
		__udelay(1000);
		arria10_cm_wait_for_lock(LOCKED_MASK);
	}

	writel((main_cfg->vco1_denom << ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_LSB) |
	       main_cfg->vco1_numer, &arria10_clkmgr_base->main_pll_vco1);

	__udelay(1000);
	arria10_cm_wait_for_lock(LOCKED_MASK);
}

/* ramping the periph PLL to final value */
static void arria10_cm_pll_ramp_periph(struct arria10_mainpll_cfg *main_cfg,
				       struct arria10_perpll_cfg *per_cfg,
				       uint32_t pll_ramp_periph_hz)
{
	uint32_t clk_hz = 0;
	uint32_t clk_incr_hz = 0;
	uint32_t clk_final_hz = 0;

	/* find out the increment value */
	if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI) {
		clk_incr_hz = ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_INCREMENT_HZ;
		clk_final_hz = arria10_cm_calc_handoff_mpu_clk_hz(main_cfg, per_cfg);
	} else if (main_cfg->nocclk_src == ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI) {
		clk_incr_hz = ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_INCREMENT_HZ;
		clk_final_hz = arria10_cm_calc_handoff_noc_clk_hz(main_cfg, per_cfg);
	}

	/* execute the ramping here */
	for (clk_hz = pll_ramp_periph_hz + clk_incr_hz;
	     clk_hz < clk_final_hz; clk_hz += clk_incr_hz) {
		writel((per_cfg->vco1_denom << ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_LSB) |
		       arria10_cm_calc_safe_pll_numer(1, main_cfg, per_cfg, clk_hz),
		       &arria10_clkmgr_base->per_pll_vco1);
		__udelay(1000);
		arria10_cm_wait_for_lock(LOCKED_MASK);
	}

	writel((per_cfg->vco1_denom << ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_LSB) |
	       per_cfg->vco1_numer, &arria10_clkmgr_base->per_pll_vco1);
	__udelay(1000);
	arria10_cm_wait_for_lock(LOCKED_MASK);
}

/*
 * Setup clocks while making no assumptions of the
 * previous state of the clocks.
 *
 * - Start by being paranoid and gate all sw managed clocks
 * - Put all plls in bypass
 * - Put all plls VCO registers back to reset value (bgpwr dwn).
 * - Put peripheral and main pll src to reset value to avoid glitch.
 * - Delay 5 us.
 * - Deassert bg pwr dn and set numerator and denominator
 * - Start 7 us timer.
 * - set internal dividers
 * - Wait for 7 us timer.
 * - Enable plls
 * - Set external dividers while plls are locking
 * - Wait for pll lock
 * - Assert/deassert outreset all.
 * - Take all pll's out of bypass
 * - Clear safe mode
 * - set source main and peripheral clocks
 * - Ungate clocks
 */
static int arria10_cm_full_cfg(struct arria10_mainpll_cfg *main_cfg,
			       struct arria10_perpll_cfg *per_cfg)
{
	uint32_t pll_ramp_main_hz = 0;
	uint32_t pll_ramp_periph_hz = 0;

	/* gate off all mainpll clock excpet HW managed clock */
	writel(ARRIA10_CLKMGR_MAINPLL_EN_S2FUSER0CLKEN_SET_MSK |
	       ARRIA10_CLKMGR_MAINPLL_EN_HMCPLLREFCLKEN_SET_MSK,
	       &arria10_clkmgr_base->main_pll_enr);

	/* now we can gate off the rest of the peripheral clocks */
	writel(0, &arria10_clkmgr_base->per_pll_en);

	/* Put all plls in external bypass */
	writel(ARRIA10_CLKMGR_MAINPLL_BYPASS_RESET,
	       &arria10_clkmgr_base->main_pll_bypasss);
	writel(ARRIA10_CLKMGR_PERPLL_BYPASS_RESET,
	       &arria10_clkmgr_base->per_pll_bypasss);

	/*
	 * Put all plls VCO registers back to reset value.
	 * Some code might have messed with them. At same time set the
	 * desired clock source
	 */
	writel(ARRIA10_CLKMGR_MAINPLL_VCO0_RESET |
	       ARRIA10_CLKMGR_MAINPLL_VCO0_REGEXTSEL_SET_MSK |
	       (main_cfg->vco0_psrc << ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_LSB),
	       &arria10_clkmgr_base->main_pll_vco0);

	writel(ARRIA10_CLKMGR_PERPLL_VCO0_RESET |
	       ARRIA10_CLKMGR_PERPLL_VCO0_REGEXTSEL_SET_MSK |
	       (per_cfg->vco0_psrc << ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_LSB),
	       &arria10_clkmgr_base->per_pll_vco0);

	writel(ARRIA10_CLKMGR_MAINPLL_VCO1_RESET,
	       &arria10_clkmgr_base->main_pll_vco1);
	writel(ARRIA10_CLKMGR_PERPLL_VCO1_RESET,
	       &arria10_clkmgr_base->per_pll_vco1);

	/* clear the interrupt register status register */
	writel(ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLLOST_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLLOST_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLRFSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLRFSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLFBSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLFBSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLACHIEVED_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLACHIEVED_SET_MSK,
	       &arria10_clkmgr_base->intr);

	/* Program VCO “Numerator” and “Denominator” for main PLL */
	if (arria10_cm_is_pll_ramp_required(0, main_cfg, per_cfg)) {
		/* set main PLL to safe starting threshold frequency */
		if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN)
			pll_ramp_main_hz = ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_THRESHOLD_HZ;
		else if (main_cfg->nocclk_src == ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN)
			pll_ramp_main_hz = ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_THRESHOLD_HZ;

		writel((main_cfg->vco1_denom << ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_LSB) |
		       arria10_cm_calc_safe_pll_numer(0, main_cfg, per_cfg,
						      pll_ramp_main_hz),
		       &arria10_clkmgr_base->main_pll_vco1);
	} else {
		writel((main_cfg->vco1_denom << ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_LSB) |
		       main_cfg->vco1_numer,
		       &arria10_clkmgr_base->main_pll_vco1);
	}

	/* Program VCO “Numerator” and “Denominator” for periph PLL */
	if (arria10_cm_is_pll_ramp_required(1, main_cfg, per_cfg)) {
		/* set periph PLL to safe starting threshold frequency */
		if (main_cfg->mpuclk_src == ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI)
			pll_ramp_periph_hz = ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_THRESHOLD_HZ;
		else if (main_cfg->nocclk_src == ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI)
			pll_ramp_periph_hz = ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_THRESHOLD_HZ;

		writel((per_cfg->vco1_denom << ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_LSB) |
		       arria10_cm_calc_safe_pll_numer(1, main_cfg, per_cfg,
						      pll_ramp_periph_hz),
		       &arria10_clkmgr_base->per_pll_vco1);
	} else {
		writel((per_cfg->vco1_denom << ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_LSB) |
		       per_cfg->vco1_numer, &arria10_clkmgr_base->per_pll_vco1);
	}

	/* Wait for at least 5 us */
	__udelay(5);

	/* Now deassert BGPWRDN and PWRDN */
	clrbits_le32(&arria10_clkmgr_base->main_pll_vco0,
		     ARRIA10_CLKMGR_MAINPLL_VCO0_BGPWRDN_SET_MSK |
		     ARRIA10_CLKMGR_MAINPLL_VCO0_PWRDN_SET_MSK);
	clrbits_le32(&arria10_clkmgr_base->per_pll_vco0,
		     ARRIA10_CLKMGR_PERPLL_VCO0_BGPWRDN_SET_MSK |
		     ARRIA10_CLKMGR_PERPLL_VCO0_PWRDN_SET_MSK);

	/* Wait for at least 7 us */
	__udelay(7);

	/* enable the VCO and disable the external regulator to PLL */
	writel((readl(&arria10_clkmgr_base->main_pll_vco0) &
		~ARRIA10_CLKMGR_MAINPLL_VCO0_REGEXTSEL_SET_MSK) |
	       ARRIA10_CLKMGR_MAINPLL_VCO0_EN_SET_MSK,
	       &arria10_clkmgr_base->main_pll_vco0);
	writel((readl(&arria10_clkmgr_base->per_pll_vco0) &
		~ARRIA10_CLKMGR_PERPLL_VCO0_REGEXTSEL_SET_MSK) |
	       ARRIA10_CLKMGR_PERPLL_VCO0_EN_SET_MSK,
	       &arria10_clkmgr_base->per_pll_vco0);

	/* setup all the main PLL counter and clock source */
	writel(main_cfg->nocclk,
	       ARRIA10_CLKMGR_ADDR + ARRIA10_CLKMGR_MAINPLL_NOC_CLK_OFFSET);
	writel(main_cfg->mpuclk,
	       ARRIA10_CLKMGR_ADDR + ARRIA10_CLKMGR_ALTERAGRP_MPU_CLK_OFFSET);

	/* main_emaca_clk divider */
	writel(main_cfg->cntr2clk_cnt, &arria10_clkmgr_base->main_pll_cntr2clk);
	/* main_emacb_clk divider */
	writel(main_cfg->cntr3clk_cnt, &arria10_clkmgr_base->main_pll_cntr3clk);
	/* main_emac_ptp_clk divider */
	writel(main_cfg->cntr4clk_cnt, &arria10_clkmgr_base->main_pll_cntr4clk);
	/* main_gpio_db_clk divider */
	writel(main_cfg->cntr5clk_cnt, &arria10_clkmgr_base->main_pll_cntr5clk);
	/* main_sdmmc_clk divider */
	writel(main_cfg->cntr6clk_cnt, &arria10_clkmgr_base->main_pll_cntr6clk);
	/* main_s2f_user0_clk divider */
	writel(main_cfg->cntr7clk_cnt |
	       (main_cfg->cntr7clk_src << ARRIA10_CLKMGR_MAINPLL_CNTR7CLK_SRC_LSB),
	       &arria10_clkmgr_base->main_pll_cntr7clk);
	/* main_s2f_user1_clk divider */
	writel(main_cfg->cntr8clk_cnt, &arria10_clkmgr_base->main_pll_cntr8clk);
	/* main_hmc_pll_clk divider */
	writel(main_cfg->cntr9clk_cnt |
	       (main_cfg->cntr9clk_src << ARRIA10_CLKMGR_MAINPLL_CNTR9CLK_SRC_LSB),
	       &arria10_clkmgr_base->main_pll_cntr9clk);
	/* main_periph_ref_clk divider */
	writel(main_cfg->cntr15clk_cnt,	&arria10_clkmgr_base->main_pll_cntr15clk);

	/* setup all the peripheral PLL counter and clock source */
	/* peri_emaca_clk divider */
	writel(per_cfg->cntr2clk_cnt |
	       (per_cfg->cntr2clk_src << ARRIA10_CLKMGR_PERPLL_CNTR2CLK_SRC_LSB),
	       &arria10_clkmgr_base->per_pll_cntr2clk);
	/* peri_emacb_clk divider */
	writel(per_cfg->cntr3clk_cnt |
	       (per_cfg->cntr3clk_src << ARRIA10_CLKMGR_PERPLL_CNTR3CLK_SRC_LSB),
	       &arria10_clkmgr_base->per_pll_cntr3clk);
	/* peri_emac_ptp_clk divider */
	writel(per_cfg->cntr4clk_cnt |
	       (per_cfg->cntr4clk_src << ARRIA10_CLKMGR_PERPLL_CNTR4CLK_SRC_LSB),
	       &arria10_clkmgr_base->per_pll_cntr4clk);
	/* peri_gpio_db_clk divider */
	writel(per_cfg->cntr5clk_cnt |
	       (per_cfg->cntr5clk_src << ARRIA10_CLKMGR_PERPLL_CNTR5CLK_SRC_LSB),
	       &arria10_clkmgr_base->per_pll_cntr5clk);
	/* peri_sdmmc_clk divider */
	writel(per_cfg->cntr6clk_cnt |
	       (per_cfg->cntr6clk_src << ARRIA10_CLKMGR_PERPLL_CNTR6CLK_SRC_LSB),
	       &arria10_clkmgr_base->per_pll_cntr6clk);
	/* peri_s2f_user0_clk divider */
	writel(per_cfg->cntr7clk_cnt, &arria10_clkmgr_base->per_pll_cntr7clk);
	/* peri_s2f_user1_clk divider */
	writel(per_cfg->cntr8clk_cnt |
	       (per_cfg->cntr8clk_src << ARRIA10_CLKMGR_PERPLL_CNTR8CLK_SRC_LSB),
	       &arria10_clkmgr_base->per_pll_cntr8clk);
	/* peri_hmc_pll_clk divider */
	writel(per_cfg->cntr9clk_cnt, &arria10_clkmgr_base->per_pll_cntr9clk);

	/* setup all the external PLL counter */
	/* mpu wrapper / external divider */
	writel(main_cfg->mpuclk_cnt |
	       (main_cfg->mpuclk_src << ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_LSB),
	       &arria10_clkmgr_base->main_pll_mpuclk);
	/* NOC wrapper / external divider */
	writel(main_cfg->nocclk_cnt |
	       (main_cfg->nocclk_src << ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_LSB),
	       &arria10_clkmgr_base->main_pll_nocclk);
	/* NOC subclock divider such as l4 */
	writel(main_cfg->nocdiv_l4mainclk |
	       (main_cfg->nocdiv_l4mpclk << ARRIA10_CLKMGR_MAINPLL_NOCDIV_L4MPCLK_LSB) |
	       (main_cfg->nocdiv_l4spclk << ARRIA10_CLKMGR_MAINPLL_NOCDIV_L4SPCLK_LSB) |
	       (main_cfg->nocdiv_csatclk << ARRIA10_CLKMGR_MAINPLL_NOCDIV_CSATCLK_LSB) |
	       (main_cfg->nocdiv_cstraceclk << ARRIA10_CLKMGR_MAINPLL_NOCDIV_CSTRACECLK_LSB) |
	       (main_cfg->nocdiv_cspdbgclk << ARRIA10_CLKMGR_MAINPLL_NOCDIV_CSPDBGCLK_LSB),
	       &arria10_clkmgr_base->main_pll_nocdiv);
	/* gpio_db external divider */
	writel(per_cfg->gpiodiv_gpiodbclk, &arria10_clkmgr_base->per_pll_gpiodiv);

	/* setup the EMAC clock mux select */
	writel((per_cfg->emacctl_emac0sel < ARRIA10_CLKMGR_PERPLL_EMACCTL_EMAC0SEL_LSB) |
	       (per_cfg->emacctl_emac1sel << ARRIA10_CLKMGR_PERPLL_EMACCTL_EMAC1SEL_LSB) |
	       (per_cfg->emacctl_emac2sel << ARRIA10_CLKMGR_PERPLL_EMACCTL_EMAC2SEL_LSB),
	       &arria10_clkmgr_base->per_pll_emacctl);

	/* at this stage, check for PLL lock status */
	arria10_cm_wait_for_lock(LOCKED_MASK);

	/*
	 * after locking, but before taking out of bypass,
	 * assert/deassert outresetall
	 */

	/* assert mainpll outresetall */
	setbits_le32(&arria10_clkmgr_base->main_pll_vco0,
		     ARRIA10_CLKMGR_MAINPLL_VCO0_OUTRSTALL_SET_MSK);
	/* assert perpll outresetall */
	setbits_le32(&arria10_clkmgr_base->per_pll_vco0,
		     ARRIA10_CLKMGR_PERPLL_VCO0_OUTRSTALL_SET_MSK);
	/* de-assert mainpll outresetall */
	clrbits_le32(&arria10_clkmgr_base->main_pll_vco0,
		     ARRIA10_CLKMGR_MAINPLL_VCO0_OUTRSTALL_SET_MSK);
	/* de-assert perpll outresetall */
	clrbits_le32(&arria10_clkmgr_base->per_pll_vco0,
		     ARRIA10_CLKMGR_PERPLL_VCO0_OUTRSTALL_SET_MSK);

	/*
	 * Take all PLLs out of bypass when boot mode is cleared.
	 * release mainpll from bypass
	 */
	writel(ARRIA10_CLKMGR_MAINPLL_BYPASS_RESET,
	       &arria10_clkmgr_base->main_pll_bypassr);
	/* wait till Clock Manager is not busy */
	arria10_cm_wait4fsm();

	/* release perpll from bypass */
	writel(ARRIA10_CLKMGR_PERPLL_BYPASS_RESET,
	       &arria10_clkmgr_base->per_pll_bypassr);
	/* wait till Clock Manager is not busy */
	arria10_cm_wait4fsm();

	/* clear boot mode */
	clrbits_le32(&arria10_clkmgr_base->ctrl,
		     ARRIA10_CLKMGR_CLKMGR_CTL_BOOTMOD_SET_MSK);
	/* wait till Clock Manager is not busy */
	arria10_cm_wait4fsm();

	/* At here, we need to ramp to final value if needed */
	if (pll_ramp_main_hz != 0)
		arria10_cm_pll_ramp_main(main_cfg, per_cfg, pll_ramp_main_hz);
	if (pll_ramp_periph_hz != 0)
		arria10_cm_pll_ramp_periph(main_cfg, per_cfg, pll_ramp_periph_hz);

	/* Now ungate non-hw-managed clocks */
	writel(ARRIA10_CLKMGR_MAINPLL_EN_S2FUSER0CLKEN_SET_MSK |
	       ARRIA10_CLKMGR_MAINPLL_EN_HMCPLLREFCLKEN_SET_MSK,
	       &arria10_clkmgr_base->main_pll_ens);
	writel(ARRIA10_CLKMGR_PERPLL_EN_RESET,
	       &arria10_clkmgr_base->per_pll_ens);

	/*
	 * Clear the loss lock and slip bits as they might set during
	 * clock reconfiguration
	 */
	writel(ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLLOST_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLLOST_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLRFSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLRFSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLFBSLIP_SET_MSK |
	       ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLFBSLIP_SET_MSK,
	       &arria10_clkmgr_base->intr);

	return 0;
}

int arria10_cm_basic_init(struct arria10_mainpll_cfg *mainpll,
			  struct arria10_perpll_cfg *perpll)
{
	return arria10_cm_full_cfg(mainpll, perpll);
}

void arria10_cm_use_intosc(void)
{
	setbits_le32(&arria10_clkmgr_base->ctrl,
		     ARRIA10_CLKMGR_CLKMGR_CTL_BOOTCLK_INTOSC_SET_MSK);
}
