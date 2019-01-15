/**
 * @file
 * @brief OMAP DPLL and various clock configuration
 *
 * @ref prcm_init This is the second level clock init for PRCM as defined in
 * clocks.h -- called from SRAM, or Flash (using temp SRAM stack).
 *
 * During reconfiguring the clocks while in SDRAM/Flash, we can have invalid
 * clock configuration to which ARM instruction/data fetch ops can fail.
 * This critical path is handled by relocating the relevant functions in
 * omap3_clock_core.S to OMAP's ISRAM and executing it there.
 *
 * @warning IMPORTANT: These functions run from ISRAM stack, so no bss sections
 * should be used, functions cannot use global variables/switch constructs.
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 *
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <io.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-generic.h>
#include <mach/clocks.h>
#include <mach/omap3-clock.h>
#include <mach/timers.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>

#define S32K_CR			(OMAP3_32KTIMER_BASE + 0x10)

/* Following functions are exported from omap3_clock_core.S */
/* Helper functions */
static u32 get_osc_clk_speed(void);
static void get_sys_clkin_sel(u32 osc_clk, u32 *sys_clkin_sel);
static void per_clocks_enable(void);

/**
 * @brief Determine reference oscillator speed
 *
 *  This is based on known 32kHz clock and gptimer.
 *
 * @return clock speed S38_4M, S26M S24M S19_2M S13M S12M
 */
static u32 get_osc_clk_speed(void)
{
	u32 start, cstart, cend, cdiff, cdiv, val;

	val = readl(OMAP3_PRM_REG(CLKSRC_CTRL));

	if (val & SYSCLK_DIV_2)
		cdiv = 2;
	else if (val & SYSCLK_DIV_1)
		cdiv = 1;
	else
		/*
		 * Should never reach here!
		 * To proceed, assume divider as 1.
		 */
		cdiv = 1;

	/* enable timer2 */
	val = readl(OMAP3_CM_REG(CLKSEL_WKUP)) | (0x1 << 0);
	writel(val, OMAP3_CM_REG(CLKSEL_WKUP));	/* select sys_clk for GPT1 */

	/* Enable I and F Clocks for GPT1 */
	val = readl(OMAP3_CM_REG(ICLKEN_WKUP)) | (0x1 << 0) | (0x1 << 2);
	writel(val, OMAP3_CM_REG(ICLKEN_WKUP));
	val = readl(OMAP3_CM_REG(FCLKEN_WKUP)) | (0x1 << 0);
	writel(val, OMAP3_CM_REG(FCLKEN_WKUP));
	/* start counting at 0 */
	writel(0, OMAP3_GPTIMER1_BASE + TLDR);
	/* enable clock */
	writel(GPT_EN, OMAP3_GPTIMER1_BASE + TCLR);
	/* enable 32kHz source - enabled out of reset */
	/* determine sys_clk via gauging */

	start = 20 + readl(S32K_CR);	/* start time in 20 cycles */
	while (readl(S32K_CR) < start) ;	/* dead loop till start time */
	/* get start sys_clk count */
	cstart = readl(OMAP3_GPTIMER1_BASE + TCRR);
	while (readl(S32K_CR) < (start + 20)) ;	/* wait for 40 cycles */
	/* get end sys_clk count */
	cend = readl(OMAP3_GPTIMER1_BASE + TCRR);
	cdiff = cend - cstart;	/* get elapsed ticks */

	if (cdiv == 2)
		cdiff *= 2;

	/* based on number of ticks assign speed */
	if (cdiff > 19000)
		return S38_4M;
	else if (cdiff > 15200)
		return S26M;
	else if (cdiff > 13000)
		return S24M;
	else if (cdiff > 9000)
		return S19_2M;
	else if (cdiff > 7600)
		return S13M;
	else
		return S12M;
}

/**
 * @brief Returns the sys_clkin_sel field value
 *
 * This is based on input oscillator clock frequency.
 *
 * @param[in] osc_clk - Oscilaltor Clock to OMAP
 * @param[out] sys_clkin_sel - returns the sys_clk selection
 *
 * @return void
 */
