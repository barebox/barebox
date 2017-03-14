/*
 * Copyright (C) 2014 Sascha Hauer <s.hauer@pengutronix.de>
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
#define pr_fmt(fmt) "Santaro: " fmt

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx6-regs.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/generic.h>
#include <linux/sizes.h>
#include <bootsource.h>
#include <bbu.h>
#include <mach/bbu.h>
#include <mach/imx6.h>
#include <i2c/i2c.h>
#include <gpio.h>

static int i2c_device_present(struct i2c_adapter *adapter, int addr)
{
	struct i2c_client client = {};
	u8 reg;

	client.adapter = adapter;
	client.addr = addr;

	return i2c_write_reg(&client, 0x00, &reg, 0) < 0 ? false : true;
}

#define TOUCH_RESET_GPIO	IMX_GPIO_NR(1, 20)

static int edt_present, egalax_present;

static int santaro_touch_fixup(struct device_node *root, void *unused)
{
	struct device_node *i2cnp, *np;

	i2cnp = of_find_node_by_alias(root, "i2c2");
	if (!i2cnp) {
		pr_err("Cannot find node i2c2\n");
		return -EINVAL;
	}

	for_each_child_of_node(i2cnp, np) {
		int present;

		if (of_device_is_compatible(np, "edt,edt-ft5206"))
			present = edt_present;
		else if (of_device_is_compatible(np, "eeti,egalax_ts"))
			present = egalax_present;
		else
			continue;

		if (present)
			of_device_enable(np);
		else
			of_device_disable(np);
	}

	return 0;
}

static int santaro_detect_touch(void)
{
	struct device_node *np;
	struct i2c_adapter *adapter;
	const char *model = NULL;

	if (!of_machine_is_compatible("guf,imx6q-santaro"))
		return 0;

	/*
	 * The Santaro has two different possible Touchscreen
	 * controllers. Both are on different I2C addresses.
	 * Let's probe both of them and enable in the device tree
	 * the one that's actually found on the hardware.
	 */

	np = of_find_node_by_alias(NULL, "i2c2");
	if (!np) {
		pr_err("Cannot find node i2c2\n");
		return -EINVAL;
	}

	adapter = of_find_i2c_adapter_by_node(np);
	if (!adapter) {
		pr_err("Cannot find i2c2 adapter\n");
		return -EINVAL;
	}

	gpio_direction_output(TOUCH_RESET_GPIO, 0);
	mdelay(10);
	gpio_set_value(TOUCH_RESET_GPIO, 1);
	mdelay(10);

	edt_present = i2c_device_present(adapter, 0x38);

	gpio_set_value(TOUCH_RESET_GPIO, 0);
	mdelay(10);

	egalax_present = i2c_device_present(adapter, 0x4);

	if (edt_present)
		model = "edt,edt-ft5206";
	if (egalax_present)
		model = "eeti,egalax_ts";

	pr_info("Found %s Touchscreen controller\n", model);

	of_register_fixup(santaro_touch_fixup, NULL);

	return 0;
}
late_initcall(santaro_detect_touch);

static int santaro_device_init(void)
{
	uint32_t flag_sd = 0, flag_emmc = 0;

	if (!of_machine_is_compatible("guf,imx6q-santaro"))
		return 0;

	barebox_set_hostname("santaro");

	writew(0x0, MX6_WDOG1_BASE_ADDR + 0x8);
	writew(0x0, MX6_WDOG2_BASE_ADDR + 0x8);

	if (bootsource_get() == BOOTSOURCE_MMC) {
		switch (bootsource_get_instance()) {
		case 1:
			flag_sd |= BBU_HANDLER_FLAG_DEFAULT;
			break;
		case 3:
			flag_emmc |= BBU_HANDLER_FLAG_DEFAULT;
			break;
		}
	}

	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc1", flag_sd);
	imx6_bbu_internal_mmc_register_handler("emmc", "/dev/mmc3.boot0", flag_emmc);

	return 0;
}
device_initcall(santaro_device_init);
