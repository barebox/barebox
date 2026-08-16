// SPDX-License-Identifier: GPL-2.0-only
/*
 * Marvell PXA3xxx family clocks
 *
 * Copyright (C) 2014 Robert Jarzmik
 *
 * Heavily inspired from former arch/arm/mach-pxa/pxa3xx.c
 *
 * Ported from the Linux kernel. Everything the bootloader cannot make use
 * of -- frequency changing, the cpufreq helpers and the clocks feeding
 * peripherals barebox has no driver for -- has been left out.
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <of.h>
#include <mach/pxa/hardware.h>

#include <dt-bindings/clock/pxa-clock.h>
#include "clk-pxa.h"

#define KHz 1000
#define MHz (1000 * 1000)

#define ACCR	(0x0000)	/* Application Subsystem Clock Configuration */
#define ACSR	(0x0004)	/* Application Subsystem Clock Status */
#define CKENA	(0x000C)	/* A Clock Enable Register */
#define CKENB	(0x0010)	/* B Clock Enable Register */
#define AC97_DIV (0x0014)	/* AC97 clock divisor value register */

/*
 * Static memory controller. pxa3xx-regs.h has this, but it also has
 * ACCR/ACSR/CKENA/CKENB as absolute addresses, which collide with the
 * offsets above, so take just the one register we need.
 */
#define MEMCLKCFG	0x4a000068

#define ACCR_D0CS	(1 << 26)	/* D0 Mode Clock Select */
#define ACCR_XN_MASK	(0x7 << 8)	/* Core PLL Turbo-to-Run Ratio */
#define ACCR_XL_MASK	(0x1f)		/* Core PLL Run-to-Oscillator Ratio */

/*
 * Clock Enable Bit
 */
#define CKEN_LCD	1	/* < LCD Clock Enable */
#define CKEN_USBH	2	/* < USB host clock enable */
#define CKEN_CAMERA	3	/* < Camera interface clock enable */
#define CKEN_NAND	4	/* < NAND Flash Controller Clock Enable */
#define CKEN_USB2	6	/* < USB 2.0 client clock enable. */
#define CKEN_SMC	9	/* < Static Memory Controller clock enable */
#define CKEN_MMC1	12	/* < MMC1 Clock enable */
#define CKEN_MMC2	13	/* < MMC2 clock enable */
#define CKEN_KEYPAD	14	/* < Keypand Controller Clock Enable */
#define CKEN_UDC	20	/* < UDC clock enable */
#define CKEN_BTUART	21	/* < BTUART clock enable */
#define CKEN_FFUART	22	/* < FFUART clock enable */
#define CKEN_STUART	23	/* < STUART clock enable */
#define CKEN_AC97	24	/* < AC97 clock enable */
#define CKEN_SSP1	26	/* < SSP1 clock enable */
#define CKEN_SSP2	27	/* < SSP2 clock enable */
#define CKEN_SSP3	28	/* < SSP3 clock enable */
#define CKEN_SSP4	29	/* < SSP4 clock enable */
#define CKEN_PWM0	32	/* < PWM[0] clock enable */
#define CKEN_PWM1	33	/* < PWM[1] clock enable */
#define CKEN_I2C	36	/* < I2C clock enable */
#define CKEN_GPIO	39	/* < GPIO clock enable */
#define CKEN_MMC3	5	/* < MMC3 Clock Enable */
#define CKEN_PXA300_GCU	42	/* Graphics controller clock enable */
#define CKEN_PXA320_GCU	7	/* Graphics controller clock enable */

enum {
	PXA_CORE_60Mhz = 0,
	PXA_CORE_RUN,
	PXA_CORE_TURBO,
};

enum {
	PXA_BUS_60Mhz = 0,
	PXA_BUS_HSS,
};

/* crystal frequency to HSIO bus frequency multiplier (HSS) */
static unsigned char hss_mult[4] = { 8, 12, 16, 24 };

/* crystal frequency to static memory controller multiplier (SMCFS) */
static unsigned int smcfs_mult[8] = { 6, 0, 8, 0, 0, 16, };
static const unsigned int df_clkdiv[4] = { 1, 2, 4, 1 };

static void __iomem *clk_regs;

static unsigned int pxa3xx_smemc_get_memclkdiv(void)
{
	return df_clkdiv[(readl(IOMEM(MEMCLKCFG)) >> 16) & 0x3];
}

