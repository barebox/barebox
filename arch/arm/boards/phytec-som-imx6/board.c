/*
 * Copyright (C) 2013 Sascha Hauer, Pengutronix
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH,
 * Author: Stefan Christ <s.christ@phytec.de>
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
#define pr_fmt(fmt) "phySOM-i.MX6: " fmt

#include <malloc.h>
#include <envfs.h>
#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <gpio.h>
#include <init.h>
#include <of.h>
#include <i2c/i2c.h>
#include <mach/bbu.h>
#include <platform_data/eth-fec.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <linux/micrel_phy.h>

#include <globalvar.h>

#include <linux/micrel_phy.h>

#include <mach/iomux-mx6.h>
#include <mach/imx6.h>

#define PHYFLEX_MODULE_REV_1	0x1
#define PHYFLEX_MODULE_REV_2	0x2

#define GPIO_2_11_PD_CTL	MX6_PAD_CTL_PUS_100K_DOWN | MX6_PAD_CTL_PUE | MX6_PAD_CTL_PKE | \
				MX6_PAD_CTL_SPEED_MED | MX6_PAD_CTL_DSE_40ohm | MX6_PAD_CTL_HYS

#define MX6Q_PAD_SD4_DAT3__GPIO_2_11_PD (_MX6Q_PAD_SD4_DAT3__GPIO_2_11 | MUX_PAD_CTRL(GPIO_2_11_PD_CTL))
#define MX6DL_PAD_SD4_DAT3__GPIO_2_11 IOMUX_PAD(0x0734, 0x034C, 5, 0x0000, 0, GPIO_2_11_PD_CTL)

#define MX6_PHYFLEX_ERR006282	IMX_GPIO_NR(2, 11)

#define DA9062_I2C_ADDRESS		0x58

#define DA9062_BUCK1_CFG		0x9e
#define DA9062_BUCK2_CFG		0x9d
#define DA9062_BUCK3_CFG		0xa0
#define DA9062_BUCK4_CFG		0x9f
#define DA9062_BUCKx_MODE_SYNCHRONOUS	(2 << 6)

static void phyflex_err006282_workaround(void)
{
	/*
	 * Boards beginning with 1362.2 have the SD4_DAT3 pin connected
	 * to the CMIC. If this pin isn't toggled within 10s the boards
	 * reset. The pin is unconnected on older boards, so we do not
	 * need a check for older boards before applying this fixup.
	 */

	gpio_direction_output(MX6_PHYFLEX_ERR006282, 0);
	mdelay(2);
	gpio_direction_output(MX6_PHYFLEX_ERR006282, 1);
	mdelay(2);
	gpio_set_value(MX6_PHYFLEX_ERR006282, 0);

	if (cpu_is_mx6q() || cpu_is_mx6d() || cpu_is_mx6qp() || cpu_is_mx6dp())
		mxc_iomux_v3_setup_pad(MX6Q_PAD_SD4_DAT3__GPIO_2_11_PD);
	else if (cpu_is_mx6dl() || cpu_is_mx6s())
		mxc_iomux_v3_setup_pad(MX6DL_PAD_SD4_DAT3__GPIO_2_11);

	gpio_direction_input(MX6_PHYFLEX_ERR006282);
}

static unsigned int pfla02_module_revision;

static unsigned int get_module_rev(void)
{
	unsigned int val = 0;

	val = gpio_get_value(IMX_GPIO_NR(2, 12));
	val |= (gpio_get_value(IMX_GPIO_NR(2, 13)) << 1);
	val |= (gpio_get_value(IMX_GPIO_NR(2, 14)) << 2);
	val |= (gpio_get_value(IMX_GPIO_NR(2, 15)) << 3);

	return 16 - val;
}

int ksz8081_phy_fixup(struct phy_device *phydev)
{
	phy_write(phydev, 0x1f, 0x8190);
	phy_write(phydev, 0x16, 0x202);

	return 0;
}

