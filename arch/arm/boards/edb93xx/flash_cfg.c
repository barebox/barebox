/*
 * Flash setup for Cirrus edb93xx boards
 *
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

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
