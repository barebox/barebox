/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <memory.h>
#include <ns16550.h>
#include <mach/hardware.h>
#include <io.h>
#include <partition.h>
#include <sizes.h>
#include <asm/common.h>

static int malta_mem_init(void)
{
	barebox_add_memory_bank("ram0", 0xa0000000, SZ_256M);

	return 0;
}
mem_initcall(malta_mem_init);

static int malta_devices_init(void)
{
	add_cfi_flash_device(0, 0x1e000000, SZ_4M, 0);

	devfs_add_partition("nor0", 0x0, SZ_512K, DEVFS_PARTITION_FIXED, "self");
	devfs_add_partition("nor0", SZ_512K, SZ_64K, DEVFS_PARTITION_FIXED, "env0");

	return 0;
}
device_initcall(malta_devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 1843200, /* no matter for emulated port */
	.shift = 0,
};

static int malta_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(-1, DEBUG_LL_UART_ADDR, 8,
			IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(malta_console_init);
