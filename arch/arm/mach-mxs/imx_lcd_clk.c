/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix <kernel@pengutronix.de>
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
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <asm/io.h>

#ifdef CONFIG_ARCH_IMX23

# define HW_CLKCTRL_DIS_LCDIF 0x060
#  define CLKCTRL_DIS_LCDIF_GATE (1 << 31)
#  define CLKCTRL_DIS_LCDIF_BUSY (1 << 29)
#  define MASK_DIS_LCDIF_DIV 0xfff
#  define SET_DIS_LCDIF_DIV(x) ((x) & MASK_DIS_LCDIF_DIV)
#  define GET_DIS_LCDIF_DIV(x) ((x) & MASK_DIS_LCDIF_DIV)

# define HW_CLKCTRL_FRAC 0xf0
#  define MASK_PIXFRAC 0x3f
#  define GET_PIXFRAC(x) (((x) >> 16) & MASK_PIXFRAC)
#  define SET_PIXFRAC(x) (((x) & MASK_PIXFRAC) << 16)
#  define CLKCTRL_FRAC_CLKGATEPIX (1 << 23)

# define HW_CLKCTRL_CLKSEQ 0x110
#  define CLKCTRL_CLKSEQ_BYPASS_DIS_LCDIF (1 << 1)

#endif

#ifdef CONFIG_ARCH_IMX28

# define HW_CLKCTRL_DIS_LCDIF 0x120
#  define CLKCTRL_DIS_LCDIF_GATE (1 << 31)
#  define CLKCTRL_DIS_LCDIF_BUSY (1 << 29)
#  define MASK_DIS_LCDIF_DIV 0x1fff
#  define SET_DIS_LCDIF_DIV(x) ((x) & MASK_DIS_LCDIF_DIV)
#  define GET_DIS_LCDIF_DIV(x) ((x) & MASK_DIS_LCDIF_DIV)

/* note: On i.MX28 this is called 'FRAC1' */
# define HW_CLKCTRL_FRAC 0x1c0
#  define MASK_PIXFRAC 0x3f
#  define GET_PIXFRAC(x) ((x) & MASK_PIXFRAC)
#  define SET_PIXFRAC(x) ((x) & MASK_PIXFRAC)
#  define CLKCTRL_FRAC_CLKGATEPIX (1 << 7)

# define HW_CLKCTRL_CLKSEQ 0x1d0
#  define CLKCTRL_CLKSEQ_BYPASS_DIS_LCDIF (1 << 14)

#endif

unsigned imx_get_lcdifclk(void)
{
	unsigned rate = (imx_get_mpllclk() / 1000) * 18U;
	unsigned div;

	div = GET_PIXFRAC(readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC));
	if (div != 0U) {
		rate /= div;
		div = GET_DIS_LCDIF_DIV(readl(IMX_CCM_BASE +
							HW_CLKCTRL_DIS_LCDIF));
		if (div != 0U)
			rate /= div;
		else
			pr_debug("LCDIF clock has divisor 0!\n");
	} else
		pr_debug("LCDIF clock has frac divisor 0!\n");

	return rate * 1000;
}

/*
 * The source of the pixel clock can be the external 24 MHz crystal or the
 * internal PLL running at 480 MHz. In order to support at least VGA sized
 * displays/resolutions this routine forces the PLL as the clock source.
 */
unsigned imx_set_lcdifclk(unsigned nc)
{
	unsigned frac, best_frac = 0, div, best_div = 0, result;
	int delta, best_delta = 0xffffff;
	unsigned i, parent_rate = imx_get_mpllclk() / 1000;
	uint32_t reg;

#define SH_DIV(NOM, DEN, LSH) ((((NOM) / (DEN)) << (LSH)) + \
			DIV_ROUND_CLOSEST(((NOM) % (DEN)) << (LSH), DEN))
#define SHIFT 4

	nc /= 1000;
	nc <<= SHIFT;

	for (frac = 18; frac <= 35; ++frac) {
		for (div = 1; div <= 255; ++div) {
			result = DIV_ROUND_CLOSEST(parent_rate *
						SH_DIV(18U, frac, SHIFT), div);
			delta = nc - result;
			if (abs(delta) < abs(best_delta)) {
				best_delta = delta;
				best_frac = frac;
				best_div = div;
			}
		}
	}

	if (best_delta == 0xffffff) {
		pr_debug("Unable to match the pixelclock\n");
		return 0;
	}

	pr_debug("Programming PFD=%u,DIV=%u ref_pix=%u MHz PIXCLK=%u kHz\n",
			best_frac, best_div, 480 * 18 / best_frac,
			480000 * 18 / best_frac / best_div);

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC);
	reg &= ~SET_PIXFRAC(MASK_PIXFRAC);
	reg |= SET_PIXFRAC(best_frac);
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_FRAC);
	writel(reg & ~CLKCTRL_FRAC_CLKGATEPIX, IMX_CCM_BASE + HW_CLKCTRL_FRAC);

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_DIS_LCDIF) & ~MASK_DIS_LCDIF_DIV;
	reg &= ~CLKCTRL_DIS_LCDIF_GATE;
	reg |= SET_DIS_LCDIF_DIV(best_div);
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_DIS_LCDIF);

	/* Wait for divider update */
	for (i = 0; i < 10000; i++) {
		if (!(readl(IMX_CCM_BASE + HW_CLKCTRL_DIS_LCDIF) &
						CLKCTRL_DIS_LCDIF_BUSY))
			break;
	}

	if (i >= 10000) {
		pr_debug("Setting LCD clock failed\n");
		return 0;
	}

	writel(CLKCTRL_CLKSEQ_BYPASS_DIS_LCDIF,
		IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ + BIT_CLR);

	return imx_get_lcdifclk();
}
