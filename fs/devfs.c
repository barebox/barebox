/*
 * devfs.c - a device file system for barebox
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
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <command.h>
#include <errno.h>
#include <xfuncs.h>
#include <linux/stat.h>
#include <ioctl.h>
#include <nand.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/mtd-abi.h>
#include <partition.h>

extern struct list_head cdev_list;

static int devfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct cdev *cdev = f->priv;

	return cdev_read(cdev, buf, size, f->pos, f->flags);
}

static int devfs_write(struct device_d *_dev, FILE *f, const void *buf, size_t size)
{
	struct cdev *cdev = f->priv;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	return cdev_write(cdev, buf, size, f->pos, f->flags);
}

static loff_t devfs_lseek(struct device_d *_dev, FILE *f, loff_t pos)
{
	struct cdev *cdev = f->priv;
	loff_t ret = -1;

	if (cdev->ops->lseek)
		ret = cdev->ops->lseek(cdev, pos + cdev->offset);

	if (ret != -1)
		f->pos = pos;

	return ret - cdev->offset;
}

static int devfs_erase(struct device_d *_dev, FILE *f, loff_t count, loff_t offset)
{
	struct cdev *cdev = f->priv;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	if (!cdev->ops->erase)
		return -ENOSYS;

	if (count + offset > cdev->size)
		count = cdev->size - offset;

	return cdev->ops->erase(cdev, count, offset + cdev->offset);
}

static int devfs_protect(struct device_d *_dev, FILE *f, size_t count, loff_t offset, int prot)
{
	struct cdev *cdev = f->priv;

	if (!cdev->ops->protect)
		return -ENOSYS;

	return cdev->ops->protect(cdev, count, offset + cdev->offset, prot);
}

static int devfs_memmap(struct device_d *_dev, FILE *f, void **map, int flags)
{
	struct cdev *cdev = f->priv;
	int ret = -ENOSYS;

	if (!cdev->ops->memmap)
		return -EINVAL;

	ret = cdev->ops->memmap(cdev, map, flags);

	if (!ret)
		*map = (void *)((unsigned long)*map + (unsigned long)cdev->offset);

	return ret;
}

static int devfs_open(struct device_d *_dev, FILE *f, const char *filename)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_by_name(filename + 1);

	if (!cdev)
		return -ENOENT;

	f->size = cdev->flags & DEVFS_IS_CHARACTER_DEV ?
			FILE_SIZE_STREAM : cdev->size;
	f->priv = cdev;

	if (cdev->ops->open) {
		ret = cdev->ops->open(cdev, f->flags);
		if (ret)
			return ret;
	}

	cdev->open++;

	return 0;
}

static int devfs_close(struct device_d *_dev, FILE *f)
{
	struct cdev *cdev = f->priv;
	int ret;

	if (cdev->ops->close) {
		ret = cdev->ops->close(cdev);
		if (ret)
			return ret;
	}

	cdev->open--;

	return 0;
}

static int devfs_flush(struct device_d *_dev, FILE *f)
{
	struct cdev *cdev = f->priv;

	if (cdev->ops->flush)
		return cdev->ops->flush(cdev);

	return 0;
}

static int devfs_ioctl(struct device_d *_dev, FILE *f, int request, void *buf)
{
	struct cdev *cdev = f->priv;

	return cdev_ioctl(cdev, request, buf);
}

static int devfs_truncate(struct device_d *dev, FILE *f, ulong size)
{
	struct cdev *cdev = f->priv;

	if (cdev->ops->truncate)
		return cdev->ops->truncate(cdev, size);

	if (f->fsdev->dev.num_resources < 1)
		return -ENOSPC;
	if (size > resource_size(&f->fsdev->dev.resource[0]))
		return -ENOSPC;
	return 0;
}

static DIR* devfs_opendir(struct device_d *dev, const char *pathname)
{
	DIR *dir;

	dir = xzalloc(sizeof(DIR));

	if (!list_empty(&cdev_list))
		dir->priv = list_first_entry(&cdev_list, struct cdev, list);

	return dir;
}

static struct dirent* devfs_readdir(struct device_d *_dev, DIR *dir)
{
	struct cdev *cdev = dir->priv;

	if (!cdev)
		return NULL;

	list_for_each_entry_from(cdev, &cdev_list, list) {
		strcpy(dir->d.d_name, cdev->name);
		dir->priv = list_entry(cdev->list.next, struct cdev, list);
		return &dir->d;
	}
	return NULL;
}

static int devfs_closedir(struct device_d *dev, DIR *dir)
{
	free(dir);
	return 0;
}

static int devfs_stat(struct device_d *_dev, const char *filename, struct stat *s)
{
	struct cdev *cdev;

	cdev = lcdev_by_name(filename + 1);
	if (!cdev)
		return -ENOENT;

	s->st_mode = S_IFCHR;
	s->st_size = cdev->size;

	if (cdev->link)
		s->st_mode |= S_IFLNK;

	cdev = cdev_readlink(cdev);

	if (cdev->ops->write)
		s->st_mode |= S_IWUSR;
	if (cdev->ops->read)
		s->st_mode |= S_IRUSR;

	return 0;
}

static int devfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);

	if (strcmp(fsdev->path, "/dev")) {
		dev_err(dev, "devfs can only be mounted on /dev/\n");
		return -EINVAL;
	}

	return 0;
}

static void devfs_delete(struct device_d *dev)
{
}

static int devfs_readlink(struct device_d *dev, const char *pathname,
			  char *buf, size_t bufsz)
{
	struct cdev *cdev;

	cdev = cdev_by_name(pathname + 1);
	if (!cdev)
		return -ENOENT;

	while (cdev->link)
		cdev = cdev->link;

	bufsz = min(bufsz, strlen(cdev->name));
	memcpy(buf, cdev->name, bufsz);

	return 0;
}

static struct fs_driver_d devfs_driver = {
	.read      = devfs_read,
	.write     = devfs_write,
	.lseek     = devfs_lseek,
	.open      = devfs_open,
	.close     = devfs_close,
	.flush     = devfs_flush,
	.ioctl     = devfs_ioctl,
	.opendir   = devfs_opendir,
	.readdir   = devfs_readdir,
	.truncate  = devfs_truncate,
	.closedir  = devfs_closedir,
	.stat      = devfs_stat,
	.erase     = devfs_erase,
	.protect   = devfs_protect,
	.memmap    = devfs_memmap,
	.readlink  = devfs_readlink,
	.flags     = FS_DRIVER_NO_DEV,
	.drv = {
		.probe  = devfs_probe,
		.remove = devfs_delete,
		.name = "devfs",
	}
};

static int devfs_init(void)
{
	return register_fs_driver(&devfs_driver);
}

coredevice_initcall(devfs_init);
