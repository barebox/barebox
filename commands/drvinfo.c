// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2013 Sascha Hauer, Pengutronix
// SPDX-FileCopyrightText: © 2014 Holger Schurig

#include <common.h>
#include <command.h>
#include <driver.h>
#include <complete.h>
#include <fnmatch.h>

static int do_drvinfo(int argc, char *argv[])
{
	char *pattern = argv[1];
	struct driver *drv;
	struct device *dev;

	printf("Driver\tDevice(s)\n");
	printf("--------------------\n");
	for_each_driver(drv) {
		if (pattern && fnmatch(pattern, drv->name, 0))
			continue;

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
	BAREBOX_CMD_OPTS("[DRIVER]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(driver_complete)
BAREBOX_CMD_END
