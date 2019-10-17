/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <kernel@pengutronix.de>
 * Copyright (C) 2011 Marc Kleine-Budde, Pengutronix <mkl@pengutronix.de>
 * Copyright (C) 2011 Wolfram Sang, Pengutronix <w.sang@pengutronix.de>
 * Copyright (C) 2012 Maxime Ripard, Free Electrons <maxime.ripard@free-electrons.com>
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
#include <envfs.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <mci.h>
#include <io.h>
#include <net.h>
#include <linux/sizes.h>

#include <i2c/i2c.h>
#include <i2c/i2c-gpio.h>
#include <i2c/at24.h>

#include <mach/clock.h>
#include <mach/imx-regs.h>
#include <mach/iomux.h>
#include <mach/mci.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>
#include <asm/barebox-arm.h>

#include <mach/fb.h>

#include <generated/mach-types.h>

#include "hwdetect.h"

/* setup the CPU card internal signals */
static const uint32_t cfa10036_pads[] = {
	/* duart */
	FUNC(2) | PORTF(3, 2) | VE_3_3V,
	FUNC(2) | PORTF(3, 3) | VE_3_3V,

	/* mmc0 */
	SSP0_D0 | VE_3_3V | PULLUP(1),
	SSP0_D1 | VE_3_3V | PULLUP(1),
	SSP0_D2 | VE_3_3V | PULLUP(1),
	SSP0_D3 | VE_3_3V | PULLUP(1),
	SSP0_D4 | VE_3_3V | PULLUP(1),
	SSP0_D5 | VE_3_3V | PULLUP(1),
	SSP0_D6 | VE_3_3V | PULLUP(1),
	SSP0_D7 | VE_3_3V | PULLUP(1),
	SSP0_CMD | VE_3_3V | PULLUP(1),
	SSP0_CD | VE_3_3V | PULLUP(1),
	SSP0_SCK | VE_3_3V | BITKEEPER(0),
	/* MCI slot power control 1 = off */
	PWM3_GPIO | VE_3_3V | GPIO_OUT | GPIO_VALUE(0),

	/* i2c0 */
	AUART0_TX_GPIO | VE_3_3V | PULLUP(1),
	AUART0_RX_GPIO | VE_3_3V | PULLUP(1),
};

static struct mxs_mci_platform_data mci_pdata = {
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
	.f_min = 400 * 1000,
	.f_max = 25000000,
};

static struct i2c_board_info cfa10036_i2c_devices[] = {
	{
		I2C_BOARD_INFO("24c02", 0x50)
	},
};

static struct i2c_gpio_platform_data i2c_gpio_pdata = {
	.sda_pin		= 3 * 32 + 1,
	.scl_pin		= 3 * 32 + 0,
	.udelay			= 5,		/* ~100 kHz */
};

static int cfa10036_devices_init(void)
{
	int i;

	if (barebox_arm_machine() != MACH_TYPE_CFA10036)
		return 0;

	/* initizalize muxing */
	for (i = 0; i < ARRAY_SIZE(cfa10036_pads); i++)
		imx_gpio_mode(cfa10036_pads[i]);

	armlinux_set_architecture(MACH_TYPE_CFA10036);

	add_generic_device("mxs_mci", 0, NULL, IMX_SSP0_BASE, SZ_8K,
			   IORESOURCE_MEM, &mci_pdata);

	i2c_register_board_info(0, cfa10036_i2c_devices, ARRAY_SIZE(cfa10036_i2c_devices));
	add_generic_device_res("i2c-gpio", 0, NULL, 0, &i2c_gpio_pdata);

	cfa10036_detect_hw();

	default_environment_path_set("/dev/disk0.1");

	return 0;
}
device_initcall(cfa10036_devices_init);

static int cfa10036_console_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_CFA10036)
		return 0;

	barebox_set_model("crystalfontz-cfa10036");
	barebox_set_hostname("cfa10036");

	add_generic_device("stm_serial", 0, NULL, IMX_DBGUART_BASE, SZ_8K,
			   IORESOURCE_MEM, NULL);

	return 0;
}
console_initcall(cfa10036_console_init);
