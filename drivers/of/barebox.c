/*
 * barebox.c
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
#include <partition.h>
#include <envfs.h>
#include <linux/mtd/mtd.h>

static int environment_probe(struct device_d *dev)
{
	char *path;
	int ret;

	ret = of_find_path(dev->device_node, "device-path", &path, OF_FIND_PATH_FLAGS_BB);
	if (ret)
		return ret;

	dev_info(dev, "setting default environment path to %s\n", path);

	default_environment_path_set(path);

	return 0;
}

static struct of_device_id environment_dt_ids[] = {
	{
		.compatible = "barebox,environment",
	}, {
		/* sentinel */
	}
};

static struct driver_d environment_driver = {
	.name		= "barebox-environment",
	.probe		= environment_probe,
	.of_compatible	= environment_dt_ids,
};

static int barebox_of_driver_init(void)
{
	struct device_node *node;

	node = of_get_root_node();
	if (!node)
		return 0;

	node = of_find_node_by_path("/chosen");
	if (!node)
		return 0;

	of_platform_populate(node, of_default_bus_match_table, NULL);

	platform_driver_register(&environment_driver);

	return 0;
}
late_initcall(barebox_of_driver_init);
