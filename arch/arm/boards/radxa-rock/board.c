/*
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <i2c/i2c.h>
#include <i2c/i2c-gpio.h>
#include <mach/rockchip-pll.h>
#include <mfd/act8846.h>

static struct i2c_board_info radxa_rock_i2c_devices[] = {
	{
		I2C_BOARD_INFO("act8846", 0x5a)
	},
};

static struct i2c_gpio_platform_data i2c_gpio_pdata = {
	.sda_pin		= 58,
	.scl_pin		= 59,
	.udelay			= 5,
};

static void radxa_rock_pmic_init(void)
{
	struct act8846 *pmic;

	pmic = act8846_get();
	if (pmic == NULL)
		return;

	/* Power on ethernet PHY */
	act8846_set_bits(pmic, ACT8846_LDO9_CTRL, BIT(7), BIT(7));
}

static int setup_plls(void)
{
	if (!of_machine_is_compatible("radxa,rock"))
		return 0;

	/* Codec PLL frequency: 594 MHz */
	rk3188_pll_set_parameters(RK3188_CPLL, 2, 198, 4);
	/* General PLL frequency: 300 MHz */
	rk3188_pll_set_parameters(RK3188_GPLL, 1, 50, 4);

	return 0;
}
coredevice_initcall(setup_plls);

static int devices_init(void)
{
	if (!of_machine_is_compatible("radxa,rock"))
		return 0;

	i2c_register_board_info(0, radxa_rock_i2c_devices,
				ARRAY_SIZE(radxa_rock_i2c_devices));
	add_generic_device_res("i2c-gpio", 0, NULL, 0, &i2c_gpio_pdata);

	radxa_rock_pmic_init();

	/* Set mac_pll divisor to 6 (50MHz output) */
	writel((5 << 8) | (0x1f << 24), 0x20000098);

	return 0;
}
device_initcall(devices_init);

static int hostname_init(void)
{
	if (!of_machine_is_compatible("radxa,rock"))
		return 0;

	barebox_set_hostname("radxa-rock");

	return 0;
}
postcore_initcall(hostname_init);
