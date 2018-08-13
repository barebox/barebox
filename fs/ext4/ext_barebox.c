/*
 * barebox ext4 support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <linux/stat.h>
#include <linux/ctype.h>
#include <xfuncs.h>
#include <fcntl.h>
#include "ext4_common.h"

int ext4fs_devread(struct ext_filesystem *fs, int __sector, int byte_offset,
		int byte_len, char *buf)
{
	ssize_t size;
	uint64_t sector = __sector;

	size = cdev_read(fs->cdev, buf, byte_len, sector * SECTOR_SIZE + byte_offset, 0);
	if (size < 0) {
		dev_err(fs->dev, "read error at sector %d: %s\n", __sector,
				strerror(-size));
		return size;
	}

	return 0;
}

static inline struct ext2fs_node *to_ext2_node(struct inode *inode)
{
	return container_of(inode, struct ext2fs_node, i);
}

static int ext_read(struct device_d *_dev, FILE *f, void *buf, size_t insize)
{
	struct inode *inode = f->f_inode;
	struct ext2fs_node *node = to_ext2_node(inode);

	return ext4fs_read_file(node, f->pos, insize, buf);
}

static loff_t ext_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;

	return f->pos;
}

static struct inode *ext_alloc_inode(struct super_block *sb)
{
	struct fs_device_d *fsdev = container_of(sb, struct fs_device_d, sb);
	struct ext_filesystem *fs = fsdev->dev.priv;
	struct ext2fs_node *node;

	node = xzalloc(sizeof(*node));
	if (!node)
		return NULL;

	node->data = fs->data;

	return &node->i;
}

static const struct super_operations ext_ops = {
	.alloc_inode = ext_alloc_inode,
};

struct inode *ext_get_inode(struct super_block *sb, int ino);

static int ext4fs_get_ino(struct ext2fs_node *dir, struct qstr *name, int *inum)
{
	unsigned int fpos = 0;
	int ret;

	while (fpos < le32_to_cpu(dir->inode.size)) {
		struct ext2_dirent dirent;

		ret = ext4fs_read_file(dir, fpos, sizeof(dirent), (char *)&dirent);
		if (ret < 1)
			return -EINVAL;

		if (dirent.namelen != 0) {
			char filename[dirent.namelen];
			int ino;

			ret = ext4fs_read_file(dir, fpos + sizeof(dirent),
						  dirent.namelen, filename);
			if (ret < 1)
				return -EINVAL;

			ino = le32_to_cpu(dirent.inode);

			if (name->len == dirent.namelen &&
			    !strncmp(name->name, filename, name->len)) {
				*inum = ino;
				return 0;
			}
		}
		fpos += le16_to_cpu(dirent.direntlen);
	}

	*inum = 0;

	return 0;
}

static struct dentry *ext_lookup(struct inode *dir, struct dentry *dentry,
				 unsigned int flags)
{
	struct ext2fs_node *e2dir = to_ext2_node(dir);
	int ret, ino;
	struct inode *inode;

	ret = ext4fs_get_ino(e2dir, &dentry->d_name, &ino);
	if (ret)
		return ERR_PTR(ret);

	if (ino) {
		inode = ext_get_inode(dir->i_sb, ino);

		d_add(dentry, inode);
	}

	return NULL;
}

static const struct inode_operations ext_inode_operations = {
	.lookup = ext_lookup,
};

static int ext_iterate(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *dir = d_inode(dentry);
	unsigned int fpos = 0;
	int status, ret;
	struct ext2fs_node *diro = to_ext2_node(dir);
	void *buf;

	buf = malloc(dir->i_size);
	if (!buf)
		return -ENOMEM;

	status = ext4fs_read_file(diro, 0, dir->i_size, buf);
	if (status < 1) {
		ret = -EINVAL;
		goto out;
	}

	while (fpos < dir->i_size) {
		const struct ext2_dirent *dirent = buf + fpos;
		const char *filename = buf + fpos + sizeof(*dirent);

		if (dirent->namelen != 0)
			dir_emit(ctx, filename, dirent->namelen,
				 le32_to_cpu(dirent->inode), DT_UNKNOWN);

		fpos += le16_to_cpu(dirent->direntlen);
	}
	ret = 0;
out:
	free(buf);

	return ret;

}

const struct file_operations ext_dir_operations = {
	.iterate = ext_iterate,
};

static const char *ext_get_link(struct dentry *dentry, struct inode *inode)
{
	struct ext2fs_node *node = to_ext2_node(inode);
	int ret;

	if (inode->i_size < sizeof(node->inode.b.symlink))
		return inode->i_link;

	BUG_ON(inode->i_link);

	inode->i_link = zalloc(inode->i_size + 1);

	ret = ext4fs_read_file(node, 0, inode->i_size, inode->i_link);
	if (ret == 0) {
		free(inode->i_link);
		inode->i_link = NULL;
	}

	return inode->i_link;
}

static const struct inode_operations ext_symlink_inode_operations =
{
	.get_link = ext_get_link,
};

struct inode *ext_get_inode(struct super_block *sb, int ino)
{
	struct inode *inode;
	struct ext2fs_node *node;
	struct fs_device_d *fsdev = container_of(sb, struct fs_device_d, sb);
	struct ext_filesystem *fs = fsdev->dev.priv;
	int ret;

	inode = new_inode(sb);

	node = container_of(inode, struct ext2fs_node, i);

	ret = ext4fs_read_inode(fs->data, ino, &node->inode);

	inode->i_ino = ino;
	inode->i_mode = le16_to_cpu(node->inode.mode);
	inode->i_size = le32_to_cpu(node->inode.size);

	switch (inode->i_mode & S_IFMT) {
	default:
		return NULL;
	case S_IFREG:
		inode->i_op = &ext_inode_operations;
		break;
	case S_IFDIR:
		inode->i_op = &ext_inode_operations;
		inode->i_fop = &ext_dir_operations;
		inc_nlink(inode);
		break;
	case S_IFLNK:
		inode->i_op = &ext_symlink_inode_operations;
		if (inode->i_size < sizeof(node->inode.b.symlink)) {
			inode->i_link = zalloc(inode->i_size + 1);
			strncpy(inode->i_link, node->inode.b.symlink,
				inode->i_size);
		}
		break;
	}

	return inode;
}

static int ext_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	int ret;
	struct ext_filesystem *fs;
	struct super_block *sb = &fsdev->sb;
	struct inode *inode;

	fs = xzalloc(sizeof(*fs));

	dev->priv = fs;
	fs->dev = dev;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_open;

	fs->cdev = fsdev->cdev;

	ret = ext4fs_mount(fs);
	if (ret)
		goto err_mount;

	sb->s_op = &ext_ops;

	inode = ext_get_inode(sb, 2);
	sb->s_root = d_make_root(inode);

	return 0;

err_mount:
err_open:
	free(fs);

	return ret;
}

static void ext_remove(struct device_d *dev)
{
	struct ext_filesystem *fs = dev->priv;

	ext4fs_umount(fs);
	free(fs);
}

static struct fs_driver_d ext_driver = {
	.read      = ext_read,
	.lseek     = ext_lseek,
	.type      = filetype_ext,
	.flags     = 0,
	.drv = {
		.probe  = ext_probe,
		.remove = ext_remove,
		.name = "ext4",
	}
};

static int ext_init(void)
{
	return register_fs_driver(&ext_driver);
}

coredevice_initcall(ext_init);
