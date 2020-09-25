// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix

/**
 * @file
 * @brief Generic PC support for the BIOS disk interface
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/err.h>
#include "envsector.h"

static int bios_disk_init(void)
{
	struct cdev *cdev;

	add_generic_device("biosdrive", DEVICE_ID_DYNAMIC, NULL, 0, 0,
			   IORESOURCE_MEM, NULL);

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
device_initcall(bios_disk_init);
