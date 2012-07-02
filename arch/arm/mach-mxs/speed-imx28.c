/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix <kernel@pengutronix.de>
 *
 * This code is based partially on code that has:
 *
 * (c) 2008 Embedded Alley Solutions, Inc.
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <init.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <mach/generic.h>
#include <mach/clock.h>

#define HW_CLKCTRL_PLL0CTRL0 0x000
#define HW_CLKCTRL_PLL0CTRL1 0x010
#define HW_CLKCTRL_PLL1CTRL0 0x020
#define HW_CLKCTRL_PLL1CTRL1 0x030
#define HW_CLKCTRL_PLL2CTRL0 0x040
# define CLKCTRL_PLL2CTRL0_CLKGATE (1 << 31)
# define CLKCTRL_PLL2CTRL0_POWER (1 << 23)
#define HW_CLKCTRL_CPU 0x50
# define GET_CPU_XTAL_DIV(x) (((x) >> 16) & 0x3ff)
# define GET_CPU_PLL_DIV(x) ((x) & 0x3f)
#define HW_CLKCTRL_HBUS 0x60
#define HW_CLKCTRL_XBUS 0x70
#define HW_CLKCTRL_XTAL 0x080
#define HW_CLKCTRL_SSP0 0x090
#define HW_CLKCTRL_SSP1 0x0a0
#define HW_CLKCTRL_SSP2 0x0b0
#define HW_CLKCTRL_SSP3 0x0c0
/* note: no set/clear register! */
# define CLKCTRL_SSP_CLKGATE (1 << 31)
# define CLKCTRL_SSP_BUSY (1 << 29)
# define CLKCTRL_SSP_DIV_FRAC_EN (1 << 9)
# define CLKCTRL_SSP_DIV_MASK 0x1ff
# define GET_SSP_DIV(x) ((x) & CLKCTRL_SSP_DIV_MASK)
# define SET_SSP_DIV(x) ((x) & CLKCTRL_SSP_DIV_MASK)
#define HW_CLKCTRL_GPMI 0x0d0
# define CLKCTRL_GPMI_CLKGATE (1 << 31)
# define CLKCTRL_GPMI_DIV_MASK 0x3ff
/* note: no set/clear register! */
#define HW_CLKCTRL_SPDIF 0x0e0
/* note: no set/clear register! */
#define HW_CLKCTRL_EMI	0xf0
/* note: no set/clear register! */
# define CLKCTRL_EMI_CLKGATE (1 << 31)
# define GET_EMI_XTAL_DIV(x) (((x) >> 8) & 0xf)
# define GET_EMI_PLL_DIV(x) ((x) & 0x3f)
#define HW_CLKCTRL_SAIF0 0x100
#define HW_CLKCTRL_SAIF1 0x110
#define HW_CLKCTRL_DIS_LCDIF 0x120
# define CLKCTRL_DIS_LCDIF_GATE (1 << 31)
# define CLKCTRL_DIS_LCDIF_BUSY (1 << 29)
# define SET_DIS_LCDIF_DIV(x) ((x) & 0x1fff)
# define GET_DIS_LCDIF_DIV(x) ((x) & 0x1fff)
#define HW_CLKCTRL_ETM 0x130
#define HW_CLKCTRL_ENET 0x140
# define SET_CLKCTRL_ENET_DIV(x) (((x) & 0x3f) << 21)
# define SET_CLKCTRL_ENET_SEL(x) (((x) & 0x3) << 19)
# define CLKCTRL_ENET_CLK_OUT_EN (1 << 18)
#define HW_CLKCTRL_HSADC 0x150
#define HW_CLKCTRL_FLEXCAN 0x160
#define HW_CLKCTRL_FRAC0 0x1b0
# define CLKCTRL_FRAC_CLKGATEIO0 (1 << 31)
# define GET_IO0FRAC(x) (((x) >> 24) & 0x3f)
# define SET_IO0FRAC(x) (((x) & 0x3f) << 24)
# define CLKCTRL_FRAC_CLKGATEIO1 (1 << 23)
# define GET_IO1FRAC(x) (((x) >> 16) & 0x3f)
# define SET_IO1FRAC(x) (((x) & 0x3f) << 16)
# define CLKCTRL_FRAC_CLKGATEEMI (1 << 15)
# define GET_EMIFRAC(x) (((x) >> 8) & 0x3f)
# define CLKCTRL_FRAC_CLKGATECPU (1 << 7)
# define GET_CPUFRAC(x) ((x) & 0x3f)
#define HW_CLKCTRL_FRAC1 0x1c0
# define CLKCTRL_FRAC_CLKGATEGPMI (1 << 23)
# define GET_GPMIFRAC(x) (((x) >> 16) & 0x3f)
# define CLKCTRL_FRAC_CLKGATEHSADC (1 << 15)
# define GET_HSADCFRAC(x) (((x) >> 8) & 0x3f)
# define CLKCTRL_FRAC_CLKGATEPIX (1 << 7)
# define GET_PIXFRAC(x) ((x) & 0x3f)
# define SET_PIXFRAC(x) ((x) & 0x3f)
#define HW_CLKCTRL_CLKSEQ 0x1d0
# define CLKCTRL_CLKSEQ_BYPASS_CPU (1 << 18)
# define CLKCTRL_CLKSEQ_BYPASS_DIS_LCDIF (1 << 14)
# define CLKCTRL_CLKSEQ_BYPASS_ETM (1 << 8)
# define CLKCTRL_CLKSEQ_BYPASS_EMI (1 << 7)
# define CLKCTRL_CLKSEQ_BYPASS_SSP3 (1 << 6)
# define CLKCTRL_CLKSEQ_BYPASS_SSP2 (1 << 5)
# define CLKCTRL_CLKSEQ_BYPASS_SSP1 (1 << 4)
# define CLKCTRL_CLKSEQ_BYPASS_SSP0 (1 << 3)
# define CLKCTRL_CLKSEQ_BYPASS_GPMI (1 << 2)
# define CLKCTRL_CLKSEQ_BYPASS_SAIF1 (1 << 1)
# define CLKCTRL_CLKSEQ_BYPASS_SAIF0 (1 << 0)
#define HW_CLKCTRL_RESET 0x1e0
#define HW_CLKCTRL_STATUS 0x1f0
#define HW_CLKCTRL_VERSION 0x200

