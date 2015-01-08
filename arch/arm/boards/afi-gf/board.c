/*
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <string.h>
#include <init.h>
#include <linux/sizes.h>
#include <envfs.h>
#include <bootsource.h>
#include <asm/armlinux.h>
#include <mach/bbu.h>
#include <mach/am33xx-generic.h>

static int board_console_init(void)
{
	if (!of_machine_is_compatible("afi,gf"))
		return 0;

	switch (bootsource_get()) {
	default:
	case BOOTSOURCE_SPI:
		of_device_enable_path("/chosen/environment-spi");
		break;
	case BOOTSOURCE_MMC:
		omap_set_bootmmc_devname("mmc0");
		break;
	}

	defaultenv_append_directory(defaultenv_gf);
	am33xx_register_ethaddr(0, 0);
	am33xx_register_ethaddr(1, 1);
	barebox_set_hostname("gf");
	am33xx_bbu_spi_nor_mlo_register_handler("MLO.spi", "/dev/m25p0.mlo");
	am33xx_bbu_spi_nor_register_handler("spi", "/dev/m25p0.boot");

	return 0;
}
coredevice_initcall(board_console_init);