static void get_sys_clkin_sel(u32 osc_clk, u32 *sys_clkin_sel)
{
	if (osc_clk == S38_4M)
		*sys_clkin_sel = 4;
	else if (osc_clk == S26M)
		*sys_clkin_sel = 3;
	else if (osc_clk == S19_2M)
		*sys_clkin_sel = 2;
	else if (osc_clk == S13M)
		*sys_clkin_sel = 1;
	else if (osc_clk == S12M)
		*sys_clkin_sel = 0;
}

static struct dpll_param core_dpll_param_34x_es1[] = {
	{ .m = 0x19F, .n = 0x0E, .fsel = 0x03, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x1B2, .n = 0x10, .fsel = 0x03, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x19F, .n = 0x17, .fsel = 0x03, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x1B2, .n = 0x21, .fsel = 0x03, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x19F, .n = 0x2F, .fsel = 0x03, .m2 = 0x01, }, /* 38.4 MHz */
};

static struct dpll_param core_dpll_param_34x_es2[] = {
	{ .m = 0x0A6, .n = 0x05, .fsel = 0x07, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x14C, .n = 0x0C, .fsel = 0x03, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x19F, .n = 0x17, .fsel = 0x03, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x0A6, .n = 0x0C, .fsel = 0x07, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x19F, .n = 0x2F, .fsel = 0x03, .m2 = 0x01, }, /* 38.4 MHz */
};

/**
 * @brief Initialize CORE DPLL for OMAP34x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_core_dpll_34x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp;
	if (cpu_rev == OMAP34XX_ES1)
		dp = core_dpll_param_34x_es1;
	else
		dp = core_dpll_param_34x_es2;

	dp += clk_sel;

	if (omap3_running_in_sram()) {
		sr32(OMAP3_CM_REG(CLKEN_PLL), 0, 3, PLL_FAST_RELOCK_BYPASS);
		wait_on_value((0x1 << 0), 0, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);

		/*
		 * OMAP3430 ES1.0 Errata 1.50
		 * Writing default value doesn't work. First write a different
		 * value and then write the default value.
		 */

		/* CM_CLKSEL1_EMU[DIV_DPLL3] */
		sr32(OMAP3_CM_REG(CLKSEL1_EMU), 16, 5, CORE_M3X2 + 1);
		sr32(OMAP3_CM_REG(CLKSEL1_EMU), 16, 5, CORE_M3X2);

		/* M2 (CORE_DPLL_CLKOUT_DIV): CM_CLKSEL1_PLL[27:31] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 27, 2, dp->m2);

		/* M (CORE_DPLL_MULT): CM_CLKSEL1_PLL[16:26] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 16, 11, dp->m);

		/* N (CORE_DPLL_DIV): CM_CLKSEL1_PLL[8:14] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 8, 7, dp->n);

		/* Set source CM_96M_FCLK: CM_CLKSEL1_PLL[6] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 6, 1, 0);

		sr32(OMAP3_CM_REG(CLKSEL_CORE), 8, 4, CORE_SSI_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_CORE), 4, 2, CORE_FUSB_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_CORE), 2, 2, CORE_L4_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_CORE), 0, 2, CORE_L3_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_GFX), 0, 3, GFX_DIV_34X);
		sr32(OMAP3_CM_REG(CLKSEL_WKUP), 1, 2, WKUP_RSM);

		/* FREQSEL (CORE_DPLL_FREQSEL): CM_CLKEN_PLL[4:7] */
		sr32(OMAP3_CM_REG(CLKEN_PLL), 4, 4, dp->fsel);

		/* Lock Mode */
		sr32(OMAP3_CM_REG(CLKEN_PLL), 0, 3, PLL_LOCK);
		wait_on_value((0x1 << 0), 1, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);
	} else if (omap3_running_in_flash()) {
		/***Oopps.. Wrong .config!! *****/
		hang();
	}
}

