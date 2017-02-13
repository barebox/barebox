/*
 * partition.c - devicetree partition parsing
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
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
 */
#include <common.h>
#include <of.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/err.h>
#include <nand.h>

struct cdev *of_parse_partition(struct cdev *cdev, struct device_node *node)
{
	const char *partname;
	char *filename;
	struct cdev *new;
	const __be32 *reg;
	unsigned long offset, size;
	const char *name;
	int len;
	unsigned long flags = 0;

	if (!node)
		return NULL;

	reg = of_get_property(node, "reg", &len);
	if (!reg)
		return NULL;

	offset = be32_to_cpu(reg[0]);
	size = be32_to_cpu(reg[1]);

	partname = of_get_property(node, "label", &len);
	if (!partname)
		partname = of_get_property(node, "name", &len);
	if (!partname)
		return NULL;

	name = (char *)partname;

	debug("add partition: %s.%s 0x%08lx 0x%08lx\n", cdev->name, partname, offset, size);

	if (of_get_property(node, "read-only", &len))
		flags = DEVFS_PARTITION_READONLY;

	filename = basprintf("%s.%s", cdev->name, partname);

	new = devfs_add_partition(cdev->name, offset, size, flags, filename);
	if (IS_ERR(new))
		new = NULL;

	if (new)
		new->device_node = node;;

	free(filename);

	return new;
}

int of_parse_partitions(struct cdev *cdev, struct device_node *node)
{
	struct device_node *n, *subnode;

	if (!node)
		return -EINVAL;

	subnode = of_get_child_by_name(node, "partitions");
	if (subnode) {
		if (!of_device_is_compatible(subnode, "fixed-partitions"))
			return -EINVAL;
		node = subnode;
	}

	for_each_child_of_node(node, n) {
		of_parse_partition(cdev, n);
	}

	return 0;
}
