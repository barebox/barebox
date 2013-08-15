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

struct netx_eth_platform_data eth0_data = {
	.xcno = 0,
};

struct netx_eth_platform_data eth1_data = {
	.xcno = 1,
};

static int netx_mem_init(void)
{
	arm_add_mem_device("ram0", 0x80000000, 64 * 1024 * 1024);

	return 0;
}
mem_initcall(netx_mem_init);

static int netx_devices_init(void) {
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, 0xC0000000, 32 * 1024 * 1024, 0);

	add_generic_device("netx-eth", DEVICE_ID_DYNAMIC, NULL, 0, 0, IORESOURCE_MEM,
			&eth0_data);
	add_generic_device("netx-eth", DEVICE_ID_DYNAMIC, NULL, 0, 0, IORESOURCE_MEM,
			&eth1_data);

	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self0");

	/* Do not overwrite primary env for now */
	devfs_add_partition("nor0", 0xc0000, 0x80000, DEVFS_PARTITION_FIXED, "env0");

	protect_file("/dev/env0", 1);

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_NXDB500);

	return 0;
}

device_initcall(netx_devices_init);

static int netx_console_init(void)
{
	/* configure gpio for serial */
	*(volatile unsigned long *)(0x00100800) = 2;
	*(volatile unsigned long *)(0x00100804) = 2;
	*(volatile unsigned long *)(0x00100808) = 2;
	*(volatile unsigned long *)(0x0010080c) = 2;

	barebox_set_model("Hilscher Netx nxdb500");
	barebox_set_hostname("nxdb500");

	add_generic_device("netx_serial", DEVICE_ID_DYNAMIC, NULL, NETX_PA_UART0, 0x40,
			   IORESOURCE_MEM, NULL);
	return 0;
}

console_initcall(netx_console_init);