/* PER DPLL values are same for both ES1 and ES2 */
static struct dpll_param per_dpll_param_34x[] = {
	{ .m = 0x0D8, .n = 0x05, .fsel = 0x07, .m2 = 0x09, }, /* 12   MHz */
	{ .m = 0x1B0, .n = 0x0C, .fsel = 0x03, .m2 = 0x09, }, /* 13   MHz */
	{ .m = 0x0E1, .n = 0x09, .fsel = 0x07, .m2 = 0x09, }, /* 19.2 MHz */
	{ .m = 0x0D8, .n = 0x0C, .fsel = 0x07, .m2 = 0x09, }, /* 26   MHz */
	{ .m = 0x0E1, .n = 0x13, .fsel = 0x07, .m2 = 0x09, }, /* 38.4 MHz */
};

/**
 * @brief Initialize PER DPLL for OMAP34x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_per_dpll_34x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp = per_dpll_param_34x;

	dp += clk_sel;

	/*
	 * OMAP3430 ES1.0 Errata 1.50
	 * Writing default value doesn't work. First write a different
	 * value and then write the default value.
	 */

	sr32(OMAP3_CM_REG(CLKEN_PLL), 16, 3, PLL_STOP);
	wait_on_value((0x1 << 1), 0, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);

	/* Set M6 */
	sr32(OMAP3_CM_REG(CLKSEL1_EMU), 24, 5, PER_M6X2 + 1);
	sr32(OMAP3_CM_REG(CLKSEL1_EMU), 24, 5, PER_M6X2);

	/* Set M5 */
	sr32(OMAP3_CM_REG(CLKSEL_CAM), 0, 5, PER_M5X2 + 1);
	sr32(OMAP3_CM_REG(CLKSEL_CAM), 0, 5, PER_M5X2);

	/* Set M4 */
	sr32(OMAP3_CM_REG(CLKSEL_DSS), 0, 5, PER_M4X2 + 1);
	sr32(OMAP3_CM_REG(CLKSEL_DSS), 0, 5, PER_M4X2);

	/* Set M3 */
	sr32(OMAP3_CM_REG(CLKSEL_DSS), 8, 5, PER_M3X2 + 1);
	sr32(OMAP3_CM_REG(CLKSEL_DSS), 8, 5, PER_M3X2);

	/* Set M2 */
	sr32(OMAP3_CM_REG(CLKSEL3_PLL), 0, 5, dp->m2 + 1);
	sr32(OMAP3_CM_REG(CLKSEL3_PLL), 0, 5, dp->m2);

	/* M (PERIPH_DPLL_MULT): CM_CLKSEL2_PLL[8:18] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL), 8, 11, dp->m);

	/* N (PERIPH_DPLL_DIV): CM_CLKSEL2_PLL[0:6] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL), 0, 7, dp->n);

	/* FREQSEL (PERIPH_DPLL_FREQSEL): CM_CLKEN_PLL[20:23] */
	sr32(OMAP3_CM_REG(CLKEN_PLL), 20, 4, dp->fsel);

	/* LOCK MODE (EN_PERIPH_DPLL): CM_CLKEN_PLL[16:18] */
	sr32(OMAP3_CM_REG(CLKEN_PLL), 16, 3, PLL_LOCK);
	wait_on_value((0x1 << 1), 2, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);
}

static struct dpll_param mpu_dpll_param_34x_es1[] = {
	{ .m = 0x0FE, .n = 0x07, .fsel = 0x05, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x17D, .n = 0x0C, .fsel = 0x03, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x179, .n = 0x12, .fsel = 0x04, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x17D, .n = 0x19, .fsel = 0x03, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x1FA, .n = 0x32, .fsel = 0x03, .m2 = 0x01, }, /* 38.4 MHz */
};

