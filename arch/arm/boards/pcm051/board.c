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
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <linux/phy.h>
#include <mach/am33xx-generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/bbu.h>

static struct omap_barebox_part pcm051_barebox_part = {
	.nand_offset = SZ_512K,
	.nand_size = SZ_512K,
	.nor_offset = SZ_128K,
	.nor_size = SZ_512K,
};

static int pcm051_devices_init(void)
{
	if (!of_machine_is_compatible("phytec,phycore-am335x"))
		return 0;

	am33xx_register_ethaddr(0, 0);
	writel(0x69, AM33XX_MAC_MII_SEL);

	switch (bootsource_get()) {
	case BOOTSOURCE_SPI:
		devfs_add_partition("m25p0", 0x00000, SZ_128K,
					DEVFS_PARTITION_FIXED, "xload");
		devfs_add_partition("m25p0", SZ_128K, SZ_512K,
					DEVFS_PARTITION_FIXED, "self0");
		devfs_add_partition("m25p0", SZ_128K + SZ_512K, SZ_128K,
					DEVFS_PARTITION_FIXED, "env0");
		break;
	case BOOTSOURCE_MMC:
		omap_set_bootmmc_devname("mmc0");
		break;
	default:
		devfs_add_partition("nand0", 0x00000, SZ_128K,
					DEVFS_PARTITION_FIXED, "xload_raw");
		dev_add_bb_dev("xload_raw", "xload");
		devfs_add_partition("nand0", SZ_512K, SZ_512K,
					DEVFS_PARTITION_FIXED, "self_raw");
		dev_add_bb_dev("self_raw", "self0");
		devfs_add_partition("nand0", SZ_512K + SZ_512K, SZ_128K,
					DEVFS_PARTITION_FIXED, "env_raw");
		dev_add_bb_dev("env_raw", "env0");
		break;
	}

	omap_set_barebox_part(&pcm051_barebox_part);
	armlinux_set_bootparams((void *)(AM33XX_DRAM_ADDR_SPACE_START + 0x100));
	armlinux_set_architecture(MACH_TYPE_PCM051);

	am33xx_bbu_spi_nor_mlo_register_handler("MLO.spi", "/dev/m25p0.xload");

	return 0;
}
device_initcall(pcm051_devices_init);
