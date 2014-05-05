/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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

/**
 * @file
 * @brief Generic PC support to let barebox acting as a boot loader
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/syslib.h>
#include <linux/err.h>

static int devices_init(void)
{
	/* extended memory only */
	add_mem_device("ram0", 0x0, bios_get_memsize() << 10,
		       IORESOURCE_MEM_WRITEABLE);
	return 0;
}
device_initcall(devices_init);

/** @page generic_pc Generic PC based bootloader

This platform acts as a generic PC based bootloader. It depends on at least
one boot media that is connected locally (no network boot) and can be
handled by the regular BIOS (any kind of hard disks for example).

The created @a barebox image can be used to boot a standard x86 bzImage
Linux kernel.

Refer section @ref x86_bootloader_preparations how to do so.

How to get the binary image:

Using the default configuration:

@code
make ARCH=x86 generic_defconfig
@endcode

Build the binary image:

@code
make ARCH=x86 CROSS_COMPILE=x86compiler
@endcode

@note replace the 'x86compiler' with your x86 (cross) compiler.

*/
