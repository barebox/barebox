/*
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <mach/s3c-busctl.h>

/**
 * Calculate the current M-PLL clock.
 * @return Current frequency in Hz
 */
uint32_t s3c_get_mpllclk(void)
{
	uint32_t m, p, s, reg_val;

	reg_val = readl(S3C_MPLLCON);
	m = ((reg_val & 0xFF000) >> 12) + 8;
	p = ((reg_val & 0x003F0) >> 4) + 2;
	s = reg_val & 0x3;
#ifdef CONFIG_CPU_S3C2410
	return (S3C24XX_CLOCK_REFERENCE * m) / (p << s);
#endif
#ifdef CONFIG_CPU_S3C2440
	return 2 * m * (S3C24XX_CLOCK_REFERENCE / (p << s));
#endif
}

/**
 * Calculate the current U-PLL clock
 * @return Current frequency in Hz
 */
uint32_t s3c_get_upllclk(void)
{
	uint32_t m, p, s, reg_val;

	reg_val = readl(S3C_UPLLCON);
	m = ((reg_val & 0xFF000) >> 12) + 8;
	p = ((reg_val & 0x003F0) >> 4) + 2;
	s = reg_val & 0x3;

	return (S3C24XX_CLOCK_REFERENCE * m) / (p << s);
}

/**
 * Calculate the FCLK frequency used for the ARM CPU core
 * @return Current frequency in Hz
 */
uint32_t s3c_get_fclk(void)
{
	return s3c_get_mpllclk();
}

/**
 * Calculate the HCLK frequency used for the AHB bus (CPU to main peripheral)
 * @return Current frequency in Hz
 */
uint32_t s3c_get_hclk(void)
{
	uint32_t f_clk;

	f_clk = s3c_get_fclk();
#ifdef CONFIG_CPU_S3C2410
	if (readl(S3C_CLKDIVN) & 0x02)
		return f_clk >> 1;
#endif
#ifdef CONFIG_CPU_S3C2440
	switch(readl(S3C_CLKDIVN) & 0x06) {
	case 2:
		return f_clk >> 1;
	case 4:
		return f_clk >> 2;	/* TODO consider CAMDIVN */
	case 6:
		return f_clk / 3;	/* TODO consider CAMDIVN */
	}
#endif
	return f_clk;
}

/**
 * Calculate the PCLK frequency used for the slower peripherals
 * @return Current frequency in Hz
 */
uint32_t s3c_get_pclk(void)
{
	uint32_t p_clk;

	p_clk = s3c_get_hclk();
	if (readl(S3C_CLKDIVN) & 0x01)
		return p_clk >> 1;
	return p_clk;
}

/**
 * Return correct UART frequency based on the UCON register
 */
unsigned s3c_get_uart_clk(unsigned src)
{
	switch (src & 3) {
	case 0:
	case 2:
		return s3c_get_pclk();
	case 1:
		return 0; /* TODO UEXTCLK */
	case 3:
		return 0; /* TODO FCLK/n */
	}
	return 0; /* not reached, to make compiler happy */
}

/**
 * Show the user the current clock settings
 */
static int s3c24xx_dump_clocks(void)
{
	printf("refclk:  %7d kHz\n", S3C24XX_CLOCK_REFERENCE / 1000);
	printf("mpll:    %7d kHz\n", s3c_get_mpllclk() / 1000);
	printf("upll:    %7d kHz\n", s3c_get_upllclk() / 1000);
	printf("fclk:    %7d kHz\n", s3c_get_fclk() / 1000);
	printf("hclk:    %7d kHz\n", s3c_get_hclk() / 1000);
	printf("pclk:    %7d kHz\n", s3c_get_pclk() / 1000);
	printf("SDRAM1:   CL%d@%dMHz\n", ((readl(S3C_BANKCON6) & 0xc) >> 2) + 2,
						s3c_get_hclk() / 1000000);
	if ((readl(S3C_BANKCON7) & (0x3 << 15)) == (0x3 << 15))
		printf("SDRAM2:   CL%d@%dMHz\n",
			((readl(S3C_BANKCON7) & 0xc) >> 2) + 2,
			s3c_get_hclk() / 1000000);
	return 0;
}

late_initcall(s3c24xx_dump_clocks);
