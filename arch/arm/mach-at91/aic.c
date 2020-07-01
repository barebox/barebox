// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2015, Atmel Corporation
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 */

#include <mach/aic.h>
#include <io.h>

#define SFR_AICREDIR	0x54
#define SFR_SN1		0x50	/* Serial Number 1 Register */

void at91_aic_redir(void __iomem *sfr, u32 key)
{
	u32 key32;

	if (readl(sfr + SFR_AICREDIR) & 0x01)
		return;

	key32 = readl(sfr + SFR_SN1) ^ key;
	writel(key32 | 0x01, sfr + SFR_AICREDIR);
		/* bits[31:1] = key */
		/* bit[0] = 1 => all interrupts redirected to AIC */
		/* bit[0] = 0 => secure interrupts directed to SAIC,
					others to AIC (default) */
}
