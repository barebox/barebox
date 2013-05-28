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

#include <mfd/twl6030.h>

/* MMC voltage */
#define OMAP4_CONTROL_PBIASLITE			0x4A100600
#define OMAP4_MMC1_PBIASLITE_VMODE		(1<<21)
#define OMAP4_MMC1_PBIASLITE_PWRDNZ		(1<<22)
#define OMAP4_MMC1_PWRDNZ				(1<<26)

static void twl6030_mci_write(u8 address, u8 data)
{
	int ret;
	struct twl6030 *twl6030 = twl6030_get();

	ret = twl6030_reg_write(twl6030, address, data);
	if (ret != 0)
		printf("TWL6030 Write[0x%x] Error %d\n", address, ret);
}

void set_up_mmc_voltage_omap4(void)
{
	u32 value;

	value = readl(OMAP4_CONTROL_PBIASLITE);
	value &= ~(OMAP4_MMC1_PBIASLITE_PWRDNZ | OMAP4_MMC1_PWRDNZ);
	writel(value, OMAP4_CONTROL_PBIASLITE);

	twl6030_mci_write(TWL6030_PMCS_VMMC_CFG_VOLTAGE,
			  TWL6030_VMMC_WR_S |
			  TWL6030_VMMC_VSEL_0 | TWL6030_VMMC_VSEL_2 |
			  TWL6030_VMMC_VSEL_4);
	twl6030_mci_write(TWL6030_PMCS_VMMC_CFG_STATE,
			  TWL6030_VMMC_STATE0 | TWL6030_VMMC_GRP_APP);

	value = readl(OMAP4_CONTROL_PBIASLITE);
	value |= (OMAP4_MMC1_PBIASLITE_VMODE | OMAP4_MMC1_PBIASLITE_PWRDNZ |
			OMAP4_MMC1_PWRDNZ);
	writel(value, OMAP4_CONTROL_PBIASLITE);
}
EXPORT_SYMBOL(set_up_mmc_voltage_omap4);
