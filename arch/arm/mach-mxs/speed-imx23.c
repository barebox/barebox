/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 *
 * This code is based partially on code of:
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <mach/generic.h>
#include <mach/clock.h>

#define HW_CLKCTRL_PLLCTRL0 0x000
#define HW_CLKCTRL_PLLCTRL1 0x010
#define HW_CLKCTRL_CPU 0x20
# define GET_CPU_XTAL_DIV(x) (((x) >> 16) & 0x3ff)
# define GET_CPU_PLL_DIV(x) ((x) & 0x3f)
#define HW_CLKCTRL_HBUS 0x30
#define HW_CLKCTRL_XBUS 0x40
#define HW_CLKCTRL_XTAL 0x050
#define HW_CLKCTRL_PIX 0x060
/* note: no set/clear register! */
#define HW_CLKCTRL_SSP 0x070
/* note: no set/clear register! */
# define CLKCTRL_SSP_CLKGATE (1 << 31)
# define CLKCTRL_SSP_BUSY (1 << 29)
# define CLKCTRL_SSP_DIV_MASK 0x1ff
# define GET_SSP_DIV(x) ((x) & CLKCTRL_SSP_DIV_MASK)
# define SET_SSP_DIV(x) ((x) & CLKCTRL_SSP_DIV_MASK)
#define HW_CLKCTRL_GPMI 0x080
# define CLKCTRL_GPMI_CLKGATE (1 << 31)
# define CLKCTRL_GPMI_DIV_MASK 0x3ff
/* note: no set/clear register! */
#define HW_CLKCTRL_SPDIF 0x090
/* note: no set/clear register! */
#define HW_CLKCTRL_EMI	0xa0
/* note: no set/clear register! */
# define CLKCTRL_EMI_CLKGATE (1 << 31)
# define GET_EMI_XTAL_DIV(x) (((x) >> 8) & 0xf)
# define GET_EMI_PLL_DIV(x) ((x) & 0x3f)
#define HW_CLKCTRL_SAIF 0x0c0
#define HW_CLKCTRL_TV 0x0d0
#define HW_CLKCTRL_ETM 0x0e0
#define HW_CLKCTRL_FRAC 0xf0
# define CLKCTRL_FRAC_CLKGATEIO (1 << 31)
# define GET_IOFRAC(x) (((x) >> 24) & 0x3f)
# define SET_IOFRAC(x) (((x) & 0x3f) << 24)
# define CLKCTRL_FRAC_CLKGATEPIX (1 << 23)
# define GET_PIXFRAC(x) (((x) >> 16) & 0x3f)
# define CLKCTRL_FRAC_CLKGATEEMI (1 << 15)
# define GET_EMIFRAC(x) (((x) >> 8) & 0x3f)
# define CLKCTRL_FRAC_CLKGATECPU (1 << 7)
# define GET_CPUFRAC(x) ((x) & 0x3f)
#define HW_CLKCTRL_FRAC1 0x100
#define HW_CLKCTRL_CLKSEQ 0x110
# define CLKCTRL_CLKSEQ_BYPASS_ETM (1 << 8)
# define CLKCTRL_CLKSEQ_BYPASS_CPU (1 << 7)
# define CLKCTRL_CLKSEQ_BYPASS_EMI (1 << 6)
# define CLKCTRL_CLKSEQ_BYPASS_SSP (1 << 5)
# define CLKCTRL_CLKSEQ_BYPASS_GPMI (1 << 4)
#define HW_CLKCTRL_RESET 0x120
#define HW_CLKCTRL_STATUS 0x130
#define HW_CLKCTRL_VERSION 0x140

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

/* used for the SDRAM controller */
unsigned imx_get_emiclk(void)
{
	uint32_t reg;
	unsigned rate;

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_EMI) & CLKCTRL_EMI_CLKGATE)
		return 0U;	/* clock is off */

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ) & CLKCTRL_CLKSEQ_BYPASS_EMI)
		return imx_get_xtalclk() / GET_EMI_XTAL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_EMI));

	rate = imx_get_mpllclk() / 1000;
	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC);
	if (!(reg & CLKCTRL_FRAC_CLKGATEEMI)) {
		rate *= 18U;
		rate /= GET_EMIFRAC(reg);
	}

	return (rate / GET_EMI_PLL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_EMI)))
			* 1000;
}

/*
 * Source of ssp, gpmi, ir
 */
unsigned imx_get_ioclk(void)
{
	uint32_t reg;
	unsigned rate = imx_get_mpllclk() / 1000;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC);
	if (reg & CLKCTRL_FRAC_CLKGATEIO)
		return 0U;	/* clock is off */

	rate *= 18U;
	rate /= GET_IOFRAC(reg);
	return rate * 1000;
}

