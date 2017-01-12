/*
 * of_path.c
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
#include <malloc.h>
#include <of.h>

#include <linux/mtd/mtd.h>

struct device_d *of_find_device_by_node_path(const char *path)
{
	struct device_d *dev;

	for_each_device(dev) {
		if (!dev->device_node)
			continue;
		if (!strcmp(path, dev->device_node->full_name))
			return dev;
	}

	return NULL;
}

/**
 * __of_find_path
 *
 * @node: The node to find the cdev for, can be the device or a
 *        partition in the device
 * @part: Optionally, a description of a parition of @node.  See of_find_path
 * @outpath: if this function returns 0 outpath will contain the path belonging
 *           to the input path description. Must be freed with free().
 * @flags: use OF_FIND_PATH_FLAGS_BB to return the .bb device if available
 *
 */
static int __of_find_path(struct device_node *node, const char *part, char **outpath, unsigned flags)
{
	struct device_d *dev;
	struct cdev *cdev;
	bool add_bb = false;

	dev = of_find_device_by_node_path(node->full_name);
	if (!dev) {
		struct device_node *devnode = node->parent;

		if (of_device_is_compatible(devnode, "fixed-partitions"))
			devnode = devnode->parent;

		dev = of_find_device_by_node_path(devnode->full_name);
		if (!dev)
			return -ENODEV;
	}

	if (dev->bus && !dev->driver)
		return -ENODEV;

	device_detect(dev);

	if (part)
		cdev = device_find_partition(dev, part);
	else
		cdev = cdev_by_device_node(node);

	if (!cdev)
		return -ENOENT;

	if ((flags & OF_FIND_PATH_FLAGS_BB) && cdev->mtd &&
	    mtd_can_have_bb(cdev->mtd))
		add_bb = true;

	*outpath = basprintf("/dev/%s%s", cdev->name, add_bb ? ".bb" : "");

	return 0;
}

/**
 * of_find_path_by_node - translate a node in the devicetree to a
 *                     	  barebox device path
 *
 * @node: the node we're interested in
 * @outpath: if this function returns 0 outpath will contain the path belonging
 *           to the input path description. Must be freed with free().
 * @flags: use OF_FIND_PATH_FLAGS_BB to return the .bb device if available
 *
 */
int of_find_path_by_node(struct device_node *node, char **outpath, unsigned flags)
{
	return __of_find_path(node, NULL, outpath, flags);
}

/**
 * of_find_path - translate a path description in the devicetree to a barebox
 *                path
 *
 * @node: the node containing the property with the path description
 * @propname: the property name of the path description
 * @outpath: if this function returns 0 outpath will contain the path belonging
 *           to the input path description. Must be freed with free().
 * @flags: use OF_FIND_PATH_FLAGS_BB to return the .bb device if available
 *
 * paths in the devicetree have the form of a multistring property. The first
 * string contains the full path to the physical device containing the path or
 * a full path to a partition described by the OF partition binding.
 * The remaining strings have the form "<type>:<options>". Currently supported
 * for <type> are:
 *
 * partname:<partname> - find a partition by its partition name. For mtd
 *                       partitions this is the label. For DOS partitions
 *                       this is the number beginning with 0.
 *
 * examples:
 *
 * device-path = &mmc0, "partname:0";
 * device-path = &norflash, "partname:barebox-environment";
 * device-path = &environment_nor;
 */
int of_find_path(struct device_node *node, const char *propname, char **outpath, unsigned flags)
{
	struct device_node *rnode;
	const char *path;
	const char *part = NULL;
	const char partnamestr[] = "partname:";

	path = of_get_property(node, propname, NULL);
	if (!path)
		return -EINVAL;

	rnode = of_find_node_by_path(path);
	if (!rnode)
		return -ENODEV;

	of_property_read_string_index(node, propname, 1, &part);
	if (part) {
		if (!strncmp(part, partnamestr, sizeof(partnamestr) - 1)) {
			part += sizeof(partnamestr) - 1;
		} else {
			pr_err("Invalid device-path: %s\n", part);
			return -EINVAL;
		}
	}

	return __of_find_path(rnode, part, outpath, flags);
}
