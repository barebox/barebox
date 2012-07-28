/*
 * Copyright (C) 2012 Juergen Beisert
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

#include <config.h>
#include <common.h>
#include <init.h>
#include <clock.h>
#include <io.h>
#include <asm-generic/div64.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-clocks.h>

/*
 * The main clock tree:
 *
 * ref_in
 *    |
 *    v
 *    o-----------\
 *    |            MUX -o------------\
 *    |           / ^   |             MUX --- DIV_APLL ------- ARMCLK -> CPU core
 *    o--- APLL --  |   |            / |
 *    |             |   o--/2 -------  |
 *    |       APLL_SEL  |              |<-MISC_CON_SYN667
 *    |                 \              |
 *    o-----------\      MUX-o-------\ |
 *    |            MUX--/ ^  |        MUX --- DIV -o--------- HCLKx2 -> SDRAM (max. 266 MHz)
 *    |           / ^     |  |       /             |
 *    o---- MPLL--  |     |  o--/5 --              o-- DIV -- HCLK -> AXI / AHB (max. 133 MHz)
 *    |             |     |                        |
 *    |       MPLL_SEL  OTHERS_CLK_SELECT          o-- DIV -- PCLK -> APB (max. 66 MHz)
 *    |
 *    o-----------\
 *    |            MUX---- to various hardware
 *    |           / ^
 *    o---- EPLL--  EPLL_SEL
 *
 */

static unsigned s3c_get_apllclk(void)
{
	uint32_t m, p, s, reg_val;

	if (!(readl(S3C_CLK_SRC) & S3C_CLK_SRC_FOUTAPLL))
		return S3C64XX_CLOCK_REFERENCE;

	reg_val = readl(S3C_APLLCON);
	if (!(reg_val & S3C_APLLCON_ENABLE))
		return 0;
	m = S3C_APLLCON_GET_MDIV(reg_val);
	p = S3C_APLLCON_GET_PDIV(reg_val);
	s = S3C_APLLCON_GET_SDIV(reg_val);

	return (S3C64XX_CLOCK_REFERENCE * m) / (p << s);
}

uint32_t s3c_get_mpllclk(void)
{
	uint32_t m, p, s, reg_val;

	if (!(readl(S3C_CLK_SRC) & S3C_CLK_SRC_FOUTMPLL))
		return S3C64XX_CLOCK_REFERENCE;

	reg_val = readl(S3C_MPLLCON);
	if (!(reg_val & S3C_MPLLCON_ENABLE))
		return 0;

	m = S3C_MPLLCON_GET_MDIV(reg_val);
	p = S3C_MPLLCON_GET_PDIV(reg_val);
	s = S3C_MPLLCON_GET_SDIV(reg_val);

	return (S3C64XX_CLOCK_REFERENCE * m) / (p << s);
}

unsigned s3c_get_epllclk(void)
{
	u32 m, p, s, k, reg0_val, reg1_val;
	u64 tmp;

	if (!(readl(S3C_CLK_SRC) & S3C_CLK_SRC_FOUTEPLL))
		return S3C64XX_CLOCK_REFERENCE;

	reg0_val = readl(S3C_EPLLCON0);
	if (!(reg0_val & S3C_EPLLCON0_ENABLE))
		return 0;	/* PLL is disabled */

	reg1_val = readl(S3C_EPLLCON1);
	m = S3C_EPLLCON0_GET_MDIV(reg0_val);
	p = S3C_EPLLCON0_GET_PDIV(reg0_val);
	s = S3C_EPLLCON0_GET_SDIV(reg0_val);
	k = S3C_EPLLCON1_GET_KDIV(reg1_val);

	tmp = S3C64XX_CLOCK_REFERENCE;
	tmp *= (m << 16) + k;
	do_div(tmp, (p << s));

	return (unsigned)(tmp >> 16);
}

