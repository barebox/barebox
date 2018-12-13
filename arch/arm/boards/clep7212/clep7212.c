/*
 * Copyright (C) 2012 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <driver.h>
#include <envfs.h>
#include <init.h>
#include <partition.h>
#include <io.h>
#include <linux/sizes.h>
#include <asm/armlinux.h>
#include <asm/mmu.h>
#include <generated/mach-types.h>

#include <mach/clps711x.h>
#include <mach/devices.h>

static int clps711x_devices_init(void)
{
	u32 serial_h = 0, serial_l = readl(UNIQID);
	void *cfi_io;

	/* Setup Chipselects */
	clps711x_setup_memcfg(0, MEMCFG_WAITSTATE_6_1 | MEMCFG_BUS_WIDTH_16);
	clps711x_setup_memcfg(1, MEMCFG_WAITSTATE_6_1 | MEMCFG_BUS_WIDTH_8);
	clps711x_setup_memcfg(2, MEMCFG_WAITSTATE_8_3 | MEMCFG_BUS_WIDTH_16 |
			      MEMCFG_CLKENB);
	clps711x_setup_memcfg(3, MEMCFG_WAITSTATE_7_1 | MEMCFG_BUS_WIDTH_32);

	cfi_io = map_io_sections(CS0_BASE, (void *)0x90000000, SZ_32M);
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, (unsigned long)cfi_io, SZ_32M,
			     IORESOURCE_MEM);

	devfs_add_partition("nor0", 0x00000, SZ_512K, DEVFS_PARTITION_FIXED,
			    "self0");
	devfs_add_partition("nor0", SZ_256K, SZ_256K, DEVFS_PARTITION_FIXED,
			    "env0");

	armlinux_set_architecture(MACH_TYPE_CLEP7212);
	armlinux_set_serial(((u64)serial_h << 32) | serial_l);

	defaultenv_append_directory(defaultenv_clep7212);

	return 0;
}
device_initcall(clps711x_devices_init);

static int clps711x_console_init(void)
{
	barebox_set_model("Cirrus Logic CLEP7212");
	barebox_set_hostname("clep7212");

	clps711x_add_uart(0);

	return 0;
}
console_initcall(clps711x_console_init);
