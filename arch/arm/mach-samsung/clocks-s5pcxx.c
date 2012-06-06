/*
 * Copyright (C) 2012 Alexey Galakhov
 * Copyright (C) 2012 Juergen Beisert, Pengutronix
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
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-clocks.h>

static inline uint32_t clkdiv(uint32_t clk, unsigned bit, unsigned mask)
{
	uint32_t ratio = (readl(S5P_CLK_DIV0) >> bit) & mask;
	return clk / (ratio + 1);
}

uint32_t s3c_get_mpllclk(void)
{
	uint32_t m, p, s;
	uint32_t reg = readl(S5P_xPLL_CON + S5P_MPLL);
	m = (reg >> 16) & 0x3ff;
	p = (reg >> 8) & 0x3f;
	s = (reg >> 0) & 0x7;
	return m * ((S5PCXX_CLOCK_REFERENCE) / (p << s));
}

uint32_t s3c_get_apllclk(void)
{
	uint32_t m, p, s;
	uint32_t reg = readl(S5P_xPLL_CON + S5P_APLL);
	m = (reg >> 16) & 0x3ff;
	p = (reg >> 8) & 0x3f;
	s = (reg >> 0) & 0x7;
	s -= 1;
	return m * ((S5PCXX_CLOCK_REFERENCE) / (p << s));
}

static uint32_t s5p_get_a2mclk(void)
{
	return clkdiv(s3c_get_apllclk(), 4, 0x7);
}

static uint32_t s5p_get_moutpsysclk(void)
{
	if (readl(S5P_CLK_SRC0) & (1 << 24)) /* MUX_PSYS */
		return s5p_get_a2mclk();
        else
		return s3c_get_mpllclk();
}

uint32_t s3c_get_hclk(void)
{
	return clkdiv(s5p_get_moutpsysclk(), 24, 0xf);
}

uint32_t s3c_get_pclk(void)
{
	return clkdiv(s3c_get_hclk(), 28, 0x7);
}

/* we are using the internal 'uclk1' as the UART source */
static unsigned s3c_get_uart_clk_uclk1(void)
{
	unsigned clk = s3c_get_mpllclk();	/* TODO check for EPLL */
	unsigned uartpdiv = ((readl(S5P_CLK_DIV4) >> 16) & 0x3) + 1; /* TODO this is UART0 only */
	return clk / uartpdiv;
}

unsigned s3c_get_uart_clk(unsigned src) {
	return (src & 1) ? s3c_get_uart_clk_uclk1() : s3c_get_pclk();
}

int s5pcxx_dump_clocks(void)
{
	printf("refclk:  %7d kHz\n", S5PCXX_CLOCK_REFERENCE / 1000);
	printf("apll:    %7d kHz\n", s3c_get_apllclk() / 1000);
	printf("mpll:    %7d kHz\n", s3c_get_mpllclk() / 1000);
/*	printf("CPU:     %7d kHz\n", s3c_get_cpuclk() / 1000); */
	printf("hclk:    %7d kHz\n", s3c_get_hclk() / 1000);
	printf("pclk:    %7d kHz\n", s3c_get_pclk() / 1000);
	return 0;
}

late_initcall(s5pcxx_dump_clocks);
