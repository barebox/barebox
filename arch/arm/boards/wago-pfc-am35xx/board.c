// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2014 WAGO Kontakttechnik GmbH & Co. KG <http://global.wago.com>
 * Author: Heinrich Toews <heinrich.toews@wago.com>
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <gpio.h>
#include <linux/sizes.h>
#include <linux/err.h>
#include <asm/memory.h>
#include <mach/generic.h>

static int pfc200_mem_init(void)
{
	if (!of_machine_is_compatible("ti,pfc200"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, SZ_256M);
	return 0;
}
mem_initcall(pfc200_mem_init);

#define GPIO_KSZ886x_RESET	136

static int pfc200_devices_init(void)
{
	if (!of_machine_is_compatible("ti,pfc200"))
		return 0;

	gpio_direction_output(GPIO_KSZ886x_RESET, 1);

	omap_set_bootmmc_devname("mmc0");

	return 0;
}
coredevice_initcall(pfc200_devices_init);
