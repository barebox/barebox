/*
 * devfs.c - a device file system for barebox
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
#include <block.h>
#include <stringlist.h>

struct devfs_inode {
	struct inode inode;
	struct cdev *cdev;
};

static int devfs_read(struct file *f, void *buf, size_t size)
{
	struct cdev *cdev = f->private_data;

	return cdev_read(cdev, buf, size, f->f_pos, f->f_flags);
}

static int devfs_write(struct file *f, const void *buf,
		       size_t size)
{
	struct cdev *cdev = f->private_data;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	return cdev_write(cdev, buf, size, f->f_pos, f->f_flags);
}

static int devfs_lseek(struct file *f, loff_t pos)
{
	struct cdev *cdev = f->private_data;

	return cdev_lseek(cdev, pos);
}

static int devfs_erase(struct file *f, loff_t count,
		       loff_t offset, enum erase_type type)
{
	struct cdev *cdev = f->private_data;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	if ((type == ERASE_TO_WRITE) && (cdev->flags & DEVFS_WRITE_AUTOERASE))
		return 0;

	if (count + offset > cdev->size)
		count = cdev->size - offset;

	return cdev_erase(cdev, count, offset);
}

static int devfs_protect(struct file *f, size_t count, loff_t offset, int prot)
{
	struct cdev *cdev = f->private_data;

	return cdev_protect(cdev, count, offset, prot);
}

static int devfs_discard_range(struct file *f, loff_t count, loff_t offset)
{
	struct cdev *cdev = f->private_data;

	return cdev_discard_range(cdev, count, offset);
}

static int devfs_memmap(struct file *f, void **map, int flags)
{
	struct cdev *cdev = f->private_data;

	return cdev_memmap(cdev, map, flags);
}

static int devfs_open(struct inode *inode, struct file *f)
{
	struct devfs_inode *node = container_of(inode, struct devfs_inode, inode);
	struct cdev *cdev = node->cdev;

	if (inode->cdevname) {
		cdev = cdev_by_name(inode->cdevname);
		if (!cdev)
			return -ENOENT;
	}

	f->f_size = cdev_size(cdev);
	f->private_data = cdev;

	return cdev_open(cdev, f->f_flags);
}

static int devfs_close(struct inode *inode, struct file *f)
{
	struct cdev *cdev = f->private_data;

	return cdev_close(cdev);
}

static int devfs_flush(struct file *f)
{
	struct cdev *cdev = f->private_data;

	return cdev_flush(cdev);
}

static int devfs_ioctl(struct file *f, unsigned int request, void *buf)
{
	struct cdev *cdev = f->private_data;

	return cdev_ioctl(cdev, request, buf);
}

static int devfs_truncate(struct file *f, loff_t size)
{
	struct cdev *cdev = f->private_data;

	return cdev_truncate(cdev, size);
}

static const struct file_operations devfs_file_operations = {
	.open = devfs_open,
	.release = devfs_close,
	.read = devfs_read,
	.write = devfs_write,
	.lseek = devfs_lseek,
	.flush = devfs_flush,
	.ioctl = devfs_ioctl,
	.truncate = devfs_truncate,
	.erase = devfs_erase,
	.protect = devfs_protect,
	.discard_range = devfs_discard_range,
	.memmap = devfs_memmap,
};

void init_special_inode(struct inode *inode, umode_t mode, const char *cdevname)
{
	inode->i_mode = mode;
	inode->i_fop = &devfs_file_operations;
	inode->cdevname = xstrdup_const(cdevname);
}
