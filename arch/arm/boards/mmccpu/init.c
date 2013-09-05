/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <fec.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <io.h>
#include <asm/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/iomux.h>
#include <mach/io.h>

static struct macb_platform_data macb_pdata = {
	.phy_flags	= PHYLIB_FORCE_LINK,
	.phy_addr	= 4,
};

static int mmccpu_mem_init(void)
{
	at91_add_device_sdram(128 * 1024 * 1024);

	return 0;
}
mem_initcall(mmccpu_mem_init);

static int mmccpu_devices_init(void)
{
	/*
	 * PB27 enables the 50MHz oscillator for Ethernet PHY
	 * 1 - enable
	 * 0 - disable
	 */
	at91_set_gpio_output(AT91_PIN_PB27, 1);
	gpio_set_value(AT91_PIN_PB27, 1); /* 1- enable, 0 - disable */

	at91_add_device_eth(0, &macb_pdata);
	add_cfi_flash_device(0, AT91_CHIPSELECT_0, 0, 0);

	devfs_add_partition("nor0", 0x00000, 256 * 1024, DEVFS_PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 128 * 1024, DEVFS_PARTITION_FIXED, "env0");

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	armlinux_set_architecture(MACH_TYPE_MMCCPU);

	return 0;
}

device_initcall(mmccpu_devices_init);

static int mmccpu_console_init(void)
{
	barebox_set_model("Bucyrus MMC-CPU");
	barebox_set_hostname("mmccpu");

	at91_register_uart(0, 0);
	return 0;
}

console_initcall(mmccpu_console_init);

static int mmccpu_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(mmccpu_main_clock);
