/*
 * Copyright (C) 2014 Juergen Beisert, Pengutronix, Michel Stam,
 * Fugro Intersite
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
 * @brief Generic PC support for the IDE platform driver
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/err.h>
#include <platform_ide.h>
#include "envsector.h"

static struct ide_port_info ide_plat = {
	.ioport_shift = 0,
	.dataif_be = 0,
};

static struct resource primary_ide_resources[] = {
	{
		.name = "base",
		.start = 0x1f0,
		.end =  0x1f7,
		.flags = IORESOURCE_IO
	},
	{
		.name = "alt",
		.start = 0x3f6,
		.end =  0x3f7,
		.flags = IORESOURCE_IO
	}
};

static struct resource secondary_ide_resources[] = {
	{
		.name = "base",
		.start = 0x170,
		.end =  0x177,
		.flags = IORESOURCE_IO
	},
};

static struct device_d primary_ide_device = {
	.name = "ide_intf",
	.id = 0,
	.platform_data = &ide_plat,
	.resource = primary_ide_resources,
	.num_resources = ARRAY_SIZE(primary_ide_resources),
};

static struct device_d secondary_ide_device = {
	.name = "ide_intf",
	.id = 1,
	.platform_data = &ide_plat,
	.resource = secondary_ide_resources,
	.num_resources = ARRAY_SIZE(secondary_ide_resources),
};

static int platform_ide_init(void)
{
	struct cdev *cdev;

	platform_device_register(&primary_ide_device);
	platform_device_register(&secondary_ide_device);

	if (pers_env_size != PATCH_AREA_PERS_SIZE_UNUSED) {
		cdev = devfs_add_partition("ata0",
				pers_env_storage * 512,
				(unsigned)pers_env_size * 512,
				DEVFS_PARTITION_FIXED, "env0");
		printf("Partition: %ld\n", IS_ERR(cdev) ? PTR_ERR(cdev) : 0);
	} else
		printf("No persistent storage defined\n");

	return 0;
}
device_initcall(platform_ide_init);
