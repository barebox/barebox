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
#include <mach/generic.h>
#include <mach/clock.h>
#include <init.h>

#ifndef CLK32
#define CLK32 32000
#endif

static ulong clk_in_32k(void)
{
	return 1024 * CLK32;
}

static ulong clk_in_26m(void)
{
	if (CSCR & CSCR_OSC26M_DIV1P5) {
		/* divide by 1.5 */
		return 173333333;
	} else {
		/* divide by 1 */
		return 26000000;
	}
}

ulong imx_get_mpllclk(void)
{
	ulong cscr = CSCR;
	ulong fref;

	if (cscr & CSCR_MCU_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	return imx_decode_pll(MPCTL0, fref);
}

ulong imx_get_armclk(void)
{
	ulong cscr = CSCR;
	ulong fref = imx_get_mpllclk();
	ulong div;

	if (!(cscr & CSCR_ARM_SRC_MPLL) &&
		(imx_silicon_revision() != IMX27_CHIP_REVISION_1_0))
		fref = (fref * 2) / 3;

	div = ((cscr >> 12) & 0x3) + 1;

	return fref / div;
}

ulong imx_get_ahbclk(void)
{
	ulong cscr = CSCR;
	ulong fref = imx_get_mpllclk();
	ulong div;

	if (imx_silicon_revision() == IMX27_CHIP_REVISION_1_0)
		div = ((cscr >> 9) & 0xf) + 1;
	else
		div = ((cscr >> 8) & 0x3) + 1;

	return ((fref * 2) / 3) / div;
}

ulong imx_get_ipgclk(void)
{
	ulong clk = imx_get_ahbclk();

	return clk >> 1;
}

ulong imx_get_fecclk(void)
{
	return imx_get_ipgclk();
}

ulong imx_get_spllclk(void)
{
	ulong cscr = CSCR;
	ulong spctl0;
	ulong fref;

	if (cscr & CSCR_SP_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	spctl0 = SPCTL0;
	SPCTL0 = spctl0;
	return imx_decode_pll(spctl0, fref);
}

static ulong imx_decode_perclk(ulong div)
{
	if (imx_silicon_revision() == IMX27_CHIP_REVISION_1_0)
		return imx_get_mpllclk() / div;
	else
		return (imx_get_mpllclk() * 2) / (div * 3);
}

ulong imx_get_perclk1(void)
{
	return imx_decode_perclk((PCDR1 & 0x3f) + 1);
}

ulong imx_get_perclk2(void)
{
	return imx_decode_perclk(((PCDR1 >> 8) & 0x3f) + 1);
}

ulong imx_get_perclk3(void)
{
	return imx_decode_perclk(((PCDR1 >> 16) & 0x3f) + 1);
}

ulong imx_get_perclk4(void)
{
	return imx_decode_perclk(((PCDR1 >> 24) & 0x3f) + 1);
}

ulong imx_get_uartclk(void)
{
	return imx_get_perclk1();
}

ulong imx_get_gptclk(void)
{
	return imx_decode_perclk((PCDR1 & 0x3f) + 1);
}

ulong imx_get_lcdclk(void)
{
	return imx_get_perclk3();
}

ulong imx_get_i2cclk(void)
{
	return imx_get_ipgclk();
}

ulong imx_get_mmcclk(void)
{
	return imx_get_perclk2();
}

void imx_dump_clocks(void)
{
	uint32_t	cid = CID;

	printf("chip id: [%d,%03x,%d,%03x]\n",
	       (cid >> 28) & 0xf, (cid >> 16) & 0xfff,
	       (cid >> 12) & 0xf, (cid >>  0) & 0xfff);

	printf("mpll:    %10ld Hz\n", imx_get_mpllclk());
	printf("spll:    %10ld Hz\n", imx_get_spllclk());
	printf("arm:     %10ld Hz\n", imx_get_armclk());
	printf("perclk1: %10ld Hz\n", imx_get_perclk1());
	printf("perclk2: %10ld Hz\n", imx_get_perclk2());
	printf("perclk3: %10ld Hz\n", imx_get_perclk3());
	printf("perclk4: %10ld Hz\n", imx_get_perclk4());
	printf("clkin26: %10ld Hz\n", clk_in_26m());
	printf("ahb:     %10ld Hz\n", imx_get_ahbclk());
	printf("ipg:     %10ld Hz\n", imx_get_ipgclk());
}

/*
 * Set the divider of the CLKO pin. Returns
 * the new divider (which may be smaller
 * than the desired one)
 */
int imx_clko_set_div(int num, int div)
{
	ulong pcdr;

	if (num != 1)
		return -ENODEV;

	div--;
	div &= 0x7;

	pcdr = PCDR0 & ~(7 << 22);
	pcdr |= div << 22;
	PCDR0 = pcdr;

	return div + 1;
}

/*
 * Set the clock source for the CLKO pin
 */
void imx_clko_set_src(int num, int src)
{
	unsigned long ccsr;

	if (num != 1)
		return;

	if (src < 0) {
		PCDR0 &= ~(1 << 25);
		return;
	}

	ccsr = CCSR & ~0x1f;
	ccsr |= src & 0x1f;
	CCSR = ccsr;

	PCDR0 |= (1 << 25);
}

