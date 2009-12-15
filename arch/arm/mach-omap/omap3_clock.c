/**
 * @file
 * @brief OMAP DPLL and various clock configuration
 *
 * FileName: arch/arm/mach-omap/omap3_clock.c
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
 */
/*
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <mach/silicon.h>
#include <mach/clocks.h>
#include <mach/timers.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>

/* Following functions are exported from omap3_clock_core.S */
#ifdef CONFIG_OMAP3_COPY_CLOCK_SRAM
/* A.K.A go_to_speed */
static void (*f_lock_pll) (u32, u32, u32, u32);
#endif
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
	u32 start, cstart, cend, cdiff, val;

	val = readl(PRM_REG(CLKSRC_CTRL));
	/* If SYS_CLK is being divided by 2, remove for now */
	val = (val & (~(0x1 << 7))) | (0x1 << 6);
	writel(val, PRM_REG(CLKSRC_CTRL));

	/* enable timer2 */
	val = readl(CM_REG(CLKSEL_WKUP)) | (0x1 << 0);
	writel(val, CM_REG(CLKSEL_WKUP));	/* select sys_clk for GPT1 */

	/* Enable I and F Clocks for GPT1 */
	val = readl(CM_REG(ICLKEN_WKUP)) | (0x1 << 0) | (0x1 << 2);
	writel(val, CM_REG(ICLKEN_WKUP));
	val = readl(CM_REG(FCLKEN_WKUP)) | (0x1 << 0);
	writel(val, CM_REG(FCLKEN_WKUP));
	/* start counting at 0 */
	writel(0, OMAP_GPTIMER1_BASE + TLDR);
	/* enable clock */
	writel(GPT_EN, OMAP_GPTIMER1_BASE + TCLR);
	/* enable 32kHz source - enabled out of reset */
	/* determine sys_clk via gauging */

	start = 20 + readl(S32K_CR);	/* start time in 20 cycles */
	while (readl(S32K_CR) < start) ;	/* dead loop till start time */
	/* get start sys_clk count */
	cstart = readl(OMAP_GPTIMER1_BASE + TCRR);
	while (readl(S32K_CR) < (start + 20)) ;	/* wait for 40 cycles */
	/* get end sys_clk count */
	cend = readl(OMAP_GPTIMER1_BASE + TCRR);
	cdiff = cend - cstart;	/* get elapsed ticks */

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

/**
 * @brief Inits clocks for PRCM
 *
 * This is called from SRAM, or Flash (using temp SRAM stack).
 * if CONFIG_OMAP3_COPY_CLOCK_SRAM is defined, @ref go_to_speed
 *
 * @return void
 */
