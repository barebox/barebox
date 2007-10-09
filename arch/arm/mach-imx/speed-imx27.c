#include <common.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
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
		return 26000000 / 1.5;
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

	if (!(cscr & CSCR_ARM_SRC_MPLL))
		fref = (fref * 2) / 3;

	div = ((cscr >> 12) & 0x3) + 1;

	return fref / div;
}

ulong imx_get_spllclk(void)
{
	ulong cscr = CSCR;
	ulong fref;

	if (cscr & CSCR_SP_SEL)
		fref = clk_in_26m();
	else
		fref = clk_in_32k();

	return imx_decode_pll(SPCTL0, fref);
}

static ulong imx_decode_perclk(ulong div)
{
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

int imx_dump_clocks(void)
{
	printf("mpll:    %10d\n", imx_get_mpllclk());
	printf("arm:     %10d\n", imx_get_armclk());
	printf("system:  %10d\n", imx_get_spllclk());
	printf("perclk1: %10d\n", imx_get_perclk1());
	printf("perclk2: %10d\n", imx_get_perclk2());
	printf("perclk3: %10d\n", imx_get_perclk3());
	printf("perclk4: %10d\n", imx_get_perclk4());
	printf("clkin26: %10d\n", clk_in_26m());
	return 0;
}

late_initcall(imx_dump_clocks);

