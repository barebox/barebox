#include <common.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>

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
		return 26000000 / 1.5;
	} else {
		/* divide by 1 */
		return 26000000;
	}
}

ulong imx_get_mcuclk(void)
{
	ulong cscr = CSCR;
	ulong fref;

	if (cscr & CSCR_MCU_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	return imx_decode_pll(MPCTL0, fref);
}

ulong imx_get_systemclk(void)
{
	ulong cscr = CSCR;
	ulong fref;

	if (cscr & CSCR_SP_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	return imx_decode_pll(SPCTL0, fref);
}

ulong imx_get_perclk1(void)
{
	return imx_get_mcuclk() / ((PCDR1 & 0x3f) + 1);
}

ulong imx_get_perclk2(void)
{
	return imx_get_mcuclk() / (((PCDR1 >> 8) & 0x3f) + 1);
}

ulong imx_get_perclk3(void)
{
	return imx_get_mcuclk() / (((PCDR1 >> 16) & 0x3f) + 1);
}

ulong imx_get_perclk4(void)
{
	return imx_get_mcuclk() / (((PCDR1 >> 24) & 0x3f) + 1);
}

