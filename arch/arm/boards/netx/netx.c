/*
 * Copyright (C) 2004 Sascha Hauer, Synertronixx GmbH
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
#include <init.h>
#include <environment.h>
#include <mach/netx-regs.h>
#include <partition.h>
#include <asm/armlinux.h>
#include <fs.h>
#include <fcntl.h>
#include <generated/mach-types.h>
#include <mach/netx-eth.h>

static struct device_d cfi_dev = {
	.id	  = -1,
	.name     = "cfi_flash",
	.map_base = 0xC0000000,
	.size     = 32 * 1024 * 1024,
};

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id	  = -1,
	.name     = "mem",
	.map_base = 0x80000000,
	.size     = 64 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

struct netx_eth_platform_data eth0_data = {
	.xcno = 0,
};

static struct device_d netx_eth_dev0 = {
	.id		= -1,
	.name		= "netx-eth",
	.platform_data	= &eth0_data,
};

struct netx_eth_platform_data eth1_data = {
	.xcno = 1,
};

static struct device_d netx_eth_dev1 = {
	.id		= -1,
	.name		= "netx-eth",
	.platform_data	= &eth1_data,
};

static int netx_devices_init(void) {
	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&netx_eth_dev0);
	register_device(&netx_eth_dev1);

	devfs_add_partition("nor0", 0x00000, 0x40000, PARTITION_FIXED, "self0");

	/* Do not overwrite primary env for now */
	devfs_add_partition("nor0", 0xc0000, 0x80000, PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_NXDB500);

	return 0;
}

device_initcall(netx_devices_init);

static struct device_d netx_serial_device = {
	.id	  = -1,
	.name     = "netx_serial",
	.map_base = NETX_PA_UART0,
	.size     = 0x40,
};

static int netx_console_init(void)
{
	/* configure gpio for serial */
	*(volatile unsigned long *)(0x00100800) = 2;
	*(volatile unsigned long *)(0x00100804) = 2;
	*(volatile unsigned long *)(0x00100808) = 2;
	*(volatile unsigned long *)(0x0010080c) = 2;

	register_device(&netx_serial_device);
	return 0;
}

console_initcall(netx_console_init);