void prcm_init(void)
{
	int xip_safe;
	u32 osc_clk = 0, sys_clkin_sel = 0;
	u32 clk_index, sil_index;
	struct dpll_param *dpll_param_p;
#ifdef CONFIG_OMAP3_COPY_CLOCK_SRAM
	int p0, p1, p2, p3;
	f_lock_pll = (void *)(OMAP_SRAM_INTVECT + OMAP_SRAM_INTVECT_COPYSIZE);
#endif

	xip_safe = running_in_sram();

	/* Gauge the input clock speed and find out the sys_clkin_sel
	 * value corresponding to the input clock.
	 */
	osc_clk = get_osc_clk_speed();
	get_sys_clkin_sel(osc_clk, &sys_clkin_sel);
	/* set input crystal speed */
	sr32(PRM_REG(CLKSEL), 0, 3, sys_clkin_sel);

	/* If the input clock is greater than 19.2M always divide/2 */
	if (sys_clkin_sel > 2) {
		/* input clock divider */
		sr32(PRM_REG(CLKSRC_CTRL), 6, 2, 2);
		clk_index = sys_clkin_sel / 2;
	} else {
		/* input clock divider */
		sr32(PRM_REG(CLKSRC_CTRL), 6, 2, 1);
		clk_index = sys_clkin_sel;
	}

	/* The DPLL tables are defined according to sysclk value and
	 * silicon revision. The clk_index value will be used to get
	 * the values for that input sysclk from the DPLL param table
	 * and sil_index will get the values for that SysClk for the
	 * appropriate silicon rev.
	 */
	if (get_cpu_rev() >= CPU_ES2)
		sil_index = 1;
	/* Unlock MPU DPLL (slows things down, and needed later) */
	sr32(CM_REG(CLKEN_PLL_MPU), 0, 3, PLL_LOW_POWER_BYPASS);
	wait_on_value((0x1 << 0), 0, CM_REG(IDLEST_PLL_MPU), LDELAY);

	/* Getting the base address of Core DPLL param table */
	dpll_param_p = (struct dpll_param *)get_core_dpll_param();
	/* Moving it to the right sysclk and ES rev base */
	dpll_param_p = dpll_param_p + MAX_SIL_INDEX * clk_index + sil_index;
	if (xip_safe) {
		/* CORE DPLL */
		/* sr32(CM_REG(CLKSEL2_EMU)) set override to work when asleep */
		sr32(CM_REG(CLKEN_PLL), 0, 3, PLL_FAST_RELOCK_BYPASS);
		wait_on_value((0x1 << 0), 0, CM_REG(IDLEST_CKGEN), LDELAY);
		/* For 3430 ES1.0 Errata 1.50, default value directly doesnt
		   work. write another value and then default value. */
		sr32(CM_REG(CLKSEL1_EMU), 16, 5, CORE_M3X2 + 1);
		sr32(CM_REG(CLKSEL1_EMU), 16, 5, CORE_M3X2);
		sr32(CM_REG(CLKSEL1_PLL), 27, 2, dpll_param_p->m2);
		sr32(CM_REG(CLKSEL1_PLL), 16, 11, dpll_param_p->m);
		sr32(CM_REG(CLKSEL1_PLL), 8, 7, dpll_param_p->n);
		sr32(CM_REG(CLKSEL1_PLL), 6, 1, 0);
		sr32(CM_REG(CLKSEL_CORE), 8, 4, CORE_SSI_DIV);
		sr32(CM_REG(CLKSEL_CORE), 4, 2, CORE_FUSB_DIV);
		sr32(CM_REG(CLKSEL_CORE), 2, 2, CORE_L4_DIV);
		sr32(CM_REG(CLKSEL_CORE), 0, 2, CORE_L3_DIV);
		sr32(CM_REG(CLKSEL_GFX), 0, 3, GFX_DIV);
		sr32(CM_REG(CLKSEL_WKUP), 1, 2, WKUP_RSM);
		sr32(CM_REG(CLKEN_PLL), 4, 4, dpll_param_p->fsel);
		sr32(CM_REG(CLKEN_PLL), 0, 3, PLL_LOCK);
		wait_on_value((0x1 << 0), 1, CM_REG(IDLEST_CKGEN), LDELAY);
	} else if (running_in_flash()) {
#ifdef CONFIG_OMAP3_COPY_CLOCK_SRAM
		/* if running from flash,
		 * jump to small relocated code area in SRAM.
		 */
		p0 = readl(CM_REG(CLKEN_PLL));
		sr32((u32) &p0, 0, 3, PLL_FAST_RELOCK_BYPASS);
		sr32((u32) &p0, 4, 4, dpll_param_p->fsel);

		p1 = readl(CM_REG(CLKSEL1_PLL));
		sr32((u32) &p1, 27, 2, dpll_param_p->m2);
		sr32((u32) &p1, 16, 11, dpll_param_p->m);
		sr32((u32) &p1, 8, 7, dpll_param_p->n);
		sr32((u32) &p1, 6, 1, 0);	/* set source for 96M */
		p2 = readl(CM_REG(CLKSEL_CORE));
		sr32((u32) &p2, 8, 4, CORE_SSI_DIV);
		sr32((u32) &p2, 4, 2, CORE_FUSB_DIV);
		sr32((u32) &p2, 2, 2, CORE_L4_DIV);
		sr32((u32) &p2, 0, 2, CORE_L3_DIV);

		p3 = CM_REG(IDLEST_CKGEN);

		(*f_lock_pll) (p0, p1, p2, p3);
#else
		/***Oopps.. Wrong .config!! *****/
		hang();
#endif
	}

	/* PER DPLL */
	sr32(CM_REG(CLKEN_PLL), 16, 3, PLL_STOP);
	wait_on_value((0x1 << 1), 0, CM_REG(IDLEST_CKGEN), LDELAY);

	/* Getting the base address to PER  DPLL param table */
	/* Set N */
	dpll_param_p = (struct dpll_param *)get_per_dpll_param();
	/* Moving it to the right sysclk base */
	dpll_param_p = dpll_param_p + clk_index;
	/* Errata 1.50 Workaround for 3430 ES1.0 only */
	/* If using default divisors, write default divisor + 1
	   and then the actual divisor value */
	/* Need to change it to silicon and revisino check */
	if (1) {
		sr32(CM_REG(CLKSEL1_EMU), 24, 5, PER_M6X2 + 1);	/* set M6 */
		sr32(CM_REG(CLKSEL1_EMU), 24, 5, PER_M6X2);	/* set M6 */
		sr32(CM_REG(CLKSEL_CAM), 0, 5, PER_M5X2 + 1);	/* set M5 */
		sr32(CM_REG(CLKSEL_CAM), 0, 5, PER_M5X2);	/* set M5 */
		sr32(CM_REG(CLKSEL_DSS), 0, 5, PER_M4X2 + 1);	/* set M4 */
		sr32(CM_REG(CLKSEL_DSS), 0, 5, PER_M4X2);	/* set M4 */
		sr32(CM_REG(CLKSEL_DSS), 8, 5, PER_M3X2 + 1);	/* set M3 */
		sr32(CM_REG(CLKSEL_DSS), 8, 5, PER_M3X2);	/* set M3 */
		/* set M2 */
		sr32(CM_REG(CLKSEL3_PLL), 0, 5, dpll_param_p->m2 + 1);
		sr32(CM_REG(CLKSEL3_PLL), 0, 5, dpll_param_p->m2);
	} else {
		sr32(CM_REG(CLKSEL1_EMU), 24, 5, PER_M6X2);	/* set M6 */
		sr32(CM_REG(CLKSEL_CAM), 0, 5, PER_M5X2);	/* set M5 */
		sr32(CM_REG(CLKSEL_DSS), 0, 5, PER_M4X2);	/* set M4 */
		sr32(CM_REG(CLKSEL_DSS), 8, 5, PER_M3X2);	/* set M3 */
		sr32(CM_REG(CLKSEL3_PLL), 0, 5, dpll_param_p->m2);
	}
	sr32(CM_REG(CLKSEL2_PLL), 8, 11, dpll_param_p->m);	/* set m */
	sr32(CM_REG(CLKSEL2_PLL), 0, 7, dpll_param_p->n);	/* set n */
	sr32(CM_REG(CLKEN_PLL), 20, 4, dpll_param_p->fsel);	/* FREQSEL */
	sr32(CM_REG(CLKEN_PLL), 16, 3, PLL_LOCK);	/* lock mode */
	wait_on_value((0x1 << 1), 2, CM_REG(IDLEST_CKGEN), LDELAY);

	/* Getting the base address to MPU DPLL param table */
	dpll_param_p = (struct dpll_param *)get_mpu_dpll_param();
	/* Moving it to the right sysclk and ES rev base */
	dpll_param_p = dpll_param_p + MAX_SIL_INDEX * clk_index + sil_index;
	/* MPU DPLL (unlocked already) */
	sr32(CM_REG(CLKSEL2_PLL_MPU), 0, 5, dpll_param_p->m2);	/* Set M2 */
	sr32(CM_REG(CLKSEL1_PLL_MPU), 8, 11, dpll_param_p->m);	/* Set M */
	sr32(CM_REG(CLKSEL1_PLL_MPU), 0, 7, dpll_param_p->n);	/* Set N */
	sr32(CM_REG(CLKEN_PLL_MPU), 4, 4, dpll_param_p->fsel);	/* FREQSEL */
	sr32(CM_REG(CLKEN_PLL_MPU), 0, 3, PLL_LOCK);	/* lock mode */
	wait_on_value((0x1 << 0), 1, CM_REG(IDLEST_PLL_MPU), LDELAY);

	/* Getting the base address to IVA DPLL param table */
	dpll_param_p = (struct dpll_param *)get_iva_dpll_param();
	/* Moving it to the right sysclk and ES rev base */
	dpll_param_p = dpll_param_p + MAX_SIL_INDEX * clk_index + sil_index;
	/* IVA DPLL (set to 12*20=240MHz) */
	sr32(CM_REG(CLKEN_PLL_IVA2), 0, 3, PLL_STOP);
	wait_on_value((0x1 << 0), 0, CM_REG(IDLEST_PLL_IVA2), LDELAY);
	sr32(CM_REG(CLKSEL2_PLL_IVA2), 0, 5, dpll_param_p->m2);	/* set M2 */
	sr32(CM_REG(CLKSEL1_PLL_IVA2), 8, 11, dpll_param_p->m);	/* set M */
	sr32(CM_REG(CLKSEL1_PLL_IVA2), 0, 7, dpll_param_p->n);	/* set N */
	sr32(CM_REG(CLKEN_PLL_IVA2), 4, 4, dpll_param_p->fsel);	/* FREQSEL */
	sr32(CM_REG(CLKEN_PLL_IVA2), 0, 3, PLL_LOCK);	/* lock mode */
	wait_on_value((0x1 << 0), 1, CM_REG(IDLEST_PLL_IVA2), LDELAY);

	/* Set up GPTimers to sys_clk source only */
	sr32(CM_REG(CLKSEL_PER), 0, 8, 0xff);
	sr32(CM_REG(CLKSEL_WKUP), 0, 1, 1);

	sdelay(5000);

	/* Enable Peripheral Clocks */
	per_clocks_enable();
}