static struct dpll_param mpu_dpll_param_34x_es2[] = {
	{.m = 0x0FA, .n = 0x05, .fsel = 0x07, .m2 = 0x01, }, /* 12   MHz */
	{.m = 0x258, .n = 0x0C, .fsel = 0x03, .m2 = 0x01, }, /* 13   MHz */
	{.m = 0x271, .n = 0x17, .fsel = 0x03, .m2 = 0x01, }, /* 19.2 MHz */
	{.m = 0x0FA, .n = 0x0C, .fsel = 0x07, .m2 = 0x01, }, /* 26   MHz */
	{.m = 0x271, .n = 0x2F, .fsel = 0x03, .m2 = 0x01, }, /* 38.4 MHz */
};

/**
 * @brief Initialize MPU DPLL for OMAP34x
 *
 * The MPU DPLL is already unlocked when control reaches here. This
 * function doesn't lock the DPLL either - defers to the caller.
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_mpu_dpll_34x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp;

	if (cpu_rev == OMAP34XX_ES1)
		dp = mpu_dpll_param_34x_es1;
	else
		dp = mpu_dpll_param_34x_es2;

	dp += clk_sel;

	/* M2 (MPU_DPLL_CLKOUT_DIV) : CM_CLKSEL2_PLL_MPU[0:4] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL_MPU), 0, 5, dp->m2);

	/* M (MPU_DPLL_MULT) : CM_CLKSEL2_PLL_MPU[8:18] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_MPU), 8, 11, dp->m);

	/* N (MPU_DPLL_DIV) : CM_CLKSEL2_PLL_MPU[0:6] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_MPU), 0, 7, dp->n);

	/* FREQSEL (MPU_DPLL_FREQSEL) : CM_CLKEN_PLL_MPU[4:7] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_MPU), 4, 4, dp->fsel);
}

static struct dpll_param iva_dpll_param_34x_es1[] = {
	{ .m = 	0x07D, .n = 0x05, .fsel = 0x07,	.m2 = 0x01, }, /* 12   MHz */
	{ .m = 	0x0FA, .n = 0x0C, .fsel = 0x03,	.m2 = 0x01, }, /* 13   MHz */
	{ .m = 	0x082, .n = 0x09, .fsel = 0x07,	.m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 	0x07D, .n = 0x0C, .fsel = 0x07,	.m2 = 0x01, }, /* 26   MHz */
	{ .m = 	0x13F, .n = 0x30, .fsel = 0x03,	.m2 = 0x01, }, /* 38.4 MHz */
};

static struct dpll_param iva_dpll_param_34x_es2[] = {
	{ .m = 0x0B4, .n = 0x05, .fsel = 0x07, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x168, .n = 0x0C, .fsel = 0x03, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x0E1, .n = 0x0B, .fsel = 0x06, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x0B4, .n = 0x0C, .fsel = 0x07, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x0E1, .n = 0x17, .fsel = 0x06, .m2 = 0x01, }, /* 38.4 MHz */
};

/**
 * @brief Initialize IVA DPLL for OMAP34x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_iva_dpll_34x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp;

	if (cpu_rev == OMAP34XX_ES1)
		dp = iva_dpll_param_34x_es1;
	else
		dp = iva_dpll_param_34x_es2;

	dp += clk_sel;

	/* EN_IVA2_DPLL : CM_CLKEN_PLL_IVA2[0:2] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_IVA2), 0, 3, PLL_STOP);
	wait_on_value((0x1 << 0), 0, OMAP3_CM_REG(IDLEST_PLL_IVA2), LDELAY);

	/* M2 (IVA2_DPLL_CLKOUT_DIV) : CM_CLKSEL2_PLL_IVA2[0:4] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL_IVA2), 0, 5, dp->m2);

	/* M (IVA2_DPLL_MULT) : CM_CLKSEL1_PLL_IVA2[8:18] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_IVA2), 8, 11, dp->m);

	/* N (IVA2_DPLL_DIV) : CM_CLKSEL1_PLL_IVA2[0:6] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_IVA2), 0, 7, dp->n);

	/* FREQSEL (IVA2_DPLL_FREQSEL) : CM_CLKEN_PLL_IVA2[4:7] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_IVA2), 4, 4, dp->fsel);

	/* LOCK (MODE (EN_IVA2_DPLL) : CM_CLKEN_PLL_IVA2[0:2] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_IVA2), 0, 3, PLL_LOCK);
	wait_on_value((0x1 << 0), 1, OMAP3_CM_REG(IDLEST_PLL_IVA2), LDELAY);
}

/* FIXME: All values correspond to 26MHz only */
static struct dpll_param core_dpll_param_36x[] = {
	{ .m = 0x0C8, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x0C8, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x0C8, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x0C8, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x0C8, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 38.4 MHz */
};

/**
 * @brief Initialize CORE DPLL for OMAP36x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_core_dpll_36x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp = core_dpll_param_36x;

	dp += clk_sel;

	if (omap3_running_in_sram()) {
		sr32(OMAP3_CM_REG(CLKEN_PLL), 0, 3, PLL_FAST_RELOCK_BYPASS);
		wait_on_value((0x1 << 0), 0, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);

		/* CM_CLKSEL1_EMU[DIV_DPLL3] */
		sr32(OMAP3_CM_REG(CLKSEL1_EMU), 16, 5, CORE_M3X2);

