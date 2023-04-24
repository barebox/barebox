/*
 * hostfile.c - use files from the host to simalute barebox devices
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <driver.h>
#include <block.h>
#include <disks.h>
#include <malloc.h>
#include <mach/linux.h>
#include <init.h>
#include <errno.h>
#include <linux/err.h>
#include <mach/hostfile.h>
#include <featctrl.h>
#include <xfuncs.h>

struct hf_priv {
	union {
		struct block_device blk;
		struct cdev cdev;
	};
	const char *filename;
	int fd;
	struct feature_controller feat;
};

static ssize_t hf_read(struct hf_priv *priv, void *buf, size_t count, loff_t offset, ulong flags)
{
	int fd = priv->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_read(fd, buf, count);
}

static ssize_t hf_write(struct hf_priv *priv, const void *buf, size_t count, loff_t offset, ulong flags)
{
	int fd = priv->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_write(fd, buf, count);
}

static ssize_t hf_cdev_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	return hf_read(cdev->priv, buf, count, offset, flags);
}

static ssize_t hf_cdev_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	return hf_write(cdev->priv, buf, count, offset, flags);
}

static struct cdev_operations hf_cdev_ops = {
	.read  = hf_cdev_read,
	.write = hf_cdev_write,
};

static int hf_blk_read(struct block_device *blk, void *buf, sector_t block, blkcnt_t num_blocks)
{
	ssize_t ret = hf_read(container_of(blk, struct hf_priv, blk), buf,
			      num_blocks << SECTOR_SHIFT, block << SECTOR_SHIFT, 0);
	return ret > 0 ? 0 : ret;
}

static int hf_blk_write(struct block_device *blk, const void *buf, sector_t block, blkcnt_t num_blocks)
{
	ssize_t ret = hf_write(container_of(blk, struct hf_priv, blk), buf,
			       num_blocks << SECTOR_SHIFT, block << SECTOR_SHIFT, 0);
	return ret > 0 ? 0 : ret;
}

static struct block_device_ops hf_blk_ops = {
	.read  = hf_blk_read,
	.write = hf_blk_write,
};

static void hf_info(struct device *dev)
{
	struct hf_priv *priv = dev->priv;

	printf("file: %s\n", priv->filename);
}

static int hostfile_feat_check(struct feature_controller *feat, int idx)
{
	struct hf_priv *priv = container_of(feat, struct hf_priv, feat);

	return priv->fd >= 0 ? FEATCTRL_OKAY : FEATCTRL_GATED;
}

static int hf_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct hf_priv *priv = xzalloc(sizeof(*priv));
	struct cdev *cdev;
	bool is_featctrl = false, is_blockdev;
	u64 reg[2];
	int err;

	if (!np)
		return -ENODEV;

	dev->priv = priv;
	priv->fd = -1;

	if (IS_ENABLED(CONFIG_FEATURE_CONTROLLER) &&
	    of_property_read_bool(np, "barebox,feature-controller")) {
		priv->feat.dev = dev;
		priv->feat.check = hostfile_feat_check;

		err = feature_controller_register(&priv->feat);
		if (err)
			return err;

		is_featctrl = true;
	}


	err = of_property_read_u64_array(np, "reg", reg, ARRAY_SIZE(reg));
	if (err)
		return err;

	of_property_read_u32(np, "barebox,fd", &priv->fd);

	err = of_property_read_string(np, "barebox,filename",
				      &priv->filename);
	if (err)
		return err;

	if (priv->fd < 0)
		return is_featctrl ? 0 : priv->fd;

	dev->info = hf_info;

	is_blockdev = of_property_read_bool(np, "barebox,blockdev");

	cdev = is_blockdev ? &priv->blk.cdev : &priv->cdev;

	cdev->device_node = np;

	if (is_blockdev) {
		cdev->name = np->name;
		priv->blk.dev = dev;
		priv->blk.ops = &hf_blk_ops;
		priv->blk.blockbits = SECTOR_SHIFT;
		priv->blk.num_blocks = reg[1] / SECTOR_SIZE;

		err = blockdevice_register(&priv->blk);
		if (err)
			return err;

		err = parse_partition_table(&priv->blk);
		if (err)
			dev_warn(dev, "No partition table found\n");

		dev_info(dev, "registered as block device\n");
	} else {
		cdev->name = np->name;
		cdev->dev = dev;
		cdev->ops = &hf_cdev_ops;
		cdev->size = reg[1];
		cdev->priv = priv;

		err = devfs_create(cdev);
		if (err)
			return err;

		dev_info(dev, "registered as character device\n");
	}

	of_parse_partitions(cdev, np);
	of_partitions_register_fixup(cdev);

	return 0;
}

static __maybe_unused struct of_device_id hostfile_dt_ids[] = {
	{
		.compatible = "barebox,hostfile",
	}, {
		/* sentinel */
	}
};

