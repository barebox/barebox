/*
 * Copyright (C) 2013 Sascha Hauer, Pengutronix
 * Copyright (C) 2014 Holger Schurig
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
 */

#include <common.h>
#include <command.h>
#include <driver.h>

static int do_drvinfo(int argc, char *argv[])
{
	struct driver_d *drv;
	struct device_d *dev;

	printf("Driver\tDevice(s)\n");
	printf("--------------------\n");
	for_each_driver(drv) {
		printf("%s\n",drv->name);
		for_each_device(dev) {
			if (dev->driver == drv)
				printf("\t%s\n", dev_name(dev));
		}
	}

	if (IS_ENABLED(CONFIG_CMD_DEVINFO))
		printf("\nUse 'devinfo DEVICE' for more information\n");

	return 0;
}


BAREBOX_CMD_START(drvinfo)
	.cmd		= do_drvinfo,
	BAREBOX_CMD_DESC("list compiled-in device drivers")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