		/* M2 (CORE_DPLL_CLKOUT_DIV): CM_CLKSEL1_PLL[27:31] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 27, 5, dp->m2);

		/* M (CORE_DPLL_MULT): CM_CLKSEL1_PLL[16:26] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 16, 11, dp->m);

		/* N (CORE_DPLL_DIV): CM_CLKSEL1_PLL[8:14] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 8, 7, dp->n);

		/* Set source CM_96M_FCLK: CM_CLKSEL1_PLL[6] */
		sr32(OMAP3_CM_REG(CLKSEL1_PLL), 6, 1, 0);

		sr32(OMAP3_CM_REG(CLKSEL_CORE), 8, 4, CORE_SSI_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_CORE), 4, 2, CORE_FUSB_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_CORE), 2, 2, CORE_L4_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_CORE), 0, 2, CORE_L3_DIV);
		sr32(OMAP3_CM_REG(CLKSEL_GFX),  0, 3, GFX_DIV_36X);
		sr32(OMAP3_CM_REG(CLKSEL_WKUP), 1, 2, WKUP_RSM);

		/* FREQSEL (CORE_DPLL_FREQSEL): CM_CLKEN_PLL[4:7] */
		sr32(OMAP3_CM_REG(CLKEN_PLL), 4, 4, dp->fsel);

		/* Lock Mode */
		sr32(OMAP3_CM_REG(CLKEN_PLL), 0, 3, PLL_LOCK);
		wait_on_value((0x1 << 0), 1, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);
	} else if (omap3_running_in_flash()) {
		/***Oopps.. Wrong .config!! *****/
		hang();
	}
}

/* FIXME: All values correspond to 26MHz only */
static struct dpll_param_per_36x per_dpll_param_36x[] = {
	{ .m = 0x1B0, .n = 0x0C, .m2 = 9, .m3 = 0x10, .m4 = 9, .m5 = 4,	.m6 = 3, .m2div = 1, },  /* 12   MHz */
	{ .m = 0x1B0, .n = 0x0C, .m2 = 9, .m3 = 0x10, .m4 = 9, .m5 = 4,	.m6 = 3, .m2div = 1, },  /* 13   MHz */
	{ .m = 0x1B0, .n = 0x0C, .m2 = 9, .m3 = 0x10, .m4 = 9, .m5 = 4,	.m6 = 3, .m2div = 1, },  /* 19.2 MHz */
	{ .m = 0x1B0, .n = 0x0C, .m2 = 9, .m3 = 0x10, .m4 = 9, .m5 = 4,	.m6 = 3, .m2div = 1, },  /* 26   MHz */
	{ .m = 0x1B0, .n = 0x0C, .m2 = 9, .m3 = 0x10, .m4 = 9, .m5 = 4,	.m6 = 3, .m2div = 1, },  /* 38.4 MHz */
};

