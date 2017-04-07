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

static int ext_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct ext_filesystem *fs = dev->priv;
	struct ext2fs_node *inode;
	int ret;

	ret = ext4fs_open(fs->data, filename, &inode);
	if (ret)
		return ret;

	file->size = le32_to_cpu(inode->inode.size);
	file->priv = inode;

	return 0;
}

static int ext_close(struct device_d *dev, FILE *f)
{
	struct ext_filesystem *fs = dev->priv;

	ext4fs_free_node(f->priv, &fs->data->diropen);

	return 0;
}

static int ext_read(struct device_d *_dev, FILE *f, void *buf, size_t insize)
{
	return ext4fs_read_file(f->priv, f->pos, insize, buf);
}

static loff_t ext_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;

	return f->pos;
}

struct ext4fs_dir {
	struct ext2fs_node *dirnode;
	int fpos;
	DIR dir;
};

static DIR *ext_opendir(struct device_d *dev, const char *pathname)
{
	struct ext_filesystem *fs = dev->priv;
	struct ext4fs_dir *ext4_dir;
	int type, ret;

	ext4_dir = xzalloc(sizeof(*ext4_dir));

	ret = ext4fs_find_file(pathname, &fs->data->diropen, &ext4_dir->dirnode,
				  &type);
	if (ret) {
		free(ext4_dir);
		return NULL;
	}

	if (type != FILETYPE_DIRECTORY)
		return NULL;

	ext4_dir->dir.priv = ext4_dir;

	ret = ext4fs_read_inode(ext4_dir->dirnode->data, ext4_dir->dirnode->ino,
			&ext4_dir->dirnode->inode);
	if (ret) {
		ext4fs_free_node(ext4_dir->dirnode, &fs->data->diropen);
		free(ext4_dir);

		return NULL;
	}

	return &ext4_dir->dir;
}

static struct dirent *ext_readdir(struct device_d *dev, DIR *dir)
{
	struct ext4fs_dir *ext4_dir = dir->priv;
	struct ext2_dirent dirent;
	struct ext2fs_node *diro = ext4_dir->dirnode;
	int ret;
	char *filename;

	if (ext4_dir->fpos >= le32_to_cpu(diro->inode.size))
		return NULL;

	ret = ext4fs_read_file(diro, ext4_dir->fpos, sizeof(struct ext2_dirent),
			(char *) &dirent);
	if (ret < 0)
		return NULL;

	if (dirent.namelen == 0)
		return NULL;

	filename = xzalloc(dirent.namelen + 1);

	ret = ext4fs_read_file(diro, ext4_dir->fpos + sizeof(struct ext2_dirent),
			dirent.namelen, filename);
	if (ret < 0) {
		free(filename);
		return NULL;
	}

	filename[dirent.namelen] = '\0';

	ext4_dir->fpos += le16_to_cpu(dirent.direntlen);

	strcpy(dir->d.d_name, filename);

	free(filename);

	return &dir->d;
}

static int ext_closedir(struct device_d *dev, DIR *dir)
{
	struct ext_filesystem *fs = dev->priv;
	struct ext4fs_dir *ext4_dir = dir->priv;

	ext4fs_free_node(ext4_dir->dirnode, &fs->data->diropen);

	free(ext4_dir);

	return 0;
}

static int ext_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct ext_filesystem *fs = dev->priv;
	struct ext2fs_node *node;
	int status, ret;

	status = ext4fs_find_file(filename, &fs->data->diropen, &node, NULL);
	if (status)
		return -ENOENT;

	ret = ext4fs_read_inode(node->data, node->ino, &node->inode);
	if (ret)
		return ret;

	s->st_size = le32_to_cpu(node->inode.size);
	s->st_mode = le16_to_cpu(node->inode.mode);

	ext4fs_free_node(node, &fs->data->diropen);

	return 0;
}

static int ext_readlink(struct device_d *dev, const char *pathname,
		char *buf, size_t bufsiz)
{
	struct ext_filesystem *fs = dev->priv;
	struct ext2fs_node *node;
	char *symlink;
	int ret, len, type;

	ret = ext4fs_find_file(pathname, &fs->data->diropen, &node, &type);
	if (ret)
		return ret;

	if (type != FILETYPE_SYMLINK)
		return -EINVAL;

	symlink = ext4fs_read_symlink(node);
	if (!symlink)
		return -ENOENT;

	len = min(bufsiz, strlen(symlink));

	memcpy(buf, symlink, len);

	free(symlink);

	return 0;
}

static int ext_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	int ret;
	struct ext_filesystem *fs;

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
	.open      = ext_open,
	.close     = ext_close,
	.read      = ext_read,
	.lseek     = ext_lseek,
	.opendir   = ext_opendir,
	.readdir   = ext_readdir,
	.closedir  = ext_closedir,
	.stat      = ext_stat,
	.readlink  = ext_readlink,
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
