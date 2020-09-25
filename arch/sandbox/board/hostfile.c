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
#include <xfuncs.h>

#include <linux/err.h>

struct hf_priv {
	union {
		struct block_device blk;
		struct cdev cdev;
	};
	const char *filename;
	int fd;
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

static int hf_blk_read(struct block_device *blk, void *buf, int block, int num_blocks)
{
	ssize_t ret = hf_read(container_of(blk, struct hf_priv, blk), buf,
			      num_blocks << SECTOR_SHIFT, block << SECTOR_SHIFT, 0);
	return ret > 0 ? 0 : ret;
}

static int hf_blk_write(struct block_device *blk, const void *buf, int block, int num_blocks)
{
	ssize_t ret = hf_write(container_of(blk, struct hf_priv, blk), buf,
			       num_blocks << SECTOR_SHIFT, block << SECTOR_SHIFT, 0);
	return ret > 0 ? 0 : ret;
}

static struct block_device_ops hf_blk_ops = {
	.read  = hf_blk_read,
	.write = hf_blk_write,
};

static void hf_info(struct device_d *dev)
{
	struct hf_priv *priv = dev->priv;

	printf("file: %s\n", priv->filename);
}

static int hf_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct hf_priv *priv = xzalloc(sizeof(*priv));
	struct resource *res;
	struct cdev *cdev;
	bool is_blockdev;
	resource_size_t size;
	int err;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	size = resource_size(res);

	if (!np)
		return -ENODEV;

	of_property_read_u32(np, "barebox,fd", &priv->fd);

	err = of_property_read_string(np, "barebox,filename",
				      &priv->filename);
	if (err)
		return err;

	if (!priv->fd)
		priv->fd = linux_open(priv->filename, true);

	if (priv->fd < 0)
		return priv->fd;

	dev->info = hf_info;
	dev->priv = priv;

	is_blockdev = of_property_read_bool(np, "barebox,blockdev");

	cdev = is_blockdev ? &priv->blk.cdev : &priv->cdev;

	cdev->device_node = np;

	if (is_blockdev) {
		cdev->name = np->name;
		priv->blk.dev = dev;
		priv->blk.ops = &hf_blk_ops;
		priv->blk.blockbits = SECTOR_SHIFT;
		priv->blk.num_blocks = size / SECTOR_SIZE;

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
		cdev->size = size;
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

static struct driver_d hf_drv = {
	.name  = "hostfile",
	.of_compatible = DRV_OF_COMPAT(hostfile_dt_ids),
	.probe = hf_probe,
};
device_platform_driver(hf_drv);

static int of_hostfile_fixup(struct device_node *root, void *ctx)
{
	struct hf_info *hf = ctx;
	struct device_node *node;
	uint32_t reg[] = {
		hf->base >> 32,
		hf->base,
		hf->size >> 32,
		hf->size
	};
	int ret;

	node = of_new_node(root, hf->devname);

	ret = of_property_write_string(node, "compatible", hostfile_dt_ids->compatible);
	if (ret)
		return ret;

	ret = of_property_write_u32_array(node, "reg", reg, ARRAY_SIZE(reg));
	if (ret)
		return ret;

	ret = of_property_write_u32(node, "barebox,fd", hf->fd);
	if (ret)
		return ret;

	ret = of_property_write_string(node, "barebox,filename", hf->filename);

	if (hf->is_blockdev)
		ret = of_property_write_bool(node, "barebox,blockdev", true);

	return ret;
}

int barebox_register_filedev(struct hf_info *hf)
{
	return of_register_fixup(of_hostfile_fixup, hf);
}
