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
 */

#include <common.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <init.h>

ulong imx_get_mpl_dpdgck_clk(void)
{
	ulong infreq;

	if ((readl(MX31_CCM_BASE_ADDR + CCM_CCMR) & CCMR_PRCS_MASK) == CCMR_FPM)
		infreq = CONFIG_MX31_CLK32 * 1024;
	else
		infreq = CONFIG_MX31_HCLK_FREQ;

	return imx_decode_pll(readl(MX31_CCM_BASE_ADDR + CCM_MPCTL), infreq);
}

ulong imx_get_mcu_main_clk(void)
{
	/* For now we assume mpl_dpdgck_clk == mcu_main_clk
	 * which should be correct for most boards
	 */
	return imx_get_mpl_dpdgck_clk();
}

/**
 * Calculate the current pixel clock speed (aka HSP or IPU)
 * @return 0 on failure or current frequency in Hz
 */
ulong imx_get_lcdclk(void)
{
	ulong hsp_podf = (readl(MX31_CCM_BASE_ADDR + CCM_PDR0) >> 11) & 0x03;
	ulong base_clk = imx_get_mcu_main_clk();

	return base_clk / (hsp_podf + 1);
}

ulong imx_get_perclk1(void)
{
	u32 freq = imx_get_mcu_main_clk();
	u32 pdr0 = readl(MX31_CCM_BASE_ADDR + CCM_PDR0);

	freq /= ((pdr0 >> 3) & 0x7) + 1;
	freq /= ((pdr0 >> 6) & 0x3) + 1;

	return freq;
}

void imx_dump_clocks(void)
{
	ulong cpufreq = imx_get_mcu_main_clk();
	printf("mx31 cpu clock: %ldMHz\n",cpufreq / 1000000);
	printf("ipg clock     : %ldHz\n", imx_get_perclk1());
}

ulong imx_get_uartclk(void)
{
	return imx_get_perclk1();
}

ulong imx_get_gptclk(void)
{
	return imx_get_perclk1();
}

