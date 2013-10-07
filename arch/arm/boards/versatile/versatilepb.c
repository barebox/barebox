/*
 * Copyright (C) 2010 B Labs Ltd,
 * http://l4dev.org
 * Author: Alexey Zaytsev <alexey.zaytsev@gmail.com>
 *
 * Based on mach-nomadik
 * Copyright (C) 2009-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <init.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/init.h>
#include <mach/platform.h>
#include <environment.h>
#include <partition.h>
#include <sizes.h>
#include <net/smc91111.h>

static int vpb_console_init(void)
{
	barebox_set_model("ARM Versatile/PB (ARM926EJ-S)");
	barebox_set_hostname("versatilepb");

	versatile_register_uart(0);
	return 0;
}
console_initcall(vpb_console_init);

static int vpb_mem_init(void)
{
	versatile_add_sdram(64 * 1024 *1024);

	return 0;
}
mem_initcall(vpb_mem_init);

static struct smc91c111_pdata net_pdata = {
	.qemu_fixup = 1,
};

static int vpb_devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, VERSATILE_FLASH_BASE, VERSATILE_FLASH_SIZE, 0);
	versatile_register_i2c();
	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL, VERSATILE_ETH_BASE,
			64 * 1024, IORESOURCE_MEM, &net_pdata);

	armlinux_set_architecture(MACH_TYPE_VERSATILE_PB);
	armlinux_set_bootparams((void *)(0x00000100));

	return 0;
}
device_initcall(vpb_devices_init);
