/*
 * (C) Copyright 2012 Fadil Berisha, <fadil.r.berisha@gmail.com>
 *     based on falconwing.c & mx23-evk.c
 *
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 * (C) Copyright 2011 Wolfram Sang - Pengutronix
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
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <led.h>
#include <environment.h>
#include <envfs.h>
#include <errno.h>
#include <mci.h>
#include <asm/armlinux.h>
#include <asm/barebox-arm.h>
#include <usb/ehci.h>
#include <mach/usb.h>
#include <generated/mach-types.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>
#include <mach/mci.h>
#include <mach/iomux.h>
#include <generated/mach-types.h>

static struct mxs_mci_platform_data mci_pdata = {
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,	/* fixed to 3.3 V */
	.f_min = 400000,
};

static void olinuxino_init_usb(void)
{
	imx23_usb_phy_enable();

	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC, IMX_USB_BASE, NULL);
}

static struct gpio_led led1 = {
	.gpio = 65,
	.led = {
		.name = "led1",
	}
};

static int imx23_olinuxino_devices_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_IMX233_OLINUXINO)
		return 0;

	armlinux_set_architecture(MACH_TYPE_IMX233_OLINUXINO);
	defaultenv_append_directory(defaultenv_imx233_olinuxino);

	led_gpio_register(&led1);
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &led1.led);

	add_generic_device("mxs_mci", DEVICE_ID_DYNAMIC, NULL, IMX_SSP1_BASE,
					0x8000, IORESOURCE_MEM, &mci_pdata);

	olinuxino_init_usb();

	default_environment_path_set("/dev/disk0.1");

	return 0;
}

device_initcall(imx23_olinuxino_devices_init);

static int imx23_olinuxino_console_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_IMX233_OLINUXINO)
		return 0;

	barebox_set_model("Olimex.ltd imx233-olinuxino");
	barebox_set_hostname("imx233-olinuxino");

	add_generic_device("stm_serial", 0, NULL, IMX_DBGUART_BASE, 8192,
			   IORESOURCE_MEM, NULL);

	return 0;
}

console_initcall(imx23_olinuxino_console_init);
