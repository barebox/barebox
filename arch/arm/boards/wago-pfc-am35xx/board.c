// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2014 WAGO Kontakttechnik GmbH & Co. KG <http://global.wago.com>
 * Author: Heinrich Toews <heinrich.toews@wago.com>
 */
#define pr_fmt(fmt)	"pfc200: " fmt

#include <common.h>
#include <init.h>
#include <driver.h>
#include <gpio.h>
#include <linux/sizes.h>
#include <linux/err.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>
#include <asm/memory.h>
#include <mach/generic.h>

static int pfc200_mem_init(void)
{
	if (!of_machine_is_compatible("ti,pfc200"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, SZ_256M);
	return 0;
}
mem_initcall(pfc200_mem_init);

#define BMCR_HP_MDIX 0x20

static int pfc200_phy_fixup(struct mii_bus *mii, int phyadr)
{
	struct phy_device *phydev;
	int ret;

	phydev = mdiobus_scan(mii, phyadr);

	if (IS_ERR(phydev)) {
		pr_err("Cannot find phydev %d on mii bus\n", phyadr);
		return PTR_ERR(phydev);
	}

	ret = phy_write(phydev, MII_BMCR, BMCR_ANENABLE | BMCR_HP_MDIX);
	if (ret)
		pr_err("Failed to write to phy: %s\n", strerror(-ret));

	return ret;
}

static int pfc200_late_init(void)
{
	struct mii_bus *mii;

	if (!of_machine_is_compatible("ti,pfc200"))
		return 0;

	mii = mdiobus_get_bus(0);
	if (!mii) {
		pr_err("Cannot find mii bus 0\n");
		return -ENODEV;
	}

	pfc200_phy_fixup(mii, 1);
	pfc200_phy_fixup(mii, 2);

	return 0;
}
late_initcall(pfc200_late_init);


#define GPIO_KSZ886x_RESET	136

static int pfc200_devices_init(void)
{
	if (!of_machine_is_compatible("ti,pfc200"))
		return 0;

	gpio_direction_output(GPIO_KSZ886x_RESET, 1);

	omap_set_bootmmc_devname("mmc0");

	return 0;
}
coredevice_initcall(pfc200_devices_init);
