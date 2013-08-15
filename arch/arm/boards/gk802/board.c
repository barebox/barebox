/*
 * Copyright (C) 2013 Philipp Zabel
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

#include <asm/armlinux.h>
#include <asm/io.h>
#include <bootsource.h>
#include <common.h>
#include <environment.h>
#include <envfs.h>
#include <gpio.h>
#include <init.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>
#include <mach/imx6.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <sizes.h>

#define GK802_GPIO_RECOVERY_BTN	IMX_GPIO_NR(3, 16)	/* recovery button */
#define GK802_GPIO_RTL8192_PDN	IMX_GPIO_NR(2, 0)	/* RTL8192CU powerdown */

static int gk802_env_init(void)
{
	char *bootsource_name;
	char *barebox_name;
	char *default_environment_name;

	if (!of_machine_is_compatible("zealz,imx6q-gk802"))
		return 0;

	/* Keep RTL8192CU disabled */
	gpio_direction_output(GK802_GPIO_RTL8192_PDN, 1);

	gpio_direction_input(GK802_GPIO_RECOVERY_BTN);
	setenv("recovery", gpio_get_value(GK802_GPIO_RECOVERY_BTN) ? "0" : "1");

	if (bootsource_get() != BOOTSOURCE_MMC)
		return 0;

	switch (bootsource_get_instance()) {
	case 2:
		bootsource_name = "mmc2";
		barebox_name = "mmc2.barebox";
		default_environment_name = "mmc2.bareboxenv";
		default_environment_path = "/dev/mmc2.bareboxenv";
		break;
	case 3:
		bootsource_name = "mmc3";
		barebox_name = "mmc3.barebox";
		default_environment_name = "mmc3.bareboxenv";
		default_environment_path = "/dev/mmc3.bareboxenv";
		break;
	default:
		return 0;
	}

	device_detect_by_name(bootsource_name);
	devfs_add_partition(bootsource_name, 0x00000, SZ_512K, DEVFS_PARTITION_FIXED, barebox_name);
	devfs_add_partition(bootsource_name, SZ_512K, SZ_512K, DEVFS_PARTITION_FIXED, default_environment_name);

	return 0;
}
late_initcall(gk802_env_init);

static int gk802_console_init(void)
{
	if (!of_machine_is_compatible("zealz,imx6q-gk802"))
		return 0;

	barebox_set_hostname("gk802");

	imx6_init_lowlevel();

	return 0;
}
postcore_initcall(gk802_console_init);