unsigned imx_get_mpllclk(void)
{
	/* the main PLL runs at 480 MHz */
	return 480000000;
}

unsigned imx_get_xtalclk(void)
{
	/* the external reference runs at 24 MHz */
	return 24000000;
}

unsigned imx_get_fecclk(void)
{
	/* this PLL always runs at 50 MHz */
	return 50000000;
}


/* used for the SDRAM controller */
unsigned imx_get_emiclk(void)
{
	uint32_t reg;
	unsigned rate;

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_EMI) & CLKCTRL_EMI_CLKGATE)
		return 0;	/* clock is off */

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ) & CLKCTRL_CLKSEQ_BYPASS_EMI)
		return imx_get_xtalclk() /
			GET_EMI_XTAL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_EMI));

	rate = imx_get_mpllclk() / 1000;
	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC0);
	if (!(reg & CLKCTRL_FRAC_CLKGATEEMI)) {
		rate *= 18;
		rate /= GET_EMIFRAC(reg);
	}

	return (rate / GET_EMI_PLL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_EMI)))
				* 1000;
}

/*
 * Source of ssp, gpmi, ir
 * @param index 0 or 1 for ioclk0 or ioclock1
 */
unsigned imx_get_ioclk(unsigned index)
{
	uint32_t reg;
	unsigned rate = imx_get_mpllclk() / 1000;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC0);
	switch (index) {
	case 0:
		if (reg & CLKCTRL_FRAC_CLKGATEIO0)
			return 0;	/* clock is off */

		rate *= 18;
		rate /= GET_IO0FRAC(reg);
		break;
	case 1:
		if (reg & CLKCTRL_FRAC_CLKGATEIO1)
			return 0;	/* clock is off */

		rate *= 18;
		rate /= GET_IO1FRAC(reg);
		break;
	}

	return rate * 1000;
}

/**
 * Setup a new frequency to the IOCLK domain.
 * @param index 0 or 1 for ioclk0 or ioclock1
 * @param nc New frequency in [Hz]
 *
 * The FRAC divider for the IOCLK must be between 18 (* 18/18) and 35 (* 18/35)
 *
 * ioclock0 is the shared clock source of SSP0/SSP1, ioclock1 the shared clock
 * source of SSP2/SSP3
 */
