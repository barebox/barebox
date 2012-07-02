/*
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
#include <asm-generic/errno.h>
#include <mach/imx-regs.h>
#include <io.h>
#include <mach/clock.h>
#include <mach/generic.h>
#include <init.h>

unsigned long imx_get_mpllclk(void)
{
	ulong mpctl = readl(IMX_CCM_BASE + CCM_MPCTL);
	return imx_decode_pll(mpctl, CONFIG_MX35_HCLK_FREQ);
}

static unsigned long imx_get_ppllclk(void)
{
	ulong ppctl = readl(IMX_CCM_BASE + CCM_PPCTL);
	return imx_decode_pll(ppctl, CONFIG_MX35_HCLK_FREQ);
}

struct arm_ahb_div {
	unsigned char arm, ahb, sel;
};

static struct arm_ahb_div clk_consumer[] = {
	{ .arm = 1, .ahb = 4, .sel = 0},
	{ .arm = 1, .ahb = 3, .sel = 1},
	{ .arm = 2, .ahb = 2, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 4, .ahb = 1, .sel = 0},
	{ .arm = 1, .ahb = 5, .sel = 0},
	{ .arm = 1, .ahb = 8, .sel = 0},
	{ .arm = 1, .ahb = 6, .sel = 1},
	{ .arm = 2, .ahb = 4, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
	{ .arm = 4, .ahb = 2, .sel = 0},
	{ .arm = 0, .ahb = 0, .sel = 0},
};

static unsigned long imx_get_armclk(void)
{
	unsigned long pdr0 = readl(IMX_CCM_BASE + CCM_PDR0);
	struct arm_ahb_div *aad;
	unsigned long fref = imx_get_mpllclk();

	/* consumer path is selected */
	aad = &clk_consumer[(pdr0 >> 16) & 0xf];
	if (aad->sel)
		fref = fref * 3 / 4;

	return fref / aad->arm;
}

unsigned long imx_get_ahbclk(void)
{
	unsigned long pdr0 = readl(IMX_CCM_BASE + CCM_PDR0);
	struct arm_ahb_div *aad;
	unsigned long fref = imx_get_mpllclk();

	aad = &clk_consumer[(pdr0 >> 16) & 0xf];
	if (aad->sel)
		fref = fref * 3 / 4;

	return fref / aad->ahb;
}

unsigned long imx_get_ipgclk(void)
{
	ulong clk = imx_get_ahbclk();

	return clk >> 1;
}

static unsigned long get_3_3_div(unsigned long in)
{
	return (((in >> 3) & 0x7) + 1) * ((in & 0x7) + 1);
}

static unsigned long imx_get_ipg_perclk(void)
{
	ulong pdr0 = readl(IMX_CCM_BASE + CCM_PDR0);
	ulong pdr4 = readl(IMX_CCM_BASE + CCM_PDR4);
	ulong div;
	ulong fref;

	if (pdr0 & PDR0_PER_SEL) {
		/* perclk from arm high frequency clock and synched with AHB clki */
		fref = imx_get_armclk();
		div = get_3_3_div((pdr4 >> 16));
	} else {
		/* perclk from AHB divided clock */
		fref = imx_get_ahbclk();
		div = ((pdr0 >> 12) & 0x7) + 1;
	}

	return fref / div;
}

unsigned long imx_get_gptclk(void)
{
	return imx_get_ipgclk();
}

/**
 * Calculate the current pixel clock speed (aka HSP or IPU)
 * @return 0 on failure or current frequency in Hz
 */
unsigned long imx_get_lcdclk(void)
{
	unsigned long hsp_podf = (readl(IMX_CCM_BASE + CCM_PDR0) >> 20) & 0x03;
	unsigned long base_clk = imx_get_armclk();

	if (base_clk > 400 * 1000 * 1000) {
		switch(hsp_podf) {
		case 0:
			return base_clk >> 2;
		case 1:
			return base_clk >> 3;
		case 2:
			return base_clk / 3;
		}
	} else {
		switch(hsp_podf) {
		case 0:
		case 2:
			return base_clk / 3;
		case 1:
			return base_clk / 6;
		}
	}

	return 0;
}

unsigned long imx_get_uartclk(void)
{
	unsigned long pdr3 = readl(IMX_CCM_BASE + CCM_PDR3);
	unsigned long pdr4 = readl(IMX_CCM_BASE + CCM_PDR4);
	unsigned long div = get_3_3_div(pdr4 >> 10);

	if (pdr3 & (1 << 14))
		return imx_get_armclk() / div;
	else
		return imx_get_ppllclk() / div;
}

unsigned long imx_get_mmcclk(void)
{
	unsigned long pdr3 = readl(IMX_CCM_BASE + CCM_PDR3);
	unsigned long div = get_3_3_div(pdr3);

	if (pdr3 & (1 << 6))
		return imx_get_armclk() / div;
	else
		return imx_get_ppllclk() / div;
}

ulong imx_get_fecclk(void)
{
	return imx_get_ipgclk();
}

ulong imx_get_i2cclk(void)
{
	return imx_get_ipg_perclk();
}

void imx_dump_clocks(void)
{
	printf("mpll:    %10ld Hz\n", imx_get_mpllclk());
	printf("ppll:    %10ld Hz\n", imx_get_ppllclk());
	printf("arm:     %10ld Hz\n", imx_get_armclk());
	printf("gpt:     %10ld Hz\n", imx_get_gptclk());
	printf("ahb:     %10ld Hz\n", imx_get_ahbclk());
	printf("ipg:     %10ld Hz\n", imx_get_ipgclk());
	printf("ipg_per: %10ld Hz\n", imx_get_ipg_perclk());
	printf("uart:	 %10ld Hz\n", imx_get_uartclk());
	printf("sdhc1:   %10ld Hz\n", imx_get_mmcclk());
}

/*
 * Set the divider of the CLKO pin. Returns
 * the new divider (which may be smaller
 * than the desired one)
 */
int imx_clko_set_div(int num, int div)
{
	unsigned long cosr = readl(IMX_CCM_BASE + CCM_COSR);

	if (num != 1)
		return -ENODEV;

	div -= 1;
	div &= 0x3f;

	cosr &= ~(0x3f << 10);
	cosr |= div << 10;

	writel(cosr, IMX_CCM_BASE + CCM_COSR);

	return div + 1;
}

/*
 * Set the clock source for the CLKO pin
 */
void imx_clko_set_src(int num, int src)
{
	unsigned long cosr = readl(IMX_CCM_BASE + CCM_COSR);

	if (num != 1)
		return;

	if (src < 0) {
		cosr &= ~(1 << 5);
		writel(cosr, IMX_CCM_BASE + CCM_COSR);
		return;
	}

	cosr |= 1 << 5;
	cosr &= ~0x1f;
	cosr &= ~(1 << 6);
	cosr |= src & 0x1f;

	writel(cosr, IMX_CCM_BASE + CCM_COSR);
}

