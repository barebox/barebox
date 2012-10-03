/*
 * Copyright (C) 2011 Alexander Aring <a.aring@phytec.de
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
#include <io.h>

#include <mci/twl6030.h>

/* MMC voltage */
#define OMAP4_CONTROL_PBIASLITE			0x4A100600
#define OMAP4_MMC1_PBIASLITE_VMODE		(1<<21)
#define OMAP4_MMC1_PBIASLITE_PWRDNZ		(1<<22)
#define OMAP4_MMC1_PWRDNZ				(1<<26)

void set_up_mmc_voltage_omap4(void)
{
	u32 value;

	value = readl(OMAP4_CONTROL_PBIASLITE);
	value &= ~(OMAP4_MMC1_PBIASLITE_PWRDNZ | OMAP4_MMC1_PWRDNZ);
	writel(value, OMAP4_CONTROL_PBIASLITE);

	twl6030_mci_power_init();

	value = readl(OMAP4_CONTROL_PBIASLITE);
	value |= (OMAP4_MMC1_PBIASLITE_VMODE | OMAP4_MMC1_PBIASLITE_PWRDNZ |
			OMAP4_MMC1_PWRDNZ);
	writel(value, OMAP4_CONTROL_PBIASLITE);
}
EXPORT_SYMBOL(set_up_mmc_voltage_omap4);