unsigned imx_set_ioclk(unsigned index, unsigned nc)
{
	uint32_t reg;
	unsigned div;

	nc /= 1000;
	div = (imx_get_mpllclk() / 1000) * 18;
	div = DIV_ROUND_CLOSEST(div, nc);
	if (div > 0x3f)
		div = 0x3f;

	switch (index) {
	case 0:
		reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC0) &
				~(SET_IO0FRAC(0x3f));
		/* mask the current settings */
		writel(reg | SET_IO0FRAC(div), IMX_CCM_BASE + HW_CLKCTRL_FRAC0);
		/* enable the IO clock at its new frequency */
		writel(CLKCTRL_FRAC_CLKGATEIO0,
				IMX_CCM_BASE + HW_CLKCTRL_FRAC0 + BIT_CLR);
		break;
	case 1:
		reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC0) &
				~(SET_IO1FRAC(0x3f));
		/* mask the current settings */
		writel(reg | SET_IO1FRAC(div), IMX_CCM_BASE + HW_CLKCTRL_FRAC0);
		/* enable the IO clock at its new frequency */
		writel(CLKCTRL_FRAC_CLKGATEIO1,
				IMX_CCM_BASE + HW_CLKCTRL_FRAC0 + BIT_CLR);
		break;
	}

	return imx_get_ioclk(index);
}

/* this is CPU core clock */
unsigned imx_get_armclk(void)
{
	uint32_t reg;
	unsigned rate;

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ) & CLKCTRL_CLKSEQ_BYPASS_CPU)
		return imx_get_xtalclk() /
			GET_CPU_XTAL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_CPU));

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC0);
	if (reg & CLKCTRL_FRAC_CLKGATECPU)
		return 0;	/* should not possible, shouldn't it? */

	rate = (imx_get_mpllclk() / 1000) * 18;
	rate /= GET_CPUFRAC(reg);

	return (rate / GET_CPU_PLL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_CPU)))
			* 1000;
}

/* this is the AHB and APBH bus clock */
unsigned imx_get_hclk(void)
{
	unsigned rate = imx_get_armclk() / 1000;

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_HBUS) & 0x20) {
		rate *= readl(IMX_CCM_BASE + HW_CLKCTRL_HBUS) & 0x1f;
		rate = DIV_ROUND_UP(rate, 32);
	} else
		rate = DIV_ROUND_UP(rate,
			readl(IMX_CCM_BASE + HW_CLKCTRL_HBUS) & 0x1f);
	return rate * 1000;
}

unsigned imx_set_hclk(unsigned nc)
{
	unsigned root_rate = imx_get_armclk();
	unsigned reg, div;

	div = DIV_ROUND_UP(root_rate, nc);
	if ((div == 0) || (div >= 32))
		return 0;

	if ((root_rate < nc) && (root_rate == 64000000))
		div = 3;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_HBUS) & ~0x3f;
	writel(reg | div, IMX_CCM_BASE + HW_CLKCTRL_HBUS);

	while (readl(IMX_CCM_BASE + HW_CLKCTRL_HBUS) & (1 << 31))
		;

	return imx_get_hclk();
}

/*
 * Source of UART, debug UART, audio, PWM, dri, timer, digctl
 */
unsigned imx_get_xclk(void)
{
	/* runs from the 24 MHz crystal reference */
	unsigned rate = imx_get_xtalclk();

	return rate / (readl(IMX_CCM_BASE + HW_CLKCTRL_XBUS) & 0x3ff);
}

/**
 * @param index The SSP unit (0...3)
 */
unsigned imx_get_sspclk(unsigned index)
{
	unsigned rate, offset, shift, ioclk_index;

	if (index > 3) {
		pr_debug("Unknown SSP unit: %u\n", index);
		return 0;
	}

	ioclk_index = index >> 1;

	offset = HW_CLKCTRL_SSP0 + (0x10 * index);
	shift = CLKCTRL_CLKSEQ_BYPASS_SSP0 << index;

	if (readl(IMX_CCM_BASE + offset) & CLKCTRL_SSP_CLKGATE)
		return 0;	/* clock is off */

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ) & shift)
		rate = imx_get_xtalclk();
	else
		rate = imx_get_ioclk(ioclk_index);

	return rate / GET_SSP_DIV(readl(IMX_CCM_BASE + offset));
}

/**
 * @param index The SSP unit (0...3)
 * @param nc New frequency in [Hz]
 * @param high != 0 if ioclk should be the source
 * @return The new possible frequency
 */