static struct driver hf_drv = {
	.name  = "hostfile",
	.of_compatible = DRV_OF_COMPAT(hostfile_dt_ids),
	.probe = hf_probe,
};
device_platform_driver(hf_drv);

static int of_hostfile_fixup(struct device_node *root, void *ctx)
{
	struct hf_info *hf = ctx;
	struct device_node *node;
	bool name_only = false;
	int ret;

	node = of_get_child_by_name(root, hf->devname);
	if (node)
		name_only = true;
	else
		node = of_new_node(root, hf->devname);

	ret = of_property_write_string(node, "barebox,filename", hf->filename);
	if (ret)
		return ret;

	if (name_only)
		return 0;

	ret = of_property_write_string(node, "compatible", hostfile_dt_ids->compatible);
	if (ret)
		return ret;

	ret = of_property_write_bool(node, "barebox,blockdev", hf->is_blockdev);
	if (ret)
		return ret;

	ret = of_property_write_bool(node, "barebox,cdev", hf->is_cdev);
	if (ret)
		return ret;

	return of_property_write_bool(node, "barebox,read-only", hf->is_readonly);
}

int barebox_register_filedev(struct hf_info *hf)
{
	return of_register_fixup(of_hostfile_fixup, hf);
}

static int of_hostfile_map_fixup(struct device_node *root, void *ctx)
{
	struct device_node *node;
	int ret;

	for_each_compatible_node_from(node, root, NULL, hostfile_dt_ids->compatible) {
		struct hf_info hf = {};
		uint64_t reg[2] = {};

		hf.devname = node->name;

		ret = of_property_read_string(node, "barebox,filename", &hf.filename);
		if (ret) {
			pr_err("skipping nameless hostfile %s\n", hf.devname);
			continue;
		}

		if (memcmp(hf.filename, "$build/", 7) == 0) {
			char *fullpath = xasprintf("%s/%s", linux_get_builddir(),
					           hf.filename + sizeof "$build/" - 1);

			hf.filename = fullpath;
		}

		hf.is_blockdev = of_property_read_bool(node, "barebox,blockdev");
		hf.is_cdev = of_property_read_bool(node, "barebox,cdev");
		hf.is_readonly = of_property_read_bool(node, "barebox,read-only");

		of_property_read_u64_array(node, "reg", reg, ARRAY_SIZE(reg));

		hf.base = reg[0];
		hf.size = reg[1];

		ret = linux_open_hostfile(&hf);
		if (ret)
			continue;

		reg[0] = hf.base;
		reg[1] = hf.size;

		of_property_write_u64_array(node, "reg", reg, ARRAY_SIZE(reg));
		of_property_write_bool(node, "barebox,blockdev", hf.is_blockdev);
		of_property_write_u32(node, "barebox,fd", hf.fd);
	}

	return 0;
}

static int barebox_fixup_filedevs(void)
{
	return of_register_fixup(of_hostfile_map_fixup, NULL);
}
pure_initcall(barebox_fixup_filedevs);