static int phycore_da9062_setup_buck_mode(void)
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	unsigned char value;
	int bus = 0;
	int ret;

	adapter = i2c_get_adapter(bus);
	if (!adapter)
		return -ENODEV;

	client.adapter = adapter;
	client.addr = DA9062_I2C_ADDRESS;

	value = DA9062_BUCKx_MODE_SYNCHRONOUS;

	ret = i2c_write_reg(&client, DA9062_BUCK1_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	ret = i2c_write_reg(&client, DA9062_BUCK2_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	ret = i2c_write_reg(&client, DA9062_BUCK3_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	ret = i2c_write_reg(&client, DA9062_BUCK4_CFG, &value, 1);
	if (ret != 1)
		goto err_out;

	return 0;

err_out:
	return ret;
}

static int physom_imx6_devices_init(void)
{
	int ret;
	char *environment_path, *default_environment_path;
	char *envdev, *default_envdev;

	if (of_machine_is_compatible("phytec,imx6q-pfla02")
		|| of_machine_is_compatible("phytec,imx6dl-pfla02")
		|| of_machine_is_compatible("phytec,imx6s-pfla02")) {

		phyflex_err006282_workaround();

		pfla02_module_revision = get_module_rev();
		globalvar_add_simple_int("board.revision", &pfla02_module_revision, "%u");
		pr_info("Module Revision: %u\n", pfla02_module_revision);

		barebox_set_hostname("phyFLEX-i.MX6");
		default_environment_path = "/chosen/environment-spinor";
		default_envdev = "SPI NOR flash";

	} else if (of_machine_is_compatible("phytec,imx6q-pcaaxl3")) {

		barebox_set_hostname("phyCARD-i.MX6");
		default_environment_path = "/chosen/environment-nand";
		default_envdev = "NAND flash";

	} else if (of_machine_is_compatible("phytec,imx6q-pcm058-nand")
		|| of_machine_is_compatible("phytec,imx6q-pcm058-emmc")
		|| of_machine_is_compatible("phytec,imx6dl-pcm058-nand")
		|| of_machine_is_compatible("phytec,imx6qp-pcm058-nand")
		|| of_machine_is_compatible("phytec,imx6dl-pcm058-emmc")) {

		if (phycore_da9062_setup_buck_mode())
			pr_err("Setting PMIC BUCK mode failed\n");

		barebox_set_hostname("phyCORE-i.MX6");
		default_environment_path = "/chosen/environment-spinor";
		default_envdev = "SPI NOR flash";

	} else if (of_machine_is_compatible("phytec,imx6ul-pcl063")) {
		barebox_set_hostname("phyCORE-i.MX6UL");
		default_environment_path = "/chosen/environment-nand";
		default_envdev = "NAND flash";

		phy_register_fixup_for_uid(PHY_ID_KSZ8081, MICREL_PHY_ID_MASK,
				ksz8081_phy_fixup);

	} else
		return 0;

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		environment_path = basprintf("/chosen/environment-sd%d",
					       bootsource_get_instance() + 1);
		envdev = "MMC";
		break;
	case BOOTSOURCE_NAND:
		environment_path = basprintf("/chosen/environment-nand");
		envdev = "NAND flash";
		break;
	case BOOTSOURCE_SPI_NOR:
		environment_path = basprintf("/chosen/environment-spinor");
		envdev = "SPI NOR flash";
		break;
	default:
		environment_path = basprintf(default_environment_path);
		envdev = default_envdev;
		break;
	}

	if (environment_path) {
		ret = of_device_enable_path(environment_path);
		if (ret < 0)
			pr_warn("Failed to enable environment partition '%s' (%d)\n",
				environment_path, ret);
		free(environment_path);
	}

	pr_notice("Using environment in %s\n", envdev);

	if (of_machine_is_compatible("phytec,imx6q-pcm058-emmc")
		|| of_machine_is_compatible("phytec,imx6dl-pcm058-emmc")) {
		imx6_bbu_internal_mmc_register_handler("mmc3",
						"/dev/mmc3",
						BBU_HANDLER_FLAG_DEFAULT);
	} else {
		imx6_bbu_nand_register_handler("nand", BBU_HANDLER_FLAG_DEFAULT);
	}

	defaultenv_append_directory(defaultenv_physom_imx6);

	/* Overwrite file /env/init/automount */
	if (of_machine_is_compatible("phytec,imx6qp-pcm058-nand")
		|| of_machine_is_compatible("phytec,imx6q-pcm058-nand")
		|| of_machine_is_compatible("phytec,imx6q-pcm058-emmc")
		|| of_machine_is_compatible("phytec,imx6dl-pcm058-nand")
		|| of_machine_is_compatible("phytec,imx6dl-pcm058-emmc")) {
		defaultenv_append_directory(defaultenv_physom_imx6_phycore);
	} else if (of_machine_is_compatible("phytec,imx6ul-pcl063")) {
		defaultenv_append_directory(defaultenv_physom_imx6ul_phycore);
	}

	return 0;
}
device_initcall(physom_imx6_devices_init);
