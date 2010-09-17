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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <fec.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <mach/at91_pmc.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/io.h>

static struct device_d cfi_dev = {
	.id		= -1,
	.name		= "cfi_flash",
	.map_base	= AT91_CHIPSELECT_0,
	.size		= 0,	/* zero means autodetect size */
};

static struct at91_ether_platform_data macb_pdata = {
	.flags		= AT91SAM_ETHER_MII | AT91SAM_ETHER_FORCE_LINK,
	.phy_addr	= 4,
};

static int mmccpu_devices_init(void)
{
	/*
	 * PB27 enables the 50MHz oscillator for Ethernet PHY
	 * 1 - enable
	 * 0 - disable
	 */
	at91_set_gpio_output(AT91_PIN_PB27, 1);
	at91_set_gpio_value(AT91_PIN_PB27, 1); /* 1- enable, 0 - disable */

	at91_add_device_sdram(128 * 1024 * 1024);
	at91_add_device_eth(&macb_pdata);
	register_device(&cfi_dev);

	devfs_add_partition("nor0", 0x00000, 256 * 1024, PARTITION_FIXED, "self0");
	devfs_add_partition("nor0", 0x40000, 128 * 1024, PARTITION_FIXED, "env0");

	armlinux_set_bootparams((void *)(AT91_CHIPSELECT_1 + 0x100));
	armlinux_set_architecture(MACH_TYPE_MMCCPU);

	return 0;
}

device_initcall(mmccpu_devices_init);

static int mmccpu_console_init(void)
{
	at91_register_uart(0, 0);
	return 0;
}

console_initcall(mmccpu_console_init);