static unsigned long clk_pxa3xx_ac97_get_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	unsigned long ac97_div, rate;

	ac97_div = readl(clk_regs + AC97_DIV);

	/*
	 * This may loose precision for some rates but won't for the
	 * standard 24.576MHz.
	 */
	rate = parent_rate / 2;
	rate /= ((ac97_div >> 12) & 0x7fff);
	rate *= (ac97_div & 0xfff);

	return rate;
}
PARENTS(clk_pxa3xx_ac97) = { "spll_624mhz" };
RATE_RO_OPS(clk_pxa3xx_ac97, "ac97");

static unsigned long clk_pxa3xx_smemc_get_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	unsigned long acsr = readl(clk_regs + ACSR);

	return (parent_rate / 48) * smcfs_mult[(acsr >> 23) & 0x7] /
		pxa3xx_smemc_get_memclkdiv();
}
PARENTS(clk_pxa3xx_smemc) = { "spll_624mhz" };
RATE_RO_OPS(clk_pxa3xx_smemc, "smemc");

static bool pxa3xx_is_ring_osc_forced(void)
{
	unsigned long acsr = readl(clk_regs + ACSR);

	return acsr & ACCR_D0CS;
}

PARENTS(pxa3xx_pbus) = { "ring_osc_60mhz", "spll_624mhz" };
PARENTS(pxa3xx_32Khz_bus) = { "osc_32_768khz", "osc_32_768khz" };
PARENTS(pxa3xx_13MHz_bus) = { "osc_13mhz", "osc_13mhz" };
PARENTS(pxa3xx_ac97_bus) = { "ring_osc_60mhz", "ac97" };
PARENTS(pxa3xx_sbus) = { "ring_osc_60mhz", "system_bus" };
PARENTS(pxa3xx_smemcbus) = { "ring_osc_60mhz", "smemc" };

