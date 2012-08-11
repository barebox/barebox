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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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

static int vpb_console_init(void)
{
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

static int vpb_devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, VERSATILE_FLASH_BASE, VERSATILE_FLASH_SIZE, 0);
	devfs_add_partition("nor0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env0");

	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL, VERSATILE_ETH_BASE,
			64 * 1024, IORESOURCE_MEM, NULL);

	armlinux_set_architecture(MACH_TYPE_VERSATILE_PB);
	armlinux_set_bootparams((void *)(0x00000100));

	return 0;
}
device_initcall(vpb_devices_init);
