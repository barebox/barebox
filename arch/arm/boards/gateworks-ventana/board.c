/*
 * Copyright (C) 2014 Lucas Stach, Pengutronix
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
#include <environment.h>
#include <i2c/i2c.h>
#include <init.h>
#include <linux/marvell_phy.h>
#include <linux/phy.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <net.h>

#include "gsc.h"

static int gw54xx_wdog_of_fixup(struct device_node *root, void *context)
{
	struct device_node *np;

	/* switch to the watchdog with internal reset capabilities */
	np = of_find_node_by_name(root, "wdog@020c0000");
	of_device_disable(np);
	np = of_find_node_by_name(root, "wdog@020bc000");
	of_device_enable(np);

	return 0;
}

static int gw54xx_devices_init(void)
{
	struct i2c_client client;
	struct device_node *dnode;
	u8 reg;
	u8 mac[6];

	if (!of_machine_is_compatible("gw,imx6q-gw54xx"))
		return 0;

	client.adapter = i2c_get_adapter(0);
	if (!client.adapter) {
		pr_err("could not get system controller i2c bus\n");
		return -ENODEV;
	}

	/* disable the GSC boot watchdog */
	client.addr = GSC_SC_ADDR;
	gsc_i2c_read(&client, GSC_SC_CTRL1, &reg, 1);
	reg |= GSC_SC_CTRL1_WDDIS;
	gsc_i2c_write(&client, GSC_SC_CTRL1, &reg, 1);

	/* read MAC adresses from EEPROM and attach to eth devices */
	dnode = of_find_node_by_alias(of_get_root_node(), "ethernet0");
	if (dnode) {
		client.addr = GSC_EEPROM_ADDR;
		gsc_i2c_read(&client, 0x00, mac, 6);
		of_eth_register_ethaddr(dnode, mac);
	}
	dnode = of_find_node_by_alias(of_get_root_node(), "ethernet1");
	if (dnode) {
		client.addr = GSC_EEPROM_ADDR;
		gsc_i2c_read(&client, 0x06, mac, 6);
		of_eth_register_ethaddr(dnode, mac);
	}

	/* boards before rev E don't have the external watchdog signal */
	if (gsc_get_rev(&client) < 'E')
		of_register_fixup(gw54xx_wdog_of_fixup, NULL);

	imx6_bbu_nand_register_handler("nand", BBU_HANDLER_FLAG_DEFAULT);

	barebox_set_hostname("gw54xx");

	return 0;
}
device_initcall(gw54xx_devices_init);

static int marvell_88e1510_phy_fixup(struct phy_device *dev)
{
	u32 val;

	/* LED settings */
	phy_write(dev, 22, 3);
	val = phy_read(dev, 16);
	val &= 0xff00;
	val |= 0x0017;
	phy_write(dev, 16, val);
	phy_write(dev, 22, 0);

	return 0;
}

static int gw54xx_coredevices_init(void)
{
	if (!of_machine_is_compatible("gw,imx6q-gw54xx"))
		return 0;

	phy_register_fixup_for_uid(MARVELL_PHY_ID_88E1510, MARVELL_PHY_ID_MASK,
				   marvell_88e1510_phy_fixup);

	return 0;
}
coredevice_initcall(gw54xx_coredevices_init);
