/*
 * arch/arm/mach-at91/at91sam9260_devices.c
 *
 *  Copyright (C) 2006 Atmel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <common.h>
#include <asm/armlinux.h>
#include <asm/hardware.h>
#include <asm/arch/board.h>

static struct memory_platform_data sram_pdata = {
	.name = "sram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.name     = "mem",
	.map_base = 0x20000000,
	.platform_data = &sram_pdata,
};

void at91_add_device_sdram(u32 size)
{
	sdram_dev.size = size;
	register_device(&sdram_dev);
	armlinux_add_dram(&sdram_dev);
}

#if defined(CONFIG_DRIVER_NET_MACB)
static struct device_d macb_dev = {
	.name     = "macb",
	.map_base = AT91C_BASE_EMACB,
	.size     = 0x1000,
};

void at91_add_device_eth(struct at91_ether_platform_data *data)
{
	if (!data)
		return;

	macb_dev.platform_data = data;
	register_device(&macb_dev);
}
#else
void at91_add_device_eth(struct at91_ether_platform_data *data) {}
#endif

#if defined(CONFIG_NAND_ATMEL)
static struct device_d nand_dev = {
	.name     = "atmel_nand",
	.map_base = 0x40000000,
	.size     = 0x10,
};

void at91_add_device_nand(struct atmel_nand_data *data)
{
	nand_dev.platform_data = data;
	register_device(&nand_dev);
}
#else
void at91_add_device_nand(struct atmel_nand_data *data) {}
#endif

static struct device_d dbgu_serial_device = {
	.name     = "atmel_serial",
	.map_base = AT91C_BASE_DBGU,
	.size     = 4096,
};

void at91_register_uart(unsigned id)
{
	switch (id) {
		case 0:		/* DBGU */
			register_device(&dbgu_serial_device);
			break;
		default:
			return;
	}

}
