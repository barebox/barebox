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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static ssize_t hf_read(struct cdev *cdev, void *buf, size_t count, ulong offset, ulong flags)
{
	struct hf_platform_data *hf = cdev->priv;
	int fd = hf->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_read(fd, buf, count);
}

static ssize_t hf_write(struct cdev *cdev, const void *buf, size_t count, ulong offset, ulong flags)
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
#ifdef CONFIG_FS_DEVFS
	devfs_create(&priv->cdev);
#endif

	return 0;
}

static struct driver_d hf_drv = {
	.name  = "hostfile",
	.probe = hf_probe,
	.info  = hf_info,
};

static int hf_init(void)
{
	return register_driver(&hf_drv);
}

device_initcall(hf_init);

int barebox_register_filedev(struct hf_platform_data *hf)
{
	struct device_d *dev;

	dev = xzalloc(sizeof(struct device_d));

	dev->platform_data = hf;

	strcpy(dev->name, "hostfile");
	dev->size = hf->size;
	dev->id = -1;
	dev->map_base = hf->map_base;

	return register_device(dev);
}