/**
 * @brief Initialize PER DPLL for OMAP36x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_per_dpll_36x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param_per_36x *dp = per_dpll_param_36x;

	dp += clk_sel;

	sr32(OMAP3_CM_REG(CLKEN_PLL), 16, 3, PLL_STOP);
	wait_on_value((0x1 << 1), 0, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);

	/* Set M6 (DIV_DPLL4): CM_CLKSEL1_EMU[24:29] */
	sr32(OMAP3_CM_REG(CLKSEL1_EMU), 24, 6, dp->m6);

	/* Set M5 (CLKSEL_CAM): CM_CLKSEL_CAM[0:5] */
	sr32(OMAP3_CM_REG(CLKSEL_CAM), 0, 6, dp->m5);

	/* Set M4 (CLKSEL_DSS1): CM_CLKSEL_DSS[0:5] */
	sr32(OMAP3_CM_REG(CLKSEL_DSS), 0, 6, dp->m4);

	/* Set M3 (CLKSEL_DSS2): CM_CLKSEL_DSS[8:13] */
	sr32(OMAP3_CM_REG(CLKSEL_DSS), 8, 6, dp->m3);

	/* Set M2: CM_CLKSEL3_PLL[0:4] */
	sr32(OMAP3_CM_REG(CLKSEL3_PLL), 0, 5, dp->m2);

	/* M (PERIPH_DPLL_MULT): CM_CLKSEL2_PLL[8:19] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL), 8, 12, dp->m);

	/* N (PERIPH_DPLL_DIV): CM_CLKSEL2_PLL[0:6] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL), 0, 7, dp->n);

	/* M2DIV (CLKSEL_96M): CM_CLKSEL_CORE[12:13] */
	sr32(OMAP3_CM_REG(CLKSEL_CORE), 12, 2, dp->m2div);

	/* LOCK MODE (EN_PERIPH_DPLL): CM_CLKEN_PLL[16:18] */
	sr32(OMAP3_CM_REG(CLKEN_PLL), 16, 3, PLL_LOCK);
	wait_on_value((0x1 << 1), 2, OMAP3_CM_REG(IDLEST_CKGEN), LDELAY);
}

/* FIXME: All values correspond to 26MHz only */
static struct dpll_param mpu_dpll_param_36x[] = {
	{ .m = 0x12C, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x12C, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x12C, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x12C, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x12C, .n = 0x0C, .fsel = 0x00, .m2 = 0x01, }, /* 38.4 MHz */
};

/**
 * @brief Initialize MPU DPLL for OMAP36x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_mpu_dpll_36x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp = mpu_dpll_param_36x;

	dp += clk_sel;

	/* M2 (MPU_DPLL_CLKOUT_DIV) : CM_CLKSEL2_PLL_MPU[0:4] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL_MPU), 0, 5, dp->m2);

	/* M (MPU_DPLL_MULT) : CM_CLKSEL2_PLL_MPU[8:18] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_MPU), 8, 11, dp->m);

	/* N (MPU_DPLL_DIV) : CM_CLKSEL2_PLL_MPU[0:6] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_MPU), 0, 7, dp->n);

	/* FREQSEL (MPU_DPLL_FREQSEL) : CM_CLKEN_PLL_MPU[4:7] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_MPU), 4, 4, dp->fsel);
}

/* FIXME: All values correspond to 26MHz only */
static struct dpll_param iva_dpll_param_36x[] = {
	{ .m = 0x00A, .n = 0x00, .fsel = 0x00, .m2 = 0x01, }, /* 12   MHz */
	{ .m = 0x00A, .n = 0x00, .fsel = 0x00, .m2 = 0x01, }, /* 13   MHz */
	{ .m = 0x00A, .n = 0x00, .fsel = 0x00, .m2 = 0x01, }, /* 19.2 MHz */
	{ .m = 0x00A, .n = 0x00, .fsel = 0x00, .m2 = 0x01, }, /* 26   MHz */
	{ .m = 0x00A, .n = 0x00, .fsel = 0x00, .m2 = 0x01, }, /* 38.4 MHz */
};