unsigned s3c_set_epllclk(unsigned m, unsigned p, unsigned s, unsigned k)
{
	u32 con0, con1, src = readl(S3C_CLK_SRC) & ~S3C_CLK_SRC_FOUTEPLL;

	/* do not use the EPLL clock when it is in transit to the new frequency */
	writel(src, S3C_CLK_SRC);

	con0 = S3C_EPLLCON0_SET_MDIV(m) | S3C_EPLLCON0_SET_PDIV(p) |
				S3C_EPLLCON0_SET_SDIV(s) | S3C_EPLLCON0_ENABLE;
	con1 = S3C_EPLLCON1_SET_KDIV(k);

	/*
	 * After changing the multiplication value 'm' the PLL output will
	 * be masked for the time set in the EPLL_LOCK register until it
	 * settles to the new frequency. EPLL_LOCK contains a value for a
	 * simple counter which counts the external reference clock.
	 */
	writel(con0, S3C_EPLLCON0);
	writel(con1, S3C_EPLLCON1);

	udelay((1000000000 / S3C64XX_CLOCK_REFERENCE)
		* (S3C_EPLL_LOCK_PLL_LOCKTIME(readl(S3C_EPLL_LOCK)) + 1) / 1000);

	/* enable the EPLL's clock output to the system */
	writel(src | S3C_CLK_SRC_FOUTEPLL, S3C_CLK_SRC);

	return s3c_get_epllclk();
}

uint32_t s3c_get_fclk(void)
{
	unsigned clk;

	clk = s3c_get_apllclk();
	if (readl(S3C_MISC_CON) & S3C_MISC_CON_SYN667)
		clk /= 2;

	return clk / (S3C_CLK_DIV0_GET_ADIV(readl(S3C_CLK_DIV0)) + 1);
}

static unsigned s3c_get_hclk_in(void)
{
	unsigned clk;

	if (readl(S3C_OTHERS) & S3C_OTHERS_CLK_SELECT)
		clk = s3c_get_apllclk();
	else
		clk = s3c_get_mpllclk();

	if (readl(S3C_MISC_CON) & S3C_MISC_CON_SYN667)
		clk /= 5;

	return clk;
}

static unsigned s3c_get_hclkx2(void)
{
	return s3c_get_hclk_in() /
			(S3C_CLK_DIV0_GET_HCLK2(readl(S3C_CLK_DIV0)) + 1);
}

uint32_t s3c_get_hclk(void)
{
	return s3c_get_hclkx2() /
			(S3C_CLK_DIV0_GET_HCLK(readl(S3C_CLK_DIV0)) + 1);
}

uint32_t s3c_get_pclk(void)
{
	return s3c_get_hclkx2() /
			(S3C_CLK_DIV0_GET_PCLK(readl(S3C_CLK_DIV0)) + 1);
}

static void s3c_init_mpll_dout(void)
{
	unsigned reg;

	/* keep it at the same frequency as HCLKx2 */
	reg = readl(S3C_CLK_DIV0) | S3C_CLK_DIV0_SET_MPLL_DIV(1); /* e.g. / 2 */
	writel(reg, S3C_CLK_DIV0);
}

/* configure and enable UCLK1 */
static int s3c_init_uart_clock(void)
{
	unsigned reg;

	s3c_init_mpll_dout();	/* to have a reliable clock source */

	/* source the UART clock from the MPLL, currently *not* from EPLL */
	reg = readl(S3C_CLK_SRC) | S3C_CLK_SRC_UARTMPLL;
	writel(reg, S3C_CLK_SRC);

	/* keep UART clock at the same frequency than the PCLK */
	reg = readl(S3C_CLK_DIV2) & ~S3C_CLK_DIV2_UART_MASK;
	reg |= S3C_CLK_DIV2_SET_UART(0x3);	/* / 4 */
	writel(reg, S3C_CLK_DIV2);

	/* ensure this very special clock is running */
	reg = readl(S3C_SCLK_GATE) | S3C_SCLK_GATE_UART;
	writel(reg, S3C_SCLK_GATE);

	return 0;
}
core_initcall(s3c_init_uart_clock);

/*                                         UART source selection
 * The UART related clock path:                     |
 *                                                  v
 * PCLK --------------------------------------o-----0-\
 * ???? -------------------------------UCLK0--|-----1--\MUX----- UART
 * MPLL -----DIV0------\                      +-----2--/
 *                     MUX---DIV2------UCLK1--------3-/
 * EPLL ---------------/
 *                      ^SRC_UARTMPLL
 */