#define CKEN_AB(bit) ((CKEN_ ## bit > 31) ? CKENB : CKENA)
#define PXA3XX_CKEN(dev_id, con_id, parents, mult_lp, div_lp, mult_hp,	\
		    div_hp, bit, is_lp, flags)				\
	PXA_CKEN(dev_id, con_id, bit, parents, mult_lp, div_lp,		\
		 mult_hp, div_hp, is_lp,  CKEN_AB(bit),			\
		 (CKEN_ ## bit % 32), flags)
#define PXA3XX_PBUS_CKEN(dev_id, con_id, bit, mult_lp, div_lp,		\
			 mult_hp, div_hp, delay)			\
	PXA3XX_CKEN(dev_id, con_id, pxa3xx_pbus_parents, mult_lp,	\
		    div_lp, mult_hp, div_hp, bit, pxa3xx_is_ring_osc_forced, 0)
#define PXA3XX_CKEN_1RATE(dev_id, con_id, bit, parents)			\
	PXA_CKEN_1RATE(dev_id, con_id, bit, parents,			\
		       CKEN_AB(bit), (CKEN_ ## bit % 32), 0)

static struct desc_clk_cken pxa3xx_clocks[] = {
	PXA3XX_PBUS_CKEN("pxa2xx-uart.0", NULL, FFUART, 1, 4, 1, 42, 1),
	PXA3XX_PBUS_CKEN("pxa2xx-uart.1", NULL, BTUART, 1, 4, 1, 42, 1),
	PXA3XX_PBUS_CKEN("pxa2xx-uart.2", NULL, STUART, 1, 4, 1, 42, 1),
	PXA3XX_PBUS_CKEN("pxa2xx-i2c.0", NULL, I2C, 2, 5, 1, 19, 0),
	PXA3XX_PBUS_CKEN("pxa27x-udc", NULL, UDC, 1, 4, 1, 13, 5),
	PXA3XX_PBUS_CKEN("pxa27x-ohci", NULL, USBH, 1, 4, 1, 13, 0),
	PXA3XX_PBUS_CKEN("pxa3xx-u2d", NULL, USB2, 1, 4, 1, 13, 0),
	PXA3XX_PBUS_CKEN("pxa27x-pwm.0", NULL, PWM0, 1, 6, 1, 48, 0),
	PXA3XX_PBUS_CKEN("pxa27x-pwm.1", NULL, PWM1, 1, 6, 1, 48, 0),
	PXA3XX_PBUS_CKEN("pxa2xx-mci.0", NULL, MMC1, 1, 4, 1, 24, 0),
	PXA3XX_PBUS_CKEN("pxa2xx-mci.1", NULL, MMC2, 1, 4, 1, 24, 0),
	PXA3XX_PBUS_CKEN("pxa2xx-mci.2", NULL, MMC3, 1, 4, 1, 24, 0),

	PXA3XX_CKEN_1RATE("pxa27x-keypad", NULL, KEYPAD,
			  pxa3xx_32Khz_bus_parents),
	PXA3XX_CKEN_1RATE("pxa3xx-ssp.0", NULL, SSP1, pxa3xx_13MHz_bus_parents),
	PXA3XX_CKEN_1RATE("pxa3xx-ssp.1", NULL, SSP2, pxa3xx_13MHz_bus_parents),
	PXA3XX_CKEN_1RATE("pxa3xx-ssp.2", NULL, SSP3, pxa3xx_13MHz_bus_parents),
	PXA3XX_CKEN_1RATE("pxa3xx-ssp.3", NULL, SSP4, pxa3xx_13MHz_bus_parents),

	PXA3XX_CKEN(NULL, "AC97CLK", pxa3xx_ac97_bus_parents, 1, 4, 1, 1, AC97,
		    pxa3xx_is_ring_osc_forced, 0),
	PXA3XX_CKEN(NULL, "CAMCLK", pxa3xx_sbus_parents, 1, 2, 1, 1, CAMERA,
		    pxa3xx_is_ring_osc_forced, 0),
	PXA3XX_CKEN("pxa2xx-fb", NULL, pxa3xx_sbus_parents, 1, 1, 1, 1, LCD,
		    pxa3xx_is_ring_osc_forced, 0),
	PXA3XX_CKEN("pxa2xx-pcmcia", NULL, pxa3xx_smemcbus_parents, 1, 4,
		    1, 1, SMC, pxa3xx_is_ring_osc_forced, CLK_IGNORE_UNUSED),
};

static struct desc_clk_cken pxa300_310_clocks[] = {
	PXA3XX_PBUS_CKEN("pxa3xx-gcu", NULL, PXA300_GCU, 1, 1, 1, 1, 0),
	PXA3XX_PBUS_CKEN("pxa3xx-nand", NULL, NAND, 1, 2, 1, 4, 0),
	PXA3XX_CKEN_1RATE("pxa3xx-gpio", NULL, GPIO, pxa3xx_13MHz_bus_parents),
};

static struct desc_clk_cken pxa320_clocks[] = {
	PXA3XX_PBUS_CKEN("pxa3xx-nand", NULL, NAND, 1, 2, 1, 6, 0),
	PXA3XX_PBUS_CKEN("pxa3xx-gcu", NULL, PXA320_GCU, 1, 1, 1, 1, 0),
	PXA3XX_CKEN_1RATE("pxa3xx-gpio", NULL, GPIO, pxa3xx_13MHz_bus_parents),
};

static unsigned long clk_pxa3xx_system_bus_get_rate(struct clk_hw *hw,
						    unsigned long parent_rate)
{
	unsigned long acsr = readl(clk_regs + ACSR);
	unsigned int hss = (acsr >> 14) & 0x3;

	if (pxa3xx_is_ring_osc_forced())
		return parent_rate;
	return parent_rate / 48 * hss_mult[hss];
}

static int clk_pxa3xx_system_bus_get_parent(struct clk_hw *hw)
{
	if (pxa3xx_is_ring_osc_forced())
		return PXA_BUS_60Mhz;
	else
		return PXA_BUS_HSS;
}

PARENTS(clk_pxa3xx_system_bus) = { "ring_osc_60mhz", "spll_624mhz" };
MUX_RO_RATE_RO_OPS(clk_pxa3xx_system_bus, "system_bus");

static unsigned long clk_pxa3xx_core_get_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	return parent_rate;
}

static int clk_pxa3xx_core_get_parent(struct clk_hw *hw)
{
	unsigned long xclkcfg;
	unsigned int t;

	if (pxa3xx_is_ring_osc_forced())
		return PXA_CORE_60Mhz;

	/* Read XCLKCFG register turbo bit */
	__asm__ __volatile__("mrc\tp14, 0, %0, c6, c0, 0" : "=r"(xclkcfg));
	t = xclkcfg & 0x1;

	if (t)
		return PXA_CORE_TURBO;
	return PXA_CORE_RUN;
}
PARENTS(clk_pxa3xx_core) = { "ring_osc_60mhz", "run", "cpll" };
MUX_RO_RATE_RO_OPS(clk_pxa3xx_core, "core");

static unsigned long clk_pxa3xx_run_get_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	unsigned long acsr = readl(clk_regs + ACSR);
	unsigned int xn = (acsr & ACCR_XN_MASK) >> 8;
	unsigned int t, xclkcfg;

	/* Read XCLKCFG register turbo bit */
	__asm__ __volatile__("mrc\tp14, 0, %0, c6, c0, 0" : "=r"(xclkcfg));
	t = xclkcfg & 0x1;

	return t ? (parent_rate / xn) * 2 : parent_rate;
}
PARENTS(clk_pxa3xx_run) = { "cpll" };
RATE_RO_OPS(clk_pxa3xx_run, "run");

static unsigned long clk_pxa3xx_cpll_get_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	unsigned long acsr = readl(clk_regs + ACSR);
	unsigned int xn = (acsr & ACCR_XN_MASK) >> 8;
	unsigned int xl = acsr & ACCR_XL_MASK;
	unsigned int t, xclkcfg;

	/* Read XCLKCFG register turbo bit */
	__asm__ __volatile__("mrc\tp14, 0, %0, c6, c0, 0" : "=r"(xclkcfg));
	t = xclkcfg & 0x1;

	return t ? parent_rate * xl * xn : parent_rate * xl;
}
PARENTS(clk_pxa3xx_cpll) = { "osc_13mhz" };
RATE_RO_OPS(clk_pxa3xx_cpll, "cpll");

