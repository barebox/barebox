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

static int of_mtd_fixup(struct device_node *root, void *ctx)
{
	struct cdev *cdev = ctx;
	struct mtd_info *mtd, *partmtd;
	struct device_node *np, *part, *tmp;
	int ret;

	mtd = container_of(cdev, struct mtd_info, cdev);

	np = of_find_node_by_path_from(root, mtd->of_path);
	if (!np) {
		dev_err(&mtd->class_dev, "Cannot find nodepath %s, cannot fixup\n",
				mtd->of_path);
		return -EINVAL;
	}

	for_each_child_of_node_safe(np, tmp, part) {
		if (of_get_property(part, "compatible", NULL))
			continue;
		of_delete_node(part);
	}

	list_for_each_entry(partmtd, &mtd->partitions, partitions_entry) {
		int na, ns, len = 0;
		char *name = basprintf("partition@%0llx",
					 partmtd->master_offset);
		void *p;
		u8 tmp[16 * 16]; /* Up to 64-bit address + 64-bit size */

		if (!name)
			return -ENOMEM;

		part = of_new_node(np, name);
		free(name);
		if (!part)
			return -ENOMEM;

		p = of_new_property(part, "label", partmtd->cdev.partname,
                                strlen(partmtd->cdev.partname) + 1);
		if (!p)
			return -ENOMEM;

		na = of_n_addr_cells(part);
		ns = of_n_size_cells(part);

		of_write_number(tmp + len, partmtd->master_offset, na);
		len += na * 4;
		of_write_number(tmp + len, partmtd->size, ns);
		len += ns * 4;

		ret = of_set_property(part, "reg", tmp, len, 1);
		if (ret)
			return ret;

		if (partmtd->cdev.flags & DEVFS_PARTITION_READONLY) {
			ret = of_set_property(part, "read-only", NULL, 0, 1);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int of_partitions_register_fixup(struct cdev *cdev)
{
	return of_register_fixup(of_mtd_fixup, cdev);
}