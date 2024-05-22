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

struct devfs_inode {
	struct inode inode;
	struct cdev *cdev;
};

static int devfs_read(struct device *_dev, FILE *f, void *buf, size_t size)
{
	struct cdev *cdev = f->priv;

	return cdev_read(cdev, buf, size, f->pos, f->flags);
}

static int devfs_write(struct device *_dev, FILE *f, const void *buf,
		       size_t size)
{
	struct cdev *cdev = f->priv;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	return cdev_write(cdev, buf, size, f->pos, f->flags);
}

static int devfs_lseek(struct device *_dev, FILE *f, loff_t pos)
{
	struct cdev *cdev = f->priv;

	return cdev_lseek(cdev, pos);
}

static int devfs_erase(struct device *_dev, FILE *f, loff_t count,
		       loff_t offset)
{
	struct cdev *cdev = f->priv;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	if (count + offset > cdev->size)
		count = cdev->size - offset;

	return cdev_erase(cdev, count, offset);
}

static int devfs_protect(struct device *dev, FILE *f, size_t count,
			 loff_t offset, int prot)
{
	struct cdev *cdev = f->priv;

	return cdev_protect(cdev, count, offset, prot);
}

static int devfs_discard_range(struct device *dev, FILE *f, loff_t count,
			       loff_t offset)
{
	struct cdev *cdev = f->priv;

	return cdev_discard_range(cdev, count, offset);
}

static int devfs_memmap(struct device *_dev, FILE *f, void **map, int flags)
{
	struct cdev *cdev = f->priv;

	return cdev_memmap(cdev, map, flags);
}

static int devfs_open(struct device *_dev, FILE *f, const char *filename)
{
	struct inode *inode = f->f_inode;
	struct devfs_inode *node = container_of(inode, struct devfs_inode, inode);
	struct cdev *cdev = node->cdev;

	f->size = cdev->flags & DEVFS_IS_CHARACTER_DEV ?
			FILE_SIZE_STREAM : cdev->size;
	f->priv = cdev;

	return cdev_open(cdev, f->flags);
}

static int devfs_close(struct device *_dev, FILE *f)
{
	struct cdev *cdev = f->priv;

	return cdev_close(cdev);
}

static int devfs_flush(struct device *_dev, FILE *f)
{
	struct cdev *cdev = f->priv;

	return cdev_flush(cdev);
}

static int devfs_ioctl(struct device *_dev, FILE *f, unsigned int request, void *buf)
{
	struct cdev *cdev = f->priv;

	return cdev_ioctl(cdev, request, buf);
}

static int devfs_truncate(struct device *dev, FILE *f, loff_t size)
{
	struct cdev *cdev = f->priv;

	return cdev_truncate(cdev, size);
}

static struct inode *devfs_alloc_inode(struct super_block *sb)
{
	struct devfs_inode *node;

	node = xzalloc(sizeof(*node));
	if (!node)
		return NULL;

	return &node->inode;
}

static void devfs_destroy_inode(struct inode *inode)
{
	struct devfs_inode *node = container_of(inode, struct devfs_inode, inode);

	free(node);
}

static int devfs_iterate(struct file *file, struct dir_context *ctx)
{
	struct cdev *cdev;

	dir_emit_dots(file, ctx);

	for_each_cdev(cdev) {
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
	case S_IFBLK:
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
	umode_t mode;

	cdev = cdev_by_name(dentry->name);
	if (!cdev)
		return ERR_PTR(-ENOENT);

	mode = cdev_get_block_device(cdev) ? S_IFBLK : S_IFCHR;

	inode = devfs_get_inode(dir->i_sb, dir, mode);
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
	.destroy_inode = devfs_destroy_inode,
};

static int devfs_probe(struct device *dev)
{
	struct inode *inode;
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;

	sb->s_op = &devfs_ops;
	sb->s_d_op = &devfs_dentry_operations;

	inode = devfs_get_inode(sb, NULL, S_IFDIR);
	sb->s_root = d_make_root(inode);

	return 0;
}

static void devfs_delete(struct device *dev)
{
}

static struct fs_driver devfs_driver = {
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
	.discard_range = devfs_discard_range,
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
