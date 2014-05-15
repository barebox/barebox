/*
 * pcm051 - phyCORE-AM335x Board Initalization Code
 *
 * Copyright (C) 2012 Teresa Gámez, Phytec Messtechnik GmbH
 *
 * Based on arch/arm/boards/omap/board-beagle.c
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

#include <bootsource.h>
#include <common.h>
#include <nand.h>
#include <init.h>
#include <io.h>
#include <sizes.h>
#include <envfs.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <linux/phy.h>
#include <mach/am33xx-generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/bbu.h>


static int pcm051_coredevice_init(void)
{
	if (!of_machine_is_compatible("phytec,pcm051"))
		return 0;

	am33xx_register_ethaddr(0, 0);
	return 0;
}
coredevice_initcall(pcm051_coredevice_init);

static struct omap_barebox_part pcm051_barebox_part = {
	.nand_offset = SZ_512K,
	.nand_size = SZ_512K,
	.nor_offset = SZ_128K,
	.nor_size = SZ_512K,
};

static int pcm051_devices_init(void)
{
	if (!of_machine_is_compatible("phytec,pcm051"))
		return 0;

	switch (bootsource_get()) {
	case BOOTSOURCE_SPI:
		of_device_enable_path("/chosen/environment-spi");
		break;
	case BOOTSOURCE_MMC:
		omap_set_bootmmc_devname("mmc0");
		break;
	default:
		of_device_enable_path("/chosen/environment-nand");
		break;
	}

	omap_set_barebox_part(&pcm051_barebox_part);
	armlinux_set_architecture(MACH_TYPE_PCM051);
	defaultenv_append_directory(defaultenv_phycore_am335x);

	am33xx_bbu_spi_nor_mlo_register_handler("MLO.spi", "/dev/m25p0.xload");

	return 0;
}
device_initcall(pcm051_devices_init);
