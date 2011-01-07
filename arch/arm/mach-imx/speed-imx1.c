/*
 *
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
 *
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <common.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <init.h>
#include <driver.h>

ulong imx_get_spllclk(void)
{
	return imx_decode_pll(SPCTL0, CONFIG_SYSPLL_CLK_FREQ);
}

ulong imx_get_mpllclk(void)
{
	return imx_decode_pll(MPCTL0, CONFIG_SYSPLL_CLK_FREQ);
}

ulong imx_get_fclk(void)
{
	return (( CSCR>>15)&1) ? imx_get_mpllclk()>>1 : imx_get_mpllclk();
}

ulong imx_get_hclk(void)
{
	u32 bclkdiv = (( CSCR >> 10 ) & 0xf) + 1;
	return imx_get_spllclk() / bclkdiv;
}

ulong imx_get_bclk(void)
{
	return imx_get_hclk();
}

ulong imx_get_perclk1(void)
{
	return imx_get_spllclk() / (((PCDR) & 0xf)+1);
}

ulong imx_get_perclk2(void)
{
	return imx_get_spllclk() / (((PCDR>>4) & 0xf)+1);
}

ulong imx_get_perclk3(void)
{
	return imx_get_spllclk() / (((PCDR>>16) & 0x7f)+1);
}

ulong imx_get_uartclk(void)
{
	return imx_get_perclk1();
}

ulong imx_get_gptclk(void)
{
	return imx_get_perclk1();
}

void imx_dump_clocks(void)
{
	printf("spll:    %10ld Hz\n", imx_get_spllclk());
	printf("mpll:    %10ld Hz\n", imx_get_mpllclk());
	printf("fclk:    %10ld Hz\n", imx_get_fclk());
	printf("hclk:    %10ld Hz\n", imx_get_hclk());
	printf("bclk:    %10ld Hz\n", imx_get_bclk());
	printf("perclk1: %10ld Hz\n", imx_get_perclk1());
	printf("perclk2: %10ld Hz\n", imx_get_perclk2());
	printf("perclk3: %10ld Hz\n", imx_get_perclk3());
	printf("uart:    %10ld Hz\n", imx_get_uartclk());
	printf("gpt:     %10ld Hz\n", imx_get_gptclk());
}

