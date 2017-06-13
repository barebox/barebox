/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Raghavendra KH <r-khandenahally@ti.com>
 *
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
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

/**
 * @file
 * @brief BeagleBone Specific Board Initialization routines
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <envfs.h>
#include <environment.h>
#include <globalvar.h>
#include <linux/sizes.h>
#include <net.h>
#include <envfs.h>
#include <bootsource.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/am33xx-silicon.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/gpmc.h>
#include <linux/err.h>
#include <mach/bbu.h>

#include "beaglebone.h"

static int beaglebone_coredevice_init(void)
{
	if (!of_machine_is_compatible("ti,am335x-bone"))
		return 0;

	am33xx_register_ethaddr(0, 0);
	return 0;
}
coredevice_initcall(beaglebone_coredevice_init);

static int beaglebone_mem_init(void)
{
	uint32_t sdram_size;

	if (!of_machine_is_compatible("ti,am335x-bone"))
		return 0;

	if (is_beaglebone_black())
		sdram_size = SZ_512M;
	else
		sdram_size = SZ_256M;

	arm_add_mem_device("ram0", 0x80000000, sdram_size);
	return 0;
}
mem_initcall(beaglebone_mem_init);

static int beaglebone_devices_init(void)
{
	int black;

	if (!of_machine_is_compatible("ti,am335x-bone"))
		return 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 0)
			omap_set_bootmmc_devname("mmc0");
		else
			omap_set_bootmmc_devname("mmc1");
	}

	black = is_beaglebone_black();

	defaultenv_append_directory(defaultenv_beaglebone);

	globalvar_add_simple("board.variant", black ? "boneblack" : "bone");

	printf("detected 'BeagleBone %s'\n", black ? "Black" : "White");

	armlinux_set_architecture(MACH_TYPE_BEAGLEBONE);

	/* Register update handler */
	am33xx_bbu_emmc_mlo_register_handler("MLO.emmc", "/dev/mmc1");

	if (IS_ENABLED(CONFIG_SHELL_NONE))
		return am33xx_of_register_bootdevice();

	return 0;
}
coredevice_initcall(beaglebone_devices_init);
