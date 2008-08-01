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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include <cfi_flash.h>
#include <init.h>
#include <environment.h>
#include <fec.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/memory-map.h>
#include <asm/arch/ether.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <asm/arch/gpio.h>

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x20000000,
	.size     = 64 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",

	.map_base = 0x10000000,
	.size     = 4 * 1024 * 1024,
};

static struct at91sam_ether_platform_data macb_pdata = {
	.flags = AT91SAM_ETHER_RMII,
	.phy_addr = 0,
};

static struct device_d macb_dev = {
	.name     = "macb",
	.id       = "eth0",
	.map_base = AT91SAM9263_BASE_EMAC,
	.size     = 0x1000,
	.type     = DEVICE_TYPE_ETHER,
	.platform_data = &macb_pdata,
};

static int pm9263_devices_init(void)
{
	register_device(&sdram_dev);
	register_device(&macb_dev);
	register_device(&cfi_dev);

#ifdef CONFIG_PARTITION
	dev_add_partition(&cfi_dev, 0x00000, 0x40000, PARTITION_FIXED, "self");
	dev_add_partition(&cfi_dev, 0x40000, 0x10000, PARTITION_FIXED, "env");
#endif

	armlinux_set_bootparams((void *)0x20000100);
	armlinux_set_architecture(0x4b2);

	return 0;
}

device_initcall(pm9263_devices_init);

static struct device_d pm9263_serial_device = {
	.name     = "atmel_serial",
	.id       = "cs0",
	.map_base = AT91_DBGU + AT91_BASE_SYS,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int pm9263_console_init(void)
{
	register_device(&pm9263_serial_device);
	return 0;
}

console_initcall(pm9263_console_init);
