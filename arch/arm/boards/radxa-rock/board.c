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
#include <mach/rk3188-regs.h>
#include <mfd/act8846.h>
#include <asm/armlinux.h>

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

static int devices_init(void)
{
	if (!of_machine_is_compatible("radxa,rock"))
		return 0;

	i2c_register_board_info(0, radxa_rock_i2c_devices,
				ARRAY_SIZE(radxa_rock_i2c_devices));
	add_generic_device_res("i2c-gpio", 0, NULL, 0, &i2c_gpio_pdata);

	radxa_rock_pmic_init();

	armlinux_set_architecture(3066);

	/* Map SRAM to address 0, kernel relies on this */
	writel((RK_SOC_CON0_REMAP << 16) | RK_SOC_CON0_REMAP,
	    RK_GRF_BASE + RK_GRF_SOC_CON0);

	return 0;
}
device_initcall(devices_init);
