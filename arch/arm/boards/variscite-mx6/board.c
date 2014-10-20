/*
 * Copyright (C) 2013 Michael Burkey
 * Based on code (C) Sascha Hauer, Pengutronix
 * Based on code (C) Variscite, Ltd.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */

#define pr_fmt(fmt) "var-som-mx6: " fmt

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <of.h>
#include <debug_ll.h>

#include <environment.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <sizes.h>
#include <mach/imx6.h>
#include <mach/devices-imx6.h>
#include <mach/iomux-mx6.h>
#include <spi/spi.h>
#include <mach/spi.h>
#include <i2c/i2c.h>

#define ETH_PHY_RST	IMX_GPIO_NR(1, 25)

static int setup_pmic_voltages(void)
{
	unsigned char value, rev_id = 0 ;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int addr = -1, bus = 0;

	/* I2C2 bus (2-1 = 1 in barebox numbering) */
	bus = 1;

	/* PFUZE100 device address is 0x08 */
	addr = 0x08;

	adapter = i2c_get_adapter(bus);
	if (!adapter) {
		pr_err("i2c bus %d not found\n", bus);
		return -ENODEV;
	}

	client.adapter = adapter;
	client.addr = addr;

	/* Attempt to locate the PFUZE100 chip. */
	if (i2c_read_reg(&client, 0x00, &value, 1) != 1) {
		pr_err("Read device ID error!\n");
		return -1;
	}
	if (i2c_read_reg(&client, 0x03, &rev_id, 1) != 1) {
		pr_err("Read Rev ID error!\n");
		return -1;
	}

	pr_info("Found PFUZE100! deviceid=%x,revid=%x\n", value, rev_id);

	/* Set Gigabit Ethernet voltage (SOM v1.1/1.0)*/
        value = 0x60;
	if (i2c_write_reg(&client, 0x4a, &value, 1) != 1) {
		pr_err("Set ETH error!\n");
		return -EIO;
	}

	/* set VGEN3 to 2.5V */
	value = 0x77;
	if (i2c_write_reg(&client, 0x6e, &value, 1) != 1) {
		pr_err("Set VGEN3 error!\n");
		return -EIO;
	}

	return 0;
}

static int eth_phy_reset(void)
{
	gpio_request(ETH_PHY_RST, "phy reset");
	gpio_direction_output(ETH_PHY_RST, 0);
	mdelay(1);
	gpio_set_value(ETH_PHY_RST, 1);

	return 0;
}

static int variscite_custom_init(void)
{
	if (!of_machine_is_compatible("variscite,imx6q-custom"))
		return 0;

	barebox_set_hostname("var-som-mx6");

	setup_pmic_voltages();

	eth_phy_reset();

	armlinux_set_architecture(MACH_TYPE_VAR_SOM_MX6);

	pr_debug("Completing custom_init()\n");

	return 0;
}
device_initcall(variscite_custom_init);
