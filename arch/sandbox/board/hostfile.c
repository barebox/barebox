/*
 * hostfile.c - use files from the host to simalute barebox devices
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <driver.h>
#include <malloc.h>
#include <mach/linux.h>
#include <init.h>
#include <errno.h>
#include <linux/err.h>
#include <mach/hostfile.h>
#include <xfuncs.h>

#include <linux/err.h>

struct hf_priv {
	struct cdev cdev;
	const char *filename;
	int fd;
};

static ssize_t hf_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	struct hf_priv *priv= cdev->priv;
	int fd = priv->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_read(fd, buf, count);
}

static ssize_t hf_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	struct hf_priv *priv = cdev->priv;
	int fd = priv->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_write(fd, buf, count);
}

static void hf_info(struct device_d *dev)
{
	struct hf_priv *priv = dev->priv;

	printf("file: %s\n", priv->filename);
}

static struct cdev_operations hf_fops = {
	.read  = hf_read,
	.write = hf_write,
	.lseek = dev_lseek_default,
};

static int hf_probe(struct device_d *dev)
{
	struct hf_priv *priv = xzalloc(sizeof(*priv));
	struct resource *res;
	int err;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	priv->cdev.size = resource_size(res);

	if (!dev->device_node)
		return -ENODEV;

	err = of_property_read_u32(dev->device_node, "barebox,fd", &priv->fd);
	if (err)
		return err;

	err = of_property_read_string(dev->device_node, "barebox,filename",
				      &priv->filename);
	if (err)
		return err;

	priv->cdev.name = dev->device_node->name;
	priv->cdev.dev = dev;
	priv->cdev.ops = &hf_fops;
	priv->cdev.priv = priv;

	dev->info = hf_info;
	dev->priv = priv;

	return devfs_create(&priv->cdev);
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
coredevice_platform_driver(hf_drv);

static int of_hostfile_fixup(struct device_node *root, void *ctx)
{
	struct hf_info *hf = ctx;
	struct device_node *node;
	uint32_t reg[] = {
		hf->base >> 32,
		hf->base,
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

	return ret;
}

int barebox_register_filedev(struct hf_info *hf)
{
	return of_register_fixup(of_hostfile_fixup, hf);
}