/**
 * @brief Enable the clks & power for perifs
 *
 * GPT2 Sysclk, ICLK,FCLK, 32k Sync is enabled by default
 * Uses CONFIG_OMAP_CLOCK_UART to enable UART clocks
 * Uses CONFIG_OMAP_CLOCK_I2C to enable I2C clocks
 * Uses CONFIG_OMAP_CLOCK_ALL to enable All Clocks!
 *    - Not a wise idea in most cases
 *
 * @return void
 */
static void per_clocks_enable(void)
{
	/* Enable GP2 timer. */
	sr32(CM_REG(CLKSEL_PER), 0, 1, 0x1);	/* GPT2 = sys clk */
	sr32(CM_REG(ICLKEN_PER), 3, 1, 0x1);	/* ICKen GPT2 */
	sr32(CM_REG(FCLKEN_PER), 3, 1, 0x1);	/* FCKen GPT2 */
	/* Enable the ICLK for 32K Sync Timer as its used in udelay */
	sr32(CM_REG(ICLKEN_WKUP), 2, 1, 0x1);

#ifdef CONFIG_OMAP_CLOCK_UART
	/* Enable UART1 clocks */
	sr32(CM_REG(FCLKEN1_CORE), 13, 1, 0x1);
	sr32(CM_REG(ICLKEN1_CORE), 13, 1, 0x1);
#endif
#ifdef CONFIG_OMAP_CLOCK_I2C
	/* Turn on all 3 I2C clocks */
	sr32(CM_REG(FCLKEN1_CORE), 15, 3, 0x7);
	sr32(CM_REG(ICLKEN1_CORE), 15, 3, 0x7);	/* I2C1,2,3 = on */
#endif

#ifdef CONFIG_OMAP_CLOCK_ALL
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
	sr32(CM_REG(FCLKEN_IVA2), 0, 32, FCK_IVA2_ON);
	sr32(CM_REG(FCLKEN1_CORE), 0, 32, FCK_CORE1_ON);
	sr32(CM_REG(ICLKEN1_CORE), 0, 32, ICK_CORE1_ON);
	sr32(CM_REG(ICLKEN2_CORE), 0, 32, ICK_CORE2_ON);
	sr32(CM_REG(FCLKEN_WKUP), 0, 32, FCK_WKUP_ON);
	sr32(CM_REG(ICLKEN_WKUP), 0, 32, ICK_WKUP_ON);
	sr32(CM_REG(FCLKEN_DSS), 0, 32, FCK_DSS_ON);
	sr32(CM_REG(ICLKEN_DSS), 0, 32, ICK_DSS_ON);
	sr32(CM_REG(FCLKEN_CAM), 0, 32, FCK_CAM_ON);
	sr32(CM_REG(ICLKEN_CAM), 0, 32, ICK_CAM_ON);
	sr32(CM_REG(FCLKEN_PER), 0, 32, FCK_PER_ON);
	sr32(CM_REG(ICLKEN_PER), 0, 32, ICK_PER_ON);
#endif
	/* Settle down my friend */
	sdelay(1000);
}
