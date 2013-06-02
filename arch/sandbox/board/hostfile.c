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
#include <mach/hostfile.h>
#include <xfuncs.h>

struct hf_priv {
	struct cdev cdev;
	struct hf_platform_data *pdata;
};

static ssize_t hf_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	struct hf_platform_data *hf = cdev->priv;
	int fd = hf->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_read(fd, buf, count);
}

static ssize_t hf_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	struct hf_platform_data *hf = cdev->priv;
	int fd = hf->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_write(fd, buf, count);
}

static void hf_info(struct device_d *dev)
{
	struct hf_platform_data *hf = dev->platform_data;

	printf("file: %s\n", hf->filename);
}

static struct file_operations hf_fops = {
	.read  = hf_read,
	.write = hf_write,
	.lseek = dev_lseek_default,
};

static int hf_probe(struct device_d *dev)
{
	struct hf_platform_data *hf = dev->platform_data;
	struct hf_priv *priv = xzalloc(sizeof(*priv));

	priv->pdata = hf;

	priv->cdev.name = hf->name;
	priv->cdev.size = hf->size;
	priv->cdev.ops = &hf_fops;
	priv->cdev.priv = hf;

	dev->info = hf_info;

#ifdef CONFIG_FS_DEVFS
	devfs_create(&priv->cdev);
#endif

	return 0;
}

static struct driver_d hf_drv = {
	.name  = "hostfile",
	.probe = hf_probe,
};
device_platform_driver(hf_drv);

int barebox_register_filedev(struct hf_platform_data *hf)
{
	struct device_d *dev;
	struct resource *res;

	dev = xzalloc(sizeof(*dev));
	strcpy(dev->name, "hostfile");
	dev->id = DEVICE_ID_DYNAMIC;
	dev->platform_data = hf;

	res = xzalloc(sizeof(struct resource));
	res[0].start = hf->base;
	res[0].end = hf->base + hf->size - 1;
	res[0].flags = IORESOURCE_MEM;

	dev->resource = res;
	dev->num_resources = 1;

	return sandbox_add_device(dev);
}