static void pxa3xx_register_core(void)
{
	clk_register_clk_pxa3xx_cpll();
	clk_register_clk_pxa3xx_run();

	clkdev_pxa_register(CLK_CORE, "core", NULL,
			    clk_register_clk_pxa3xx_core());
}

static void pxa3xx_register_plls(void)
{
	clk_register_fixed_rate("osc_13mhz", NULL, CLK_GET_RATE_NOCACHE,
				13 * MHz);
	clkdev_pxa_register(CLK_OSC32k768, "osc_32_768khz", NULL,
			    clk_register_fixed_rate("osc_32_768khz", NULL,
						    CLK_GET_RATE_NOCACHE,
						    32768));
	clk_register_fixed_rate("ring_osc_120mhz", NULL, CLK_GET_RATE_NOCACHE,
				120 * MHz);
	clk_register_fixed_rate("clk_dummy", NULL, 0, 0);
	clk_register_fixed_factor(NULL, "spll_624mhz", "osc_13mhz", 0, 48, 1);
	clk_register_fixed_factor(NULL, "ring_osc_60mhz", "ring_osc_120mhz",
				  0, 1, 2);
}

static void pxa3xx_base_clocks_init(void __iomem *oscc_reg)
{
	struct clk *clk;

	pxa3xx_register_plls();
	pxa3xx_register_core();
	clk_register_clk_pxa3xx_system_bus();
	clk_register_clk_pxa3xx_ac97();
	clk_register_clk_pxa3xx_smemc();

	clk = clk_register_gate(NULL, "CLK_POUT", "osc_13mhz", 0, oscc_reg,
				11, 0, NULL);
	clk_register_clkdev(clk, "CLK_POUT", NULL);
	clkdev_pxa_register(CLK_OSTIMER, "OSTIMER0", NULL,
			    clk_register_fixed_factor(NULL, "os-timer0",
						      "osc_13mhz", 0, 1, 4));
}

static int pxa3xx_clocks_init(void __iomem *regs, void __iomem *oscc_reg)
{
	int ret;

	clk_regs = regs;
	pxa3xx_base_clocks_init(oscc_reg);

	ret = clk_pxa_cken_init(pxa3xx_clocks, ARRAY_SIZE(pxa3xx_clocks), regs);
	if (ret)
		return ret;

	if (cpu_is_pxa320())
		return clk_pxa_cken_init(pxa320_clocks,
					 ARRAY_SIZE(pxa320_clocks), regs);

	return clk_pxa_cken_init(pxa300_310_clocks,
				 ARRAY_SIZE(pxa300_310_clocks), regs);
}

static int pxa3xx_dt_clocks_init(struct device_node *np)
{
	int ret;

	/*
	 * The clock unit has no reg property of its own in the binding, the
	 * addresses are implied by the compatible.
	 */
	ret = pxa3xx_clocks_init(IOMEM(0x41340000), IOMEM(0x41350000));
	if (ret)
		return ret;

	return clk_pxa_dt_common_init(np);
}
CLK_OF_DECLARE(pxa_clks, "marvell,pxa300-clocks", pxa3xx_dt_clocks_init);
