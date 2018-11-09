/*
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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
#include <envfs.h>
#include <fs.h>
#include <gpio.h>
#include <i2c/i2c.h>
#include <init.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <net.h>
#include <linux/nvmem-consumer.h>

#define RDU2_DAC1_RESET	IMX_GPIO_NR(1, 0)
#define RDU2_DAC2_RESET	IMX_GPIO_NR(1, 2)
#define RDU2_RST_TOUCH	IMX_GPIO_NR(1, 7)
#define RDU2_NFC_RESET	IMX_GPIO_NR(1, 17)
#define RDU2_HPA1_SDn	IMX_GPIO_NR(1, 4)
#define RDU2_HPA2_SDn	IMX_GPIO_NR(1, 5)

static const struct gpio rdu2_reset_gpios[] = {
	{
		.gpio = RDU2_DAC1_RESET,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "dac1-reset",
	},
	{
		.gpio = RDU2_DAC2_RESET,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "dac2-reset",
	},
	{
		.gpio = RDU2_RST_TOUCH,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "rst-touch#",
	},
	{
		.gpio = RDU2_NFC_RESET,
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "nfc-reset",
	},
	{
		.gpio = RDU2_HPA1_SDn,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "hpa1-sd-n",
	},
	{
		.gpio = RDU2_HPA2_SDn,
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "hpa2n-sd-n",
	},
};

static int rdu2_reset_audio_touchscreen_nfc(void)
{
	int ret;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	ret = gpio_request_array(rdu2_reset_gpios,
				 ARRAY_SIZE(rdu2_reset_gpios));
	if (ret) {
		pr_err("Failed to request RDU2 reset gpios: %s\n",
		       strerror(-ret));
		return ret;
	}

	mdelay(100);

	gpio_direction_output(RDU2_DAC1_RESET, 1);
	gpio_direction_output(RDU2_DAC2_RESET, 1);
	gpio_direction_output(RDU2_RST_TOUCH,  1);
	gpio_direction_output(RDU2_NFC_RESET,  0);
	gpio_direction_output(RDU2_HPA1_SDn,   1);
	gpio_direction_output(RDU2_HPA2_SDn,   1);

	mdelay(100);

	return 0;
}
/*
 * When this function is called "hog" pingroup in device tree needs to
 * be already initialized
 */
late_initcall(rdu2_reset_audio_touchscreen_nfc);

static int rdu2_devices_init(void)
{
	struct i2c_client client;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	client.adapter = i2c_get_adapter(1);
	if (client.adapter) {
		u8 reg;

		/*
		 * Reset PMIC SW1AB and SW1C rails to 1.375V. If an event
		 * caused only the i.MX6 SoC reset, the PMIC might still be
		 * stuck on the low voltage for the slow operating point.
		 */
		client.addr = 0x08; /* PMIC i2c address */
		reg = 0x2b; /* 1.375V, valid for both rails */
		i2c_write_reg(&client, 0x20, &reg, 1);
		i2c_write_reg(&client, 0x2e, &reg, 1);
	}

	barebox_set_hostname("rdu2");

	imx6_bbu_internal_spi_i2c_register_handler("SPI", "/dev/m25p0.barebox",
						   BBU_HANDLER_FLAG_DEFAULT);

	imx6_bbu_internal_mmcboot_register_handler("eMMC", "/dev/mmc3", 0);

	defaultenv_append_directory(defaultenv_rdu2);

	return 0;
}
device_initcall(rdu2_devices_init);

static int rdu2_eth_register_ethaddr(struct device_node *np)
{
	u8 mac[ETH_ALEN];
	u8 *data;
	int i;

	data = nvmem_cell_get_and_read(np, "mac-address", ETH_ALEN);
	if (IS_ERR(data))
		return PTR_ERR(data);
	/*
	 * EEPROM stores MAC address in reverse (to what we expect it
	 * to be) byte order.
	 */
	for (i = 0; i < ETH_ALEN; i++)
		mac[i] = data[ETH_ALEN - i - 1];

	free(data);

	of_eth_register_ethaddr(np, mac);

	return 0;
}

static int rdu2_ethernet_init(void)
{
	const char *aliases[] = { "ethernet0", "ethernet1" };
	struct device_node *np, *root;
	int i, ret;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	root = of_get_root_node();

	for (i = 0; i < ARRAY_SIZE(aliases); i++) {
		const char *alias = aliases[i];

		np = of_find_node_by_alias(root, alias);
		if (!np) {
			pr_warn("Failed to find %s\n", alias);
			continue;
		}

		ret = rdu2_eth_register_ethaddr(np);
		if (ret) {
			pr_warn("Failed to register MAC for %s\n", alias);
			continue;
		}
	}

	return 0;
}
late_initcall(rdu2_ethernet_init);

#define I210_CFGWORD_PCIID_157B	0x157b1a11
static int rdu2_i210_invm(void)
{
	int fd;
	u32 val;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return 0;

	fd = open("/dev/e1000-invm0", O_RDWR);
	if (fd < 0) {
		pr_err("could not open e1000 iNVM device!\n");
		return fd;
	}

	pread(fd, &val, sizeof(val), 0);
	if (val == I210_CFGWORD_PCIID_157B) {
		pr_debug("i210 already programmed correctly\n");
		return 0;
	}

	val = I210_CFGWORD_PCIID_157B;
	pwrite(fd, &val, sizeof(val), 0);

	return 0;
}
late_initcall(rdu2_i210_invm);