unsigned s3c_get_uart_clk(unsigned source)
{
	u32 reg;
	unsigned clk, pdiv, uartpdiv;

	switch (source) {
	default: /* PCLK */
		clk = s3c_get_pclk();
		pdiv = uartpdiv = 1;
		break;
	case 1: /* UCLK0 */
		clk = 0;
		pdiv = uartpdiv = 1;	/* TODO */
		break;
	case 3: /* UCLK1 */
		reg = readl(S3C_CLK_SRC);
		if (reg & S3C_CLK_SRC_UARTMPLL) {
			clk = s3c_get_mpllclk();
			pdiv = S3C_CLK_DIV0_GET_MPLL_DIV(readl(S3C_CLK_DIV0)) + 1;
		} else {
			clk = s3c_get_epllclk();
			pdiv = 1;
		}
		uartpdiv = S3C_CLK_DIV2_GET_UART(readl(S3C_CLK_DIV2)) + 1;
		break;
	}

	return clk / pdiv / uartpdiv;
}

/*
 * The MMC related clock path:
 *
 *              MMCx_SEL
 *                 |
 *                 v
 * EPLLout --------0-\
 * MPLLout --DIV0--1--\-------SCLK_MMCx----DIV_MMCx------>HSMMCx
 * EPLLin  --------2--/        on/off      / 1..16
 * 27 MHz  --------3-/
 *
 * The datasheet is not very precise here, so the schematic shown above was
 * made by checking various bits in the SYSCON.
 */
unsigned s3c_get_hsmmc_clk(int id)
{
	u32 sel, div, sclk = readl(S3C_SCLK_GATE);
	unsigned bclk;

	if (!(sclk & S3C_SCLK_GATE_MMC(id)))
		return 0; /* disabled */

	sel = S3C_CLK_SRC_GET_MMC_SEL(id, readl(S3C_CLK_SRC));
	switch (sel) {
	case 0:
		bclk = s3c_get_epllclk();
		break;
	case 1:
		bclk = s3c_get_mpllclk();
		bclk >>= S3C_CLK_DIV0_GET_MPLL_DIV(readl(S3C_CLK_DIV0));
		break;
	case 2:
		bclk = S3C64XX_CLOCK_REFERENCE;
		break;
	case 3:
		bclk = 27000000;
		break;
	}

	div = S3C_CLK_DIV0_GET_MMC(id, readl(S3C_CLK_DIV0)) + 1;

	return bclk / div;
}

void s3c_set_hsmmc_clk(int id, int src, unsigned div)
{
	u32 reg;

	if (!div)
		div = 1;

	writel(readl(S3C_SCLK_GATE) & ~S3C_SCLK_GATE_MMC(id), S3C_SCLK_GATE);

	/* select the new clock source */
	reg = readl(S3C_CLK_SRC) & ~S3C_CLK_SRC_SET_MMC_SEL(id, ~0);
	reg |= S3C_CLK_SRC_SET_MMC_SEL(id, src);
	writel(reg, S3C_CLK_SRC);

	/* select the new pre-divider */
	reg = readl(S3C_CLK_DIV0) & ~ S3C_CLK_DIV0_SET_MMC(id, ~0);
	reg |= S3C_CLK_DIV0_SET_MMC(id, div - 1);
	writel(reg, S3C_CLK_DIV0);

	/* calling this function implies enabling of the clock */
	writel(readl(S3C_SCLK_GATE) | S3C_SCLK_GATE_MMC(id), S3C_SCLK_GATE);
}

int s3c64xx_dump_clocks(void)
{
	printf("refclk:  %7d kHz\n", S3C64XX_CLOCK_REFERENCE / 1000);
	printf("apll:    %7d kHz\n", s3c_get_apllclk() / 1000);
	printf("mpll:    %7d kHz\n", s3c_get_mpllclk() / 1000);
	printf("epll:    %7d kHz\n", s3c_get_epllclk() / 1000);
	printf("CPU:     %7d kHz\n", s3c_get_fclk() / 1000);
	printf("hclkx2:  %7d kHz\n", s3c_get_hclkx2() / 1000);
	printf("hclk:    %7d kHz\n", s3c_get_hclk() / 1000);
	printf("pclk:    %7d kHz\n", s3c_get_pclk() / 1000);
	return 0;
}

late_initcall(s3c64xx_dump_clocks);
