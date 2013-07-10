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

struct of_path {
	struct cdev *cdev;
	struct device_d *dev;
};

struct of_path_type {
	const char *name;
	int (*parse)(struct of_path *op, const char *str);
};

/**
 * of_path_type_partname - find a partition based on physical device and
 *                         partition name
 * @op: of_path context
 * @name: the partition name to find
 */
static int of_path_type_partname(struct of_path *op, const char *name)
{
	if (!op->dev)
		return -EINVAL;

	op->cdev = device_find_partition(op->dev, name);
	if (op->cdev) {
		pr_debug("%s: found part '%s'\n", __func__, name);
		return 0;
	} else {
		pr_debug("%s: cannot find part '%s'\n", __func__, name);
		return -ENODEV;
	}
}

static struct of_path_type of_path_types[] = {
	{
		.name = "partname",
		.parse = of_path_type_partname,
	},
};

static int of_path_parse_one(struct of_path *op, const char *str)
{
	int i, ret;
	char *name, *desc;

	pr_debug("parsing: %s\n", str);

	name = xstrdup(str);
	desc = strchr(name, ':');
	if (!desc) {
		free(name);
		return -EINVAL;
	}

	*desc = 0;
	desc++;

	for (i = 0; i < ARRAY_SIZE(of_path_types); i++) {
		if (!strcmp(of_path_types[i].name, name)) {
			ret = of_path_types[i].parse(op, desc);
			goto out;
		}
	}

	ret = -EINVAL;
out:
	free(name);

	return ret;
}

/**
 * of_find_path - translate a path description in the devicetree to a barebox
 *                path
 *
 * @node: the node containing the property with the path description
 * @propname: the property name of the path description
 * @outpath: if this function returns 0 outpath will contain the path belonging
 *           to the input path description. Must be freed with free().
 *
 * pathes in the devicetree have the form of a multistring property. The first
 * string contains the full path to the physical device containing the path.
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
 */
int of_find_path(struct device_node *node, const char *propname, char **outpath)
{
	struct of_path op = {};
	struct device_node *rnode;
	const char *path, *str;
	int i, len, ret;

	path = of_get_property(node, propname, &len);
	if (!path)
		return -EINVAL;

	rnode = of_find_node_by_path(path);
	if (!rnode)
		return -ENODEV;

	op.dev = of_find_device_by_node_path(rnode->full_name);
	if (!op.dev)
		return -ENODEV;

	device_detect(op.dev);

	i = 1;

	while (1) {
		ret = of_property_read_string_index(node, propname, i++, &str);
		if (ret)
			break;

		ret = of_path_parse_one(&op, str);
		if (ret)
			return ret;
	}

	if (!op.cdev)
		return -ENOENT;

	*outpath = asprintf("/dev/%s", op.cdev->name);

	return 0;
}