/**
 * Setup a new frequency to the IOCLK domain.
 * @param nc New frequency in [Hz]
 *
 * The FRAC divider for the IOCLK must be between 18 (* 18/18) and 35 (* 18/35)
 */
unsigned imx_set_ioclk(unsigned nc)
{
	uint32_t reg;
	unsigned div;

	nc /= 1000;
	div = (imx_get_mpllclk() / 1000) * 18;
	div = DIV_ROUND_CLOSEST(div, nc);
	if (div > 0x3f)
		div = 0x3f;
	/* mask the current settings */
	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC) & ~(SET_IOFRAC(0x3f));
	writel(reg | SET_IOFRAC(div), IMX_CCM_BASE + HW_CLKCTRL_FRAC);
	/* enable the IO clock at its new frequency */
	writel(CLKCTRL_FRAC_CLKGATEIO, IMX_CCM_BASE + HW_CLKCTRL_FRAC + 8);

	return imx_get_ioclk();
}

/* this is CPU core clock */
unsigned imx_get_armclk(void)
{
	uint32_t reg;
	unsigned rate;

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ) & CLKCTRL_CLKSEQ_BYPASS_CPU)
		return imx_get_xtalclk() / GET_CPU_XTAL_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_CPU));

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_FRAC);
	if (reg & CLKCTRL_FRAC_CLKGATECPU)
		return 0U;	/* should not possible, shouldn't it? */

	rate = imx_get_mpllclk() / 1000;
	rate *= 18U;
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
	unsigned rate = imx_get_xtalclk();	/* runs from the 24 MHz crystal reference */

	return rate / (readl(IMX_CCM_BASE + HW_CLKCTRL_XBUS) & 0x3ff);
}

/* 'index' gets ignored on i.MX23 */
unsigned imx_get_sspclk(unsigned index)
{
	unsigned rate;

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_SSP) & CLKCTRL_SSP_CLKGATE)
		return 0U;	/* clock is off */

	if (readl(IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ) & CLKCTRL_CLKSEQ_BYPASS_SSP)
		rate = imx_get_xtalclk();
	else
		rate = imx_get_ioclk();

	return rate / GET_SSP_DIV(readl(IMX_CCM_BASE + HW_CLKCTRL_SSP));
}

/**
 * @param index Unit index (ignored on i.MX23)
 * @param nc New frequency in [Hz]
 * @param high != 0 if ioclk should be the source
 * @return The new possible frequency in [kHz]
 */
unsigned imx_set_sspclk(unsigned index, unsigned nc, int high)
{
	uint32_t reg;
	unsigned ssp_div;

	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_SSP) & ~CLKCTRL_SSP_CLKGATE;
	/* Datasheet says: Do not change the DIV setting if the clock is off */
	writel(reg, IMX_CCM_BASE + HW_CLKCTRL_SSP);
	/* Wait while clock is gated */
	while (readl(IMX_CCM_BASE + HW_CLKCTRL_SSP) & CLKCTRL_SSP_CLKGATE)
		;

	if (high)
		ssp_div = imx_get_ioclk();
	else
		ssp_div = imx_get_xtalclk();

	if (nc > ssp_div) {
		printf("Cannot setup SSP unit clock to %u Hz, base clock is only %u Hz\n", nc, ssp_div);
		ssp_div = 1U;
	} else {
		ssp_div = DIV_ROUND_UP(ssp_div, nc);
		if (ssp_div > CLKCTRL_SSP_DIV_MASK)
			ssp_div = CLKCTRL_SSP_DIV_MASK;
	}

	/* Set new divider value */
	reg = readl(IMX_CCM_BASE + HW_CLKCTRL_SSP) & ~CLKCTRL_SSP_DIV_MASK;
	writel(reg | SET_SSP_DIV(ssp_div), IMX_CCM_BASE + HW_CLKCTRL_SSP);

	/* Wait until new divider value is set */
	while (readl(IMX_CCM_BASE + HW_CLKCTRL_SSP) & CLKCTRL_SSP_BUSY)
		;

	if (high)
		/* switch to ioclock */
		writel(CLKCTRL_CLKSEQ_BYPASS_SSP, IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ + 8);
	else
		/* switch to 24 MHz crystal */
		writel(CLKCTRL_CLKSEQ_BYPASS_SSP, IMX_CCM_BASE + HW_CLKCTRL_CLKSEQ + 4);

	return imx_get_sspclk(index);
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
	printf("ioclk:   %10u kHz\n", imx_get_ioclk() / 1000);
	printf("emiclk:  %10u kHz\n", imx_get_emiclk() / 1000);
	printf("hclk:    %10u kHz\n", imx_get_hclk() / 1000);
	printf("xclk:    %10u kHz\n", imx_get_xclk() / 1000);
	printf("ssp:     %10u kHz\n", imx_get_sspclk(0) / 1000);
}
