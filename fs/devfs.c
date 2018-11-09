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

struct devfs_inode {
	struct inode inode;
	struct cdev *cdev;
};

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
	struct inode *inode = f->f_inode;
	struct devfs_inode *node = container_of(inode, struct devfs_inode, inode);
	struct cdev *cdev = node->cdev;
	int ret;

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

static struct inode *devfs_alloc_inode(struct super_block *sb)
{
	struct devfs_inode *node;

	node = xzalloc(sizeof(*node));
	if (!node)
		return NULL;

	return &node->inode;
}

static int devfs_iterate(struct file *file, struct dir_context *ctx)
{
	struct cdev *cdev;

	dir_emit_dots(file, ctx);

	list_for_each_entry(cdev, &cdev_list, list) {
		dir_emit(ctx, cdev->name, strlen(cdev->name),
				1 /* FIXME */, DT_REG);
	}

        return 0;
}

static const struct inode_operations devfs_file_inode_operations;
static const struct file_operations devfs_dir_operations;
static const struct inode_operations devfs_dir_inode_operations;
static const struct file_operations devfs_file_operations;

static int devfs_lookup_revalidate(struct dentry *dentry, unsigned int flags)
{
	struct devfs_inode *dinode;
	struct inode *inode;
	struct cdev *cdev;

	cdev = cdev_by_name(dentry->name);
	if (!cdev)
		return -ENOENT;

	inode = d_inode(dentry);
	if (!inode)
		return 0;

	dinode = container_of(inode, struct devfs_inode, inode);

	if (dinode->cdev != cdev)
		return 0;

	return 1;
}

static const struct dentry_operations devfs_dentry_operations = {
	.d_revalidate = devfs_lookup_revalidate,
};

static struct inode *devfs_get_inode(struct super_block *sb, const struct inode *dir,
                                     umode_t mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode->i_mode = mode;

	switch (mode & S_IFMT) {
	default:
		return NULL;
	case S_IFCHR:
		inode->i_op = &devfs_file_inode_operations;
		inode->i_fop = &devfs_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &devfs_dir_inode_operations;
		inode->i_fop = &devfs_dir_operations;
		inc_nlink(inode);
		break;
	}

	return inode;
}

static struct dentry *devfs_lookup(struct inode *dir, struct dentry *dentry,
				   unsigned int flags)
{
	struct devfs_inode *dinode;
	struct inode *inode;
	struct cdev *cdev;

	cdev = cdev_by_name(dentry->name);
	if (!cdev)
		return ERR_PTR(-ENOENT);

	inode = devfs_get_inode(dir->i_sb, dir, S_IFCHR);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	if (cdev->ops->write)
		inode->i_mode |= S_IWUSR;
	if (cdev->ops->read)
		inode->i_mode |= S_IRUSR;

	dinode = container_of(inode, struct devfs_inode, inode);

	inode->i_size = cdev->size;
	dinode->cdev = cdev;

	d_add(dentry, inode);

	return NULL;
}

static const struct file_operations devfs_dir_operations = {
	.iterate = devfs_iterate,
};

static const struct inode_operations devfs_dir_inode_operations =
{
	.lookup = devfs_lookup,
};

static const struct super_operations devfs_ops = {
	.alloc_inode = devfs_alloc_inode,
};

static int devfs_probe(struct device_d *dev)
{
	struct inode *inode;
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;

	sb->s_op = &devfs_ops;
	sb->s_d_op = &devfs_dentry_operations;

	inode = devfs_get_inode(sb, NULL, S_IFDIR);
	sb->s_root = d_make_root(inode);

	return 0;
}

static void devfs_delete(struct device_d *dev)
{
}

static struct fs_driver_d devfs_driver = {
	.read      = devfs_read,
	.write     = devfs_write,
	.lseek     = devfs_lseek,
	.open      = devfs_open,
	.close     = devfs_close,
	.flush     = devfs_flush,
	.ioctl     = devfs_ioctl,
	.truncate  = devfs_truncate,
	.erase     = devfs_erase,
	.protect   = devfs_protect,
	.memmap    = devfs_memmap,
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
