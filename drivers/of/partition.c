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
#include <init.h>
#include <globalvar.h>

static unsigned int of_partition_binding;

enum of_binding_name {
	MTD_OF_BINDING_NEW,
	MTD_OF_BINDING_LEGACY,
	MTD_OF_BINDING_DONTTOUCH,
};

struct cdev *of_parse_partition(struct cdev *cdev, struct device_node *node)
{
	const char *partname;
	char *filename;
	struct cdev *new;
	const __be32 *reg;
	u64 offset, size;
	const char *name;
	int len;
	unsigned long flags = 0;
	int na, ns;

	if (!node)
		return NULL;

	reg = of_get_property(node, "reg", &len);
	if (!reg)
		return NULL;

	na = of_n_addr_cells(node);
	ns = of_n_size_cells(node);

	if (len < (na + ns) * sizeof(__be32)) {
		pr_err("reg property too small in %s\n", node->full_name);
		return NULL;
	}

	offset = of_read_number(reg, na);
	size = of_read_number(reg + na, ns);

	partname = of_get_property(node, "label", &len);
	if (!partname)
		partname = of_get_property(node, "name", &len);
	if (!partname)
		return NULL;

	name = (char *)partname;

	debug("add partition: %s.%s 0x%08llx 0x%08llx\n", cdev->name, partname, offset, size);

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

	cdev->device_node = node;

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

static void delete_subnodes(struct device_node *np)
{
	struct device_node *part, *tmp;

	for_each_child_of_node_safe(np, tmp, part) {
		if (of_get_property(part, "compatible", NULL))
			continue;

		of_delete_node(part);
	}
}

static int of_partition_fixup(struct device_node *root, void *ctx)
{
	struct cdev *cdev = ctx, *partcdev;
	struct device_node *np, *part, *partnode;
	char *name;
	int ret;
	int n_cells, n_parts = 0;

	if (of_partition_binding == MTD_OF_BINDING_DONTTOUCH)
		return 0;

	if (!cdev->device_node)
		return -EINVAL;

	list_for_each_entry(partcdev, &cdev->partitions, partition_entry) {
		if (partcdev->flags & DEVFS_PARTITION_FROM_TABLE)
			continue;
		n_parts++;
	}

	if (!n_parts)
		return 0;

	if (cdev->size >= 0x100000000)
		n_cells = 2;
	else
		n_cells = 1;

	name = of_get_reproducible_name(cdev->device_node);
	np = of_find_node_by_reproducible_name(root, name);
	free(name);
	if (!np) {
		dev_err(cdev->dev, "Cannot find nodepath %s, cannot fixup\n",
				cdev->device_node->full_name);
		return -EINVAL;
	}

	partnode = of_get_child_by_name(np, "partitions");
	if (partnode) {
		if (of_partition_binding == MTD_OF_BINDING_LEGACY) {
			of_delete_node(partnode);
			partnode = np;
		}
		delete_subnodes(partnode);
	} else {
		delete_subnodes(np);

		if (of_partition_binding == MTD_OF_BINDING_LEGACY)
			partnode = np;
		else
			partnode = of_new_node(np, "partitions");
	}

	if (of_partition_binding == MTD_OF_BINDING_NEW) {
		ret = of_property_write_string(partnode, "compatible",
					       "fixed-partitions");
		if (ret)
			return ret;
	}

	ret = of_property_write_u32(partnode, "#size-cells", n_cells);
	if (ret)
		return ret;

	ret = of_property_write_u32(partnode, "#address-cells", n_cells);
	if (ret)
		return ret;

	list_for_each_entry(partcdev, &cdev->partitions, partition_entry) {
		int na, ns, len = 0;
		char *name;
		void *p;
		u8 tmp[16 * 16]; /* Up to 64-bit address + 64-bit size */
		loff_t partoffset;

		if (partcdev->flags & DEVFS_PARTITION_FROM_TABLE)
			continue;

		if (partcdev->mtd)
			partoffset = partcdev->mtd->master_offset;
		else
			partoffset = partcdev->offset;

		name = basprintf("partition@%0llx", partoffset);
		if (!name)
			return -ENOMEM;

		part = of_new_node(partnode, name);
		free(name);
		if (!part)
			return -ENOMEM;

		p = of_new_property(part, "label", partcdev->partname,
                                strlen(partcdev->partname) + 1);
		if (!p)
			return -ENOMEM;

		na = of_n_addr_cells(part);
		ns = of_n_size_cells(part);

		of_write_number(tmp + len, partoffset, na);
		len += na * 4;
		of_write_number(tmp + len, partcdev->size, ns);
		len += ns * 4;

		ret = of_set_property(part, "reg", tmp, len, 1);
		if (ret)
			return ret;

		if (partcdev->flags & DEVFS_PARTITION_READONLY) {
			ret = of_set_property(part, "read-only", NULL, 0, 1);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int of_partitions_register_fixup(struct cdev *cdev)
{
	return of_register_fixup(of_partition_fixup, cdev);
}

static const char *of_binding_names[] = {
	"new", "legacy", "donttouch"
};

static int of_partition_init(void)
{
	if (IS_ENABLED(CONFIG_GLOBALVAR))
		dev_add_param_enum(&global_device, "of_partition_binding",
				   NULL, NULL,
				   &of_partition_binding, of_binding_names,
				   ARRAY_SIZE(of_binding_names), NULL);

	return 0;
}
device_initcall(of_partition_init);