/**
 * @brief Initialize IVA DPLL for OMAP36x
 *
 * @param[in] cpu_rev - Silicon revision
 * @param[in] clk_sel - Clock selection used as index into the dpll table
 */
static void init_iva_dpll_36x(u32 cpu_rev, u32 clk_sel)
{
	struct dpll_param *dp = iva_dpll_param_36x;

	dp += clk_sel;

	/* EN_IVA2_DPLL : CM_CLKEN_PLL_IVA2[0:2] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_IVA2), 0, 3, PLL_STOP);
	wait_on_value((0x1 << 0), 0, OMAP3_CM_REG(IDLEST_PLL_IVA2), LDELAY);

	/* M2 (IVA2_DPLL_CLKOUT_DIV) : CM_CLKSEL2_PLL_IVA2[0:4] */
	sr32(OMAP3_CM_REG(CLKSEL2_PLL_IVA2), 0, 5, dp->m2);

	/* M (IVA2_DPLL_MULT) : CM_CLKSEL1_PLL_IVA2[8:18] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_IVA2), 8, 11, dp->m);

	/* N (IVA2_DPLL_DIV) : CM_CLKSEL1_PLL_IVA2[0:6] */
	sr32(OMAP3_CM_REG(CLKSEL1_PLL_IVA2), 0, 7, dp->n);

	/* FREQSEL (IVA2_DPLL_FREQSEL) : CM_CLKEN_PLL_IVA2[4:7] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_IVA2), 4, 4, dp->fsel);

	/* LOCK (MODE (EN_IVA2_DPLL) : CM_CLKEN_PLL_IVA2[0:2] */
	sr32(OMAP3_CM_REG(CLKEN_PLL_IVA2), 0, 3, PLL_LOCK);
	wait_on_value((0x1 << 0), 1, OMAP3_CM_REG(IDLEST_PLL_IVA2), LDELAY);
}

/**
 * @brief Inits clocks for PRCM
 *
 * This is called from SRAM
 *
 * @return void
 */
void prcm_init(void)
{
	u32 osc_clk = 0, sys_clkin_sel = 0;
	u32 cpu_type = get_cpu_type();
	u32 cpu_rev = get_cpu_rev();
	u32 clk_index;

	/* Gauge the input clock speed and find out the sys_clkin_sel
	 * value corresponding to the input clock.
	 */
	osc_clk = get_osc_clk_speed();
	get_sys_clkin_sel(osc_clk, &sys_clkin_sel);
	/* set input crystal speed */
	sr32(OMAP3_PRM_REG(CLKSEL), 0, 3, sys_clkin_sel);

	/*
	 * OMAP3430:
	 *	If the input clock is greater than 19.2M always divide/2
	 * OMAP3630:
	 *	DDR corruption was observed on exit from OFF mode, when
	 *	sys clock is lower than 26M. As workaround, it is maintained
	 *	at 26M.
	 */
	if ((cpu_type != CPU_3630) && (sys_clkin_sel > 2)) {
		/* input clock divider */
		sr32(OMAP3_PRM_REG(CLKSRC_CTRL), 6, 2, 2);
		clk_index = sys_clkin_sel / 2;
	} else {
		/* input clock divider */
		sr32(OMAP3_PRM_REG(CLKSRC_CTRL), 6, 2, 1);
		clk_index = sys_clkin_sel;
	}

	/*
	 * Unlock the MPU PLL. Run slow while clocks are being configured.
	 */
	sr32(OMAP3_CM_REG(CLKEN_PLL_MPU), 0, 3, PLL_LOW_POWER_BYPASS);
	wait_on_value((0x1 << 0), 0, OMAP3_CM_REG(IDLEST_PLL_MPU), LDELAY);

	if (cpu_type == CPU_3430 || cpu_type == CPU_AM35XX) {
		init_core_dpll_34x(cpu_rev, clk_index);
		init_per_dpll_34x(cpu_rev, clk_index);
		init_mpu_dpll_34x(cpu_rev, clk_index);
		if (cpu_type != CPU_AM35XX)
			init_iva_dpll_34x(cpu_rev, clk_index);
	}
	else if (cpu_type == CPU_3630) {
		init_core_dpll_36x(cpu_rev, clk_index);
		init_per_dpll_36x(cpu_rev, clk_index);
		init_mpu_dpll_36x(cpu_rev, clk_index);
		init_iva_dpll_36x(cpu_rev, clk_index);
	}
	else {
		/* Unknown CPU */
		hang();
	}

	/*
	 * Clock configuration complete. Lock MPU PLL.
	 */
	sr32(OMAP3_CM_REG(CLKEN_PLL_MPU), 0, 3, PLL_LOCK);
	wait_on_value((0x1 << 0), 1, OMAP3_CM_REG(IDLEST_PLL_MPU), LDELAY);

	/* Set up GPTimers to sys_clk source only */
	sr32(OMAP3_CM_REG(CLKSEL_PER), 0, 8, 0xff);
	sr32(OMAP3_CM_REG(CLKSEL_WKUP), 0, 1, 1);

	sdelay(5000);

	/* Enable Peripheral Clocks */
	per_clocks_enable();
}

