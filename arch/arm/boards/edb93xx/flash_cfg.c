// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Matthias Kaehlcke <matthias@kaehlcke.net>

/* Flash setup for Cirrus edb93xx boards */

#include <common.h>
#include <mach/ep93xx-regs.h>
#include <io.h>

#define SMC_BCR6_VALUE	(2 << SMC_BCR_IDCY_SHIFT | 5 << SMC_BCR_WST1_SHIFT | \
				SMC_BCR_BLE | 2 << SMC_BCR_WST2_SHIFT | \
				1 << SMC_BCR_MW_SHIFT)

void flash_cfg(void)
{
	struct smc_regs *smc = (struct smc_regs *)SMC_BASE;

	writel(SMC_BCR6_VALUE, &smc->bcr6);
}
