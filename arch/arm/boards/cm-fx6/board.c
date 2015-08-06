/*
 * Copyright (C) 2015 Sascha Hauer, Pengutronix
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

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx6-regs.h>
#include <mach/bbu.h>
#include <asm/armlinux.h>
#include <linux/phy.h>
#include <mach/generic.h>
#include <linux/sizes.h>
#include <mach/imx6.h>
#include <net.h>

static int phy_fixup(struct phy_device *phydev)
{
	unsigned short val;

	/* Ar8031 phy SmartEEE feature cause link status generates glitch,
	 * which cause ethernet link down/up issue, so disable SmartEEE
	 */
	phy_write(phydev, 0xd, 0x3);
	phy_write(phydev, 0xe, 0x805d);
	phy_write(phydev, 0xd, 0x4003);
	val = phy_read(phydev, 0xe);
	val &= ~(0x1 << 8);
	phy_write(phydev, 0xe, val);

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x8016);
	phy_write(phydev, 0xd, 0x4007);

	val = phy_read(phydev, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(phydev, 0xe, val);

	/* introduce tx clock delay */
	phy_write(phydev, 0x1d, 0x5);
	val = phy_read(phydev, 0x1e);
	val |= 0x0100;
	phy_write(phydev, 0x1e, val);

	return 0;
}

#define PHY_ID_AR8031	0x004dd074

static int cm_fx6_eeprom_init(void)
{
	struct cdev *cdev;
	u8 mac[6];
	int ret;

	if (!of_machine_is_compatible("compulab,cm-fx6"))
		return 0;

	cdev = cdev_by_name("eeprom0");
	if (!cdev)
		return -ENODEV;

	ret = cdev_read(cdev, mac, 6, 4, 0);
	if (ret < 0)
		return ret;

	eth_register_ethaddr(0, mac);

	return 0;
}
late_initcall(cm_fx6_eeprom_init);

static int cm_fx6_devices_init(void)
{
	if (!of_machine_is_compatible("compulab,cm-fx6"))
		return 0;

	if (IS_ENABLED(CONFIG_PHYLIB))
		phy_register_fixup_for_uid(PHY_ID_AR8031, 0xffffffff, phy_fixup);

	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0",
		BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
coredevice_initcall(cm_fx6_devices_init);
