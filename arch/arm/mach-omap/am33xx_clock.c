/*
 * pll.c
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <asm/io.h>
#include <mach/am33xx-clock.h>
#include <mach/am33xx-generic.h>
#include <asm-generic/div64.h>

#define PRCM_MOD_EN		0x2
#define	PRCM_FORCE_WAKEUP	0x2

#define PRCM_EMIF_CLK_ACTIVITY	(0x1 << 2)
#define PRCM_L3_GCLK_ACTIVITY	(0x1 << 4)

#define PLL_BYPASS_MODE	0x4
#define PLL_LOCK_MODE	0x7
#define PLL_MULTIPLIER_SHIFT	 8

static void interface_clocks_enable(void)
{
	/* Enable all the Interconnect Modules */
	__raw_writel(PRCM_MOD_EN, CM_PER_L3_CLKCTRL);
	while (__raw_readl(CM_PER_L3_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_PER_L4LS_CLKCTRL);
	while (__raw_readl(CM_PER_L4LS_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_PER_L4FW_CLKCTRL);
	while (__raw_readl(CM_PER_L4FW_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_WKUP_L4WKUP_CLKCTRL);
	while (__raw_readl(CM_WKUP_L4WKUP_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_PER_L3_INSTR_CLKCTRL);
	while (__raw_readl(CM_PER_L3_INSTR_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_PER_L4HS_CLKCTRL);
	while (__raw_readl(CM_PER_L4HS_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_PER_SPI1_CLKCTRL);
	while (__raw_readl(CM_PER_SPI1_CLKCTRL) != PRCM_MOD_EN);

	/* GPIO0 */
	__raw_writel(PRCM_MOD_EN, CM_WKUP_GPIO0_CLKCTRL);
	while (__raw_readl(CM_WKUP_GPIO0_CLKCTRL) != PRCM_MOD_EN);

	/* GPIO1 */
	__raw_writel(PRCM_MOD_EN, CM_PER_GPIO1_CLKCTRL);
	while (__raw_readl(CM_PER_GPIO1_CLKCTRL) != PRCM_MOD_EN);

	/* GPIO2 */
	__raw_writel(PRCM_MOD_EN, CM_PER_GPIO2_CLKCTRL);
	while (__raw_readl(CM_PER_GPIO2_CLKCTRL) != PRCM_MOD_EN);

	/* GPIO3 */
	__raw_writel(PRCM_MOD_EN, CM_PER_GPIO3_CLKCTRL);
	while (__raw_readl(CM_PER_GPIO3_CLKCTRL) != PRCM_MOD_EN);
}

static void power_domain_transition_enable(void)
{
	/*
	 * Force power domain wake up transition
	 * Ensure that the corresponding interface clock is active before
	 * using the peripheral
	 */
	__raw_writel(PRCM_FORCE_WAKEUP, CM_PER_L3_CLKSTCTRL);

	__raw_writel(PRCM_FORCE_WAKEUP, CM_PER_L4LS_CLKSTCTRL);

	__raw_writel(PRCM_FORCE_WAKEUP, CM_WKUP_CLKSTCTRL);

	__raw_writel(PRCM_FORCE_WAKEUP, CM_PER_L4FW_CLKSTCTRL);

	__raw_writel(PRCM_FORCE_WAKEUP, CM_PER_L3S_CLKSTCTRL);
}

/*
 * Enable the module clock and the power domain for required peripherals
 */
void am33xx_enable_per_clocks(void)
{
	u32 clkdcoldo;

	/* Enable the module clock */
	__raw_writel(PRCM_MOD_EN, CM_PER_TIMER2_CLKCTRL);
	while (__raw_readl(CM_PER_TIMER2_CLKCTRL) != PRCM_MOD_EN);

	/* Select the Master osc 24 MHZ as Timer2 clock source */
	__raw_writel(0x1, CLKSEL_TIMER2_CLK);

	/* UART0 */
	__raw_writel(PRCM_MOD_EN, CM_WKUP_UART0_CLKCTRL);
	while (__raw_readl(CM_WKUP_UART0_CLKCTRL) != PRCM_MOD_EN);

	/* UART1 */
	__raw_writel(PRCM_MOD_EN, CM_PER_UART1_CLKCTRL);
	while (__raw_readl(CM_PER_UART1_CLKCTRL) != PRCM_MOD_EN);

	/* UART2 */
	__raw_writel(PRCM_MOD_EN, CM_PER_UART2_CLKCTRL);
	while (__raw_readl(CM_PER_UART2_CLKCTRL) != PRCM_MOD_EN);

	/* UART3 */
	__raw_writel(PRCM_MOD_EN, CM_PER_UART3_CLKCTRL);
	while (__raw_readl(CM_PER_UART3_CLKCTRL) != PRCM_MOD_EN);

	/* GPMC */
	__raw_writel(PRCM_MOD_EN, CM_PER_GPMC_CLKCTRL);
	while (__raw_readl(CM_PER_GPMC_CLKCTRL) != PRCM_MOD_EN);

	/* ELM */
	__raw_writel(PRCM_MOD_EN, CM_PER_ELM_CLKCTRL);
	while (__raw_readl(CM_PER_ELM_CLKCTRL) != PRCM_MOD_EN);

	/* i2c0 */
	__raw_writel(PRCM_MOD_EN, CM_WKUP_I2C0_CLKCTRL);
	while (__raw_readl(CM_WKUP_I2C0_CLKCTRL) != PRCM_MOD_EN);

	/* i2c1 */
	__raw_writel(PRCM_MOD_EN, CM_PER_I2C1_CLKCTRL);
	while (__raw_readl(CM_PER_I2C1_CLKCTRL) != PRCM_MOD_EN);

	/* i2c2 */
	__raw_writel(PRCM_MOD_EN, CM_PER_I2C2_CLKCTRL);
	while (__raw_readl(CM_PER_I2C2_CLKCTRL) != PRCM_MOD_EN);

	/* Ethernet */
	__raw_writel(PRCM_MOD_EN, CM_PER_CPGMAC0_CLKCTRL);
	__raw_writel(PRCM_MOD_EN, CM_PER_CPSW_CLKSTCTRL);
	while ((__raw_readl(CM_PER_CPGMAC0_CLKCTRL) & 0x30000) != 0x0);

	/* MMC 0 & 1 */
	__raw_writel(PRCM_MOD_EN, CM_PER_MMC0_CLKCTRL);
	while (__raw_readl(CM_PER_MMC0_CLKCTRL) != PRCM_MOD_EN);
	__raw_writel(PRCM_MOD_EN, CM_PER_MMC1_CLKCTRL);
	while (__raw_readl(CM_PER_MMC1_CLKCTRL) != PRCM_MOD_EN);

	/* Enable the control module though RBL would have done it*/
	__raw_writel(PRCM_MOD_EN, CM_WKUP_CONTROL_CLKCTRL);
	while (__raw_readl(CM_WKUP_CONTROL_CLKCTRL) != PRCM_MOD_EN);

	/* SPI 0 & 1 */
	__raw_writel(PRCM_MOD_EN, CM_PER_SPI0_CLKCTRL);
	while (__raw_readl(CM_PER_SPI0_CLKCTRL) != PRCM_MOD_EN);

	__raw_writel(PRCM_MOD_EN, CM_PER_SPI1_CLKCTRL);
	while (__raw_readl(CM_PER_SPI1_CLKCTRL) != PRCM_MOD_EN);

	/* USB */
	__raw_writel(PRCM_MOD_EN, CM_PER_USB0_CLKCTRL);
	while ((__raw_readl(CM_PER_USB0_CLKCTRL) & 0x30000) != 0x0);

	clkdcoldo = __raw_readl(CM_CLKDCOLDO_DPLL_PER);
	clkdcoldo = clkdcoldo | 0x100;
	__raw_writel(clkdcoldo, CM_CLKDCOLDO_DPLL_PER);
	while ((__raw_readl(CM_CLKDCOLDO_DPLL_PER) & 0x00000200) != 0x200);
}

static void mpu_pll_config(int mpupll_M, int osc)
{
	u32 clkmode, clksel, div_m2;

	clkmode = __raw_readl(CM_CLKMODE_DPLL_MPU);
	clksel = __raw_readl(CM_CLKSEL_DPLL_MPU);
	div_m2 = __raw_readl(CM_DIV_M2_DPLL_MPU);

	/* Set the PLL to bypass Mode */
	__raw_writel(PLL_BYPASS_MODE, CM_CLKMODE_DPLL_MPU);

	while(__raw_readl(CM_IDLEST_DPLL_MPU) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((mpupll_M << 0x8) | (osc - 1));
	__raw_writel(clksel, CM_CLKSEL_DPLL_MPU);

	div_m2 = div_m2 & ~0x1f;
	div_m2 = div_m2 | MPUPLL_M2;
	__raw_writel(div_m2, CM_DIV_M2_DPLL_MPU);

	clkmode = clkmode | 0x7;
	__raw_writel(clkmode, CM_CLKMODE_DPLL_MPU);

	while(__raw_readl(CM_IDLEST_DPLL_MPU) != 0x1);
}

static void core_pll_config(int osc)
{
	u32 clkmode, clksel, div_m4, div_m5, div_m6;

	clkmode = __raw_readl(CM_CLKMODE_DPLL_CORE);
	clksel = __raw_readl(CM_CLKSEL_DPLL_CORE);
	div_m4 = __raw_readl(CM_DIV_M4_DPLL_CORE);
	div_m5 = __raw_readl(CM_DIV_M5_DPLL_CORE);
	div_m6 = __raw_readl(CM_DIV_M6_DPLL_CORE);

	/* Set the PLL to bypass Mode */
	__raw_writel(PLL_BYPASS_MODE, CM_CLKMODE_DPLL_CORE);

	while(__raw_readl(CM_IDLEST_DPLL_CORE) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((COREPLL_M << 0x8) | (osc - 1));
	__raw_writel(clksel, CM_CLKSEL_DPLL_CORE);

	div_m4 = div_m4 & ~0x1f;
	div_m4 = div_m4 | COREPLL_M4;
	__raw_writel(div_m4, CM_DIV_M4_DPLL_CORE);

	div_m5 = div_m5 & ~0x1f;
	div_m5 = div_m5 | COREPLL_M5;
	__raw_writel(div_m5, CM_DIV_M5_DPLL_CORE);

	div_m6 = div_m6 & ~0x1f;
	div_m6 = div_m6 | COREPLL_M6;
	__raw_writel(div_m6, CM_DIV_M6_DPLL_CORE);


	clkmode = clkmode | 0x7;
	__raw_writel(clkmode, CM_CLKMODE_DPLL_CORE);

	while(__raw_readl(CM_IDLEST_DPLL_CORE) != 0x1);
}

static void per_pll_config(int osc)
{
	u32 clkmode, clksel, div_m2;

	clkmode = __raw_readl(CM_CLKMODE_DPLL_PER);
	clksel = __raw_readl(CM_CLKSEL_DPLL_PER);
	div_m2 = __raw_readl(CM_DIV_M2_DPLL_PER);

	/* Set the PLL to bypass Mode */
	__raw_writel(PLL_BYPASS_MODE, CM_CLKMODE_DPLL_PER);

	while(__raw_readl(CM_IDLEST_DPLL_PER) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((PERPLL_M << 0x8) | (osc - 1));
	__raw_writel(clksel, CM_CLKSEL_DPLL_PER);

	div_m2 = div_m2 & ~0x7f;
	div_m2 = div_m2 | PERPLL_M2;
	__raw_writel(div_m2, CM_DIV_M2_DPLL_PER);

	clkmode = clkmode | 0x7;
	__raw_writel(clkmode, CM_CLKMODE_DPLL_PER);

	while(__raw_readl(CM_IDLEST_DPLL_PER) != 0x1);
}

static void ddr_pll_config(int osc, int ddrpll_M)
{
	u32 clkmode, clksel, div_m2;

	clkmode = __raw_readl(CM_CLKMODE_DPLL_DDR);
	clksel = __raw_readl(CM_CLKSEL_DPLL_DDR);
	div_m2 = __raw_readl(CM_DIV_M2_DPLL_DDR);

	/* Set the PLL to bypass Mode */
	clkmode = (clkmode & 0xfffffff8) | 0x00000004;
	__raw_writel(clkmode, CM_CLKMODE_DPLL_DDR);

	while ((__raw_readl(CM_IDLEST_DPLL_DDR) & 0x00000100) != 0x00000100);

	clksel = clksel & (~0x7ffff);
	clksel = clksel | ((ddrpll_M << 0x8) | (osc - 1));
	__raw_writel(clksel, CM_CLKSEL_DPLL_DDR);

	div_m2 = div_m2 & 0xFFFFFFE0;
	div_m2 = div_m2 | DDRPLL_M2;
	__raw_writel(div_m2, CM_DIV_M2_DPLL_DDR);

	clkmode = (clkmode & 0xfffffff8) | 0x7;
	__raw_writel(clkmode, CM_CLKMODE_DPLL_DDR);

	while ((__raw_readl(CM_IDLEST_DPLL_DDR) & 0x00000001) != 0x1);
}

void am33xx_enable_ddr_clocks(void)
{
	/* Enable the  EMIF_FW Functional clock */
	__raw_writel(PRCM_MOD_EN, CM_PER_EMIF_FW_CLKCTRL);
	/* Enable EMIF0 Clock */
	__raw_writel(PRCM_MOD_EN, CM_PER_EMIF_CLKCTRL);
	/* Poll for emif_gclk  & L3_G clock  are active */
	while ((__raw_readl(CM_PER_L3_CLKSTCTRL) & (PRCM_EMIF_CLK_ACTIVITY |
		PRCM_L3_GCLK_ACTIVITY)) != (PRCM_EMIF_CLK_ACTIVITY |
		PRCM_L3_GCLK_ACTIVITY));
	/* Poll if module is functional */
	while ((__raw_readl(CM_PER_EMIF_CLKCTRL)) != PRCM_MOD_EN);
}

/*
 * Configure the PLL/PRCM for necessary peripherals
 */
void am33xx_pll_init(int mpupll_M, int ddrpll_M)
{
	int osc;

	osc = am33xx_get_osc_clock();
	osc /= 1000;

	mpu_pll_config(mpupll_M, osc);
	core_pll_config(osc);
	per_pll_config(osc);
	ddr_pll_config(osc, ddrpll_M);

	/* Enable the required interconnect clocks */
	interface_clocks_enable();
	/* Enable power domain transition */
	power_domain_transition_enable();
	/* Enable the required peripherals */
	am33xx_enable_per_clocks();
}

/*
 * Return the OSC clock value from SYSBOOT pins in kHz.
 */
int am33xx_get_osc_clock(void)
{
	int osc;
	u32 sysboot;

	sysboot = (readl(AM33XX_CTRL_STATUS) >> 22) & 3;
	switch (sysboot) {
	case 0:
		osc = 19200;
		break;
	case 1:
		osc = 24000;
		break;
	case 2:
		osc = 25000;
		break;
	case 3:
		osc = 26000;
		break;
	}

	return osc;
}
