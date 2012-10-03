/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * Copyright 2004, 2007-2011 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2003 Motorola Inc.
 * Xianghua Xiao, (X.Xiao@motorola.com)
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 */

#include <common.h>
#include <asm/processor.h>
#include <mach/clock.h>
#include <mach/immap_85xx.h>
#include <mach/mpc85xx.h>

void fsl_get_sys_info(struct sys_info *sysInfo)
{
	void __iomem *gur = (void __iomem *)(MPC85xx_GUTS_ADDR);
	uint plat_ratio, e500_ratio, half_freqSystemBus;
	uint lcrr_div;
	int i;

	plat_ratio = in_be32(gur + MPC85xx_GUTS_PORPLLSR_OFFSET) & 0x0000003e;
	plat_ratio >>= 1;
	sysInfo->freqSystemBus = plat_ratio * CFG_SYS_CLK_FREQ;

	/*
	 * Divide before multiply to avoid integer
	 * overflow for processor speeds above 2GHz.
	 */
	half_freqSystemBus = sysInfo->freqSystemBus/2;
	for (i = 0; i < fsl_cpu_numcores(); i++) {
		e500_ratio = (in_be32(gur + MPC85xx_GUTS_PORPLLSR_OFFSET) >>
				(i * 8 + 16)) & 0x3f;
		sysInfo->freqProcessor[i] = e500_ratio * half_freqSystemBus;
	}

	/* Note: freqDDRBus is the MCLK frequency, not the data rate. */
	sysInfo->freqDDRBus = sysInfo->freqSystemBus;

#ifdef CFG_DDR_CLK_FREQ
	{
		u32 ddr_ratio = (in_be32(gur + MPC85xx_GUTS_PORPLLSR_OFFSET) &
					MPC85xx_PORPLLSR_DDR_RATIO) >>
					MPC85xx_PORPLLSR_DDR_RATIO_SHIFT;
		if (ddr_ratio != 0x7)
			sysInfo->freqDDRBus = ddr_ratio * CFG_DDR_CLK_FREQ;
	}
#endif

	lcrr_div = in_be32(LBC_BASE_ADDR + FSL_LBC_LCCR) & LCRR_CLKDIV;

	if (lcrr_div == 2 || lcrr_div == 4 || lcrr_div == 8) {
		/*
		 * The entire PQ38 family use the same bit-representation
		 * for twice the clock divider values.
		 */
		lcrr_div *= 2;

		sysInfo->freqLocalBus = sysInfo->freqSystemBus / lcrr_div;
	} else {
		/* In case anyone cares what the unknown value is */
		sysInfo->freqLocalBus = lcrr_div;
	}
}

unsigned long fsl_get_bus_freq(ulong dummy)
{
	struct sys_info sys_info;

	fsl_get_sys_info(&sys_info);

	return sys_info.freqSystemBus;
}

unsigned long fsl_get_timebase_clock(void)
{
	struct sys_info sysinfo;

	fsl_get_sys_info(&sysinfo);

	return (sysinfo.freqSystemBus + 4UL)/8UL;
}

unsigned long fsl_get_i2c_freq(void)
{
	struct sys_info sysinfo;

	fsl_get_sys_info(&sysinfo);

	return sysinfo.freqSystemBus / 2;
}