/**
 * @brief Enable the clks & power for perifs
 *
 * @return void
 */
static void per_clocks_enable(void)
{
	/* Enable GP2 timer. */
	sr32(OMAP3_CM_REG(CLKSEL_PER), 0, 1, 0x1);	/* GPT2 = sys clk */
	sr32(OMAP3_CM_REG(ICLKEN_PER), 3, 1, 0x1);	/* ICKen GPT2 */
	sr32(OMAP3_CM_REG(FCLKEN_PER), 3, 1, 0x1);	/* FCKen GPT2 */
	/* Enable the ICLK for 32K Sync Timer as its used in udelay */
	sr32(OMAP3_CM_REG(ICLKEN_WKUP), 2, 1, 0x1);

#define FCK_IVA2_ON	0x00000001
#define FCK_CORE1_ON	0x03fffe29
#define ICK_CORE1_ON	0x3ffffffb
#define ICK_CORE2_ON	0x0000001f
#define	FCK_WKUP_ON	0x000000e9
#define ICK_WKUP_ON	0x0000003f
#define FCK_DSS_ON	0x00000005	/* tv+dss1 (not dss2) */
#define ICK_DSS_ON	0x00000001
#define FCK_CAM_ON	0x00000001
#define ICK_CAM_ON	0x00000001
#define FCK_PER_ON	0x0003ffff
#define ICK_PER_ON	0x0003ffff

	if (get_cpu_type() != CPU_AM35XX) {
		sr32(OMAP3_CM_REG(FCLKEN_IVA2), 0, 32, FCK_IVA2_ON);
		sr32(OMAP3_CM_REG(FCLKEN_CAM), 0, 32, FCK_CAM_ON);
		sr32(OMAP3_CM_REG(ICLKEN_CAM), 0, 32, ICK_CAM_ON);
	}
	sr32(OMAP3_CM_REG(FCLKEN1_CORE), 0, 32, FCK_CORE1_ON);
	sr32(OMAP3_CM_REG(ICLKEN1_CORE), 0, 32, ICK_CORE1_ON);
	sr32(OMAP3_CM_REG(ICLKEN2_CORE), 0, 32, ICK_CORE2_ON);
	sr32(OMAP3_CM_REG(FCLKEN_WKUP), 0, 32, FCK_WKUP_ON);
	sr32(OMAP3_CM_REG(ICLKEN_WKUP), 0, 32, ICK_WKUP_ON);
	sr32(OMAP3_CM_REG(FCLKEN_DSS), 0, 32, FCK_DSS_ON);
	sr32(OMAP3_CM_REG(ICLKEN_DSS), 0, 32, ICK_DSS_ON);
	sr32(OMAP3_CM_REG(FCLKEN_PER), 0, 32, FCK_PER_ON);
	sr32(OMAP3_CM_REG(ICLKEN_PER), 0, 32, ICK_PER_ON);

	/* Settle down my friend */
	sdelay(1000);
}
