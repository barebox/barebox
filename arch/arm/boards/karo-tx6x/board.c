/*
 * Copyright (C) 2014 Steffen Trumtrar, Pengutronix
 *
 *
 * with the PMIC init code taken from u-boot
 * Copyright (C) 2012,2013 Lothar Waßmann <LW@KARO-electronics.de>
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

#define pr_fmt(fmt) "Karo-tx6: " fmt

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <i2c/i2c.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <environment.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include "pmic.h"

#define ETH_PHY_RST	IMX_GPIO_NR(7, 6)
#define ETH_PHY_PWR	IMX_GPIO_NR(3, 20)
#define ETH_PHY_INT	IMX_GPIO_NR(7, 1)

static void eth_init(void)
{
	void __iomem *iomux = (void *)MX6_IOMUXC_BASE_ADDR;
	uint32_t val;

	val = readl(iomux + IOMUXC_GPR1);
	val |= IMX6Q_GPR1_ENET_CLK_SEL_ANATOP;
	writel(val, iomux + IOMUXC_GPR1);
}

static struct {
	unsigned char addr;
	int (*init)(struct i2c_client *client);
	const char *name;
} i2c_addrs[] = {
	{ 0x3c, ltc3676_pmic_setup, "ltc3676" },
	{ 0x32, rn5t618_pmic_setup, "rn5t618" },
	{ 0x33, rn5t567_pmic_setup, "rn5t567" },
};

static int setup_pmic_voltages(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client client;
	int ret = -ENODEV;
	int i;
	int bus = 0;
	uint8_t reg;

	adapter = i2c_get_adapter(bus);
	if (!adapter) {
		pr_err("i2c bus %d not found\n", bus);
		return -ENODEV;
	}

	client.adapter = adapter;

	for (i = 0; i < ARRAY_SIZE(i2c_addrs); i++) {
		client.addr = i2c_addrs[i].addr;

		pr_debug("Probing for I2C dev 0x%02x\n", client.addr);

		ret = i2c_write_reg(&client, 0x00, &reg, 0);
		if (ret == 0) {
			pr_info("Detected %s PMIC\n", i2c_addrs[i].name);
			ret = i2c_addrs[i].init(&client);
			goto out;
		}
	}

	pr_err("No PMIC found\n");
out:
	if (ret)
		pr_err("PMIC setup failed with %s\n", strerror(-ret));

	return ret;
}

#define IMX6_SRC_SBMR1   0x04

static int tx6x_devices_init(void)
{
	void __iomem *src_base = IOMEM(MX6_SRC_BASE_ADDR);
	uint32_t sbmr1;

	if (!of_machine_is_compatible("karo,imx6dl-tx6dl") &&
	    !of_machine_is_compatible("karo,imx6q-tx6q"))
		return 0;

	barebox_set_hostname("tx6");

	eth_init();

	setup_pmic_voltages();

	sbmr1 = readl(src_base + IMX6_SRC_SBMR1);

	/*
	 * Check if this board is booted from eMMC or NAND to enable the
	 * corresponding device. We can't use the regular bootsource
	 * function here as it might return that we are in serial
	 * downloader mode. Even if we are SBMR1[7] indicates whether
	 * this board has eMMC or NAND.
	 */
	if (sbmr1 & (1 << 7)) {
		imx6_bbu_nand_register_handler("nand", BBU_HANDLER_FLAG_DEFAULT);
		of_device_enable_and_register_by_name("environment-nand");
		of_device_enable_and_register_by_name("gpmi-nand@00112000");
	} else {
		imx6_bbu_internal_mmc_register_handler("eMMC", "/dev/mmc3.boot0",
						       BBU_HANDLER_FLAG_DEFAULT);
		of_device_enable_and_register_by_name("environment-emmc");
		of_device_enable_and_register_by_name("usdhc@0219c000");
	}


	return 0;
}
device_initcall(tx6x_devices_init);