unsigned imx_set_sspclk(unsigned index, unsigned nc, int high)
{
	uint32_t reg;
	unsigned ssp_div, offset, shift, ioclk_index;

	if (index > 3) {
		pr_debug("Unknown SSP unit: %u\n", index);
		return 0;
	}

	ioclk_index = index >> 1;

	offset = HW_CLKCTRL_SSP0 + (0x10 * index);
	shift = CLKCTRL_CLKSEQ_BYPASS_SSP0 << index;

	reg = readl(IMX_CCM_BASE + offset) & ~CLKCTRL_SSP_CLKGATE;
	/* Datasheet says: Do not change the DIV setting if the clock is off */
	writel(reg, IMX_CCM_BASE + offset);
	/* Wait while clock is gated */
	while (readl(IMX_CCM_BASE + offset) & CLKCTRL_SSP_CLKGATE)
		;

	if (high)
		ssp_div = imx_get_ioclk(ioclk_index);
	else
		ssp_div = imx_get_xtalclk();

	if (nc > ssp_div) {
		printf("Cannot setup SSP unit clock to %u kHz, base clock is "
						"only %u kHz\n", nc, ssp_div);
		ssp_div = 1;
	} else {
		ssp_div = DIV_ROUND_UP(ssp_div, nc);
		if (ssp_div > CLKCTRL_SSP_DIV_MASK)
			ssp_div = CLKCTRL_SSP_DIV_MASK;
	}

	/* Set new divider value */
	reg = readl(IMX_CCM_BASE + offset) & ~CLKCTRL_SSP_DIV_MASK;
	writel(reg | SET_SSP_DIV(ssp_div), IMX_CCM_BASE + offset);

	/* Wait until new divider value is set */
	while (readl(IMX_CCM_BASE + offset) & CLKCTRL_SSP_BUSY)
		;

	if (high)
		/* switch to ioclock */
		writel(shift, IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ + BIT_CLR);
	else
		/* switch to 24 MHz crystal */
		writel(shift, IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ + BIT_SET);

	return imx_get_sspclk(index);
}

void imx_enable_enetclk(void)
{
	uint32_t reg;

	/* wake up main enet PLL */
	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_PLL2CTRL0);
	if (!(reg & CLKCTRL_PLL2CTRL0_POWER)) {
		reg |= CLKCTRL_PLL2CTRL0_POWER;
		writel(reg, IMX_CCM_BASE + HW_CLKCTRL_PLL2CTRL0);
		udelay(50);	/* wait until this PLL locks */
	}
	reg &= ~CLKCTRL_PLL2CTRL0_CLKGATE;
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_PLL2CTRL0);

	writel(SET_CLKCTRL_ENET_DIV(1) | SET_CLKCTRL_ENET_SEL(0) |
		CLKCTRL_ENET_CLK_OUT_EN, /* FIXME may be platform specific */
		IMX_CCM_BASE + HW_CLKCTRL_ENET);
}

void imx_enable_nandclk(void)
{
	uint32_t reg;

	/* Clear bypass bit; refman says clear, but fsl-code does set. Hooray! */
	writel(CLKCTRL_CLKSEQ_BYPASS_GPMI,
		IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ + BIT_SET);

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_GPMI) & ~CLKCTRL_GPMI_CLKGATE;
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_GPMI);
	udelay(1000);
	/* Initialize DIV to 1 */
	reg &= ~CLKCTRL_GPMI_DIV_MASK;
	reg |= 1;
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_GPMI);
}

void imx_dump_clocks(void)
{
	printf("mpll:    %10u kHz\n", imx_get_mpllclk() / 1000);
	printf("arm:     %10u kHz\n", imx_get_armclk() / 1000);
	printf("ioclk0:  %10u kHz\n", imx_get_ioclk(0) / 1000);
	printf("ioclk1:  %10u kHz\n", imx_get_ioclk(1) / 1000);
	printf("emiclk:  %10u kHz\n", imx_get_emiclk() / 1000);
	printf("hclk:    %10u kHz\n", imx_get_hclk() / 1000);
	printf("xclk:    %10u kHz\n", imx_get_xclk() / 1000);
	printf("ssp0:    %10u kHz\n", imx_get_sspclk(0) / 1000);
	printf("ssp1:    %10u kHz\n", imx_get_sspclk(1) / 1000);
	printf("ssp2:    %10u kHz\n", imx_get_sspclk(2) / 1000);
	printf("ssp3:    %10u kHz\n", imx_get_sspclk(3) / 1000);
}
