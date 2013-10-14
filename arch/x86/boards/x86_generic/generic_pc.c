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
#include <ns16550.h>
#include <linux/err.h>

/*
 * These datas are from the MBR, created by the linker and filled by the
 * setup tool while installing barebox on the disk drive
 */
extern uint64_t pers_env_storage;
extern uint16_t pers_env_size;
extern uint8_t pers_env_drive;

/**
 * Persistent environment "not used" marker.
 * Note: Must be in accordance to the value the tool "setup_mbr" writes.
 */
#define PATCH_AREA_PERS_SIZE_UNUSED 0x000

static int devices_init(void)
{
	struct cdev *cdev;

	/* extended memory only */
	add_mem_device("ram0", 0x0, bios_get_memsize() << 10,
		       IORESOURCE_MEM_WRITEABLE);
	add_generic_device("biosdrive", DEVICE_ID_DYNAMIC, NULL, 0, 0, IORESOURCE_MEM,
			NULL);

	if (pers_env_size != PATCH_AREA_PERS_SIZE_UNUSED) {
		cdev = devfs_add_partition("biosdisk0",
				pers_env_storage * 512,
				(unsigned)pers_env_size * 512,
				DEVFS_PARTITION_FIXED, "env0");
		printf("Partition: %ld\n", IS_ERR(cdev) ? PTR_ERR(cdev) : 0);
	} else
		printf("No persistent storage defined\n");

        return 0;
}
device_initcall(devices_init);

#ifdef CONFIG_DRIVER_SERIAL_NS16550

static struct NS16550_plat serial_plat = {
       .clock = 1843200,
       .reg_read = x86_uart_read,
       .reg_write = x86_uart_write,
};

static int pc_console_init(void)
{
	barebox_set_model("X86 generic barebox");
	barebox_set_hostname("x86");

	/* Register the serial port */
	add_ns16550_device(DEVICE_ID_DYNAMIC, 0x3f8, 8, 0, &serial_plat);

	return 0;
}
console_initcall(pc_console_init);

#endif

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
