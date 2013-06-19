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

#include <common.h>
#include <init.h>
#include <sizes.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/am33xx-devices.h>
#include <mach/am33xx-mux.h>
#include <mach/am33xx-silicon.h>
#include <spi/spi.h>
#include <spi/flash.h>

#include "mux.h"

/**
 * @brief UART serial port initialization
 * arch
 *
 * @return result of device registration
 */
static int pcm051_console_init(void)
{
	/* Register the serial port */
	am33xx_add_uart0();

	return 0;
}
console_initcall(pcm051_console_init);

static int pcm051_mem_init(void)
{
	omap_add_ram0(SZ_512M);

	return 0;
}
mem_initcall(pcm051_mem_init);

static struct flash_platform_data pcm051_spi_flash = {
	.name	= "nor",
	.type	= "w25q64",
};

/*
* SPI Flash works at 80Mhz however the SPI controller runs with 48MHz.
* So setup Max speed to be less than the controller speed.
*/
static struct spi_board_info pcm051_spi_board_info[] = {
	{
		.name		= "m25p80",
		.platform_data	= &pcm051_spi_flash,
		.max_speed_hz	= 24000000,
		.bus_num	= 0,
		.chip_select	= 0,
	},
};

static void pcm051_spi_init(void)
{
	int ret;

	am33xx_enable_spi0_pin_mux();

	ret = spi_register_board_info(pcm051_spi_board_info,
					ARRAY_SIZE(pcm051_spi_board_info));
	am33xx_add_spi0();
}

static int pcm051_devices_init(void)
{
	pcm051_enable_mmc0_pin_mux();

	am33xx_add_mmc0(NULL);

	pcm051_spi_init();

	devfs_add_partition("nor0", 0x00000, SZ_128K,
					DEVFS_PARTITION_FIXED, "xload");
	devfs_add_partition("nor0", SZ_128K, SZ_512K,
					DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", SZ_128K + SZ_512K, SZ_128K,
					DEVFS_PARTITION_FIXED, "env0");

	armlinux_set_bootparams((void *)(AM33XX_DRAM_ADDR_SPACE_START + 0x100));
	armlinux_set_architecture(MACH_TYPE_PCM051);

	return 0;
}
device_initcall(pcm051_devices_init);
