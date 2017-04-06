/*
 * Copyright (C) 2013 Sascha Hauer, Pengutronix
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

#include <command.h>
#include <common.h>
#include <complete.h>
#include <driver.h>

static int do_devinfo_subtree(struct device_d *dev, int depth)
{
	struct device_d *child;
	struct cdev *cdev, *cdevl;
	int i;

	for (i = 0; i < depth; i++)
		printf("   ");

	printf("`-- %s", dev_name(dev));
	if (!list_empty(&dev->cdevs)) {
		printf("\n");
		list_for_each_entry(cdev, &dev->cdevs, devices_list) {
			for (i = 0; i < depth + 1; i++)
				printf("   ");
			printf("`-- 0x%08llx-0x%08llx (%10s): /dev/%s",
					cdev->offset,
					cdev->offset + cdev->size - 1,
					size_human_readable(cdev->size),
					cdev->name);
			list_for_each_entry(cdevl, &cdev->links, link_entry)
				printf(", %s", cdevl->name);
			printf("\n");
		}
	} else {
		printf("\n");
	}

	if (!list_empty(&dev->children)) {
		device_for_each_child(dev, child) {
			do_devinfo_subtree(child, depth + 1);
		}
	}

	return 0;
}


static int do_devinfo(int argc, char *argv[])
{
	struct device_d *dev;
	struct param_d *param;
	int i;
	int first;
	struct resource *res;

	if (argc == 1) {
		for_each_device(dev) {
			if (!dev->parent)
				do_devinfo_subtree(dev, 0);
		}
	} else {
		dev = get_device_by_name(argv[1]);
		if (!dev)
			return -ENODEV;

		if (dev->num_resources)
			printf("Resources:\n");
		for (i = 0; i < dev->num_resources; i++) {
			resource_size_t size;
			res = &dev->resource[i];
			size = resource_size(res);
			printf("  num: %d\n", i);
			if (res->name)
				printf("  name: %s\n", res->name);
			printf("  start: %pa\n"
				   "  size: %pa\n",
			       &res->start, &size);
		}

		if (dev->driver)
			printf("Driver: %s\n", dev->driver->name);

		if (dev->bus)
			printf("Bus: %s\n", dev->bus->name);

		if (dev->info)
			dev->info(dev);

		first = true;
		list_for_each_entry(param, &dev->parameters, list) {
			if (first) {
				printf("Parameters:\n");
				first = false;
			}
			printf("  %s: %s (type: %s)", param->name, dev_get_param(dev, param->name),
			       get_param_type(param));
			if (param->info) {
				param->info(param);
			}
			printf("\n");
		}
#ifdef CONFIG_OFDEVICE
		if (dev->device_node) {
			printf("Device node: %s\n", dev->device_node->full_name);
			of_print_nodes(dev->device_node, 0);
		}
#endif
	}

	return 0;
}

BAREBOX_CMD_HELP_START(devinfo)
BAREBOX_CMD_HELP_TEXT("If called without arguments, devinfo shows a summary of the known")
BAREBOX_CMD_HELP_TEXT("devices.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("If called with a device path being the argument, devinfo shows more")
BAREBOX_CMD_HELP_TEXT("default information about this device and its parameters.")
BAREBOX_CMD_HELP_END


BAREBOX_CMD_START(devinfo)
	.cmd		= do_devinfo,
	BAREBOX_CMD_DESC("show information about devices")
	BAREBOX_CMD_OPTS("[DEVICE]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_devinfo_help)
	BAREBOX_CMD_COMPLETE(device_complete)
BAREBOX_CMD_END
