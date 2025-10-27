/*
 * fat.c - FAT filesystem barebox driver
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include "ff.h"
#include "integer.h"
#include "diskio.h"

struct fat_priv {
	struct cdev *cdev;
	FATFS fat;
};

/* ---------------------------------------------------------------*/

DRESULT disk_read(FATFS *fat, BYTE *buf, DWORD sector, BYTE count)
{
	struct fat_priv *priv = fat->userdata;
	int ret;

	debug("%s: sector: %ld count: %d\n", __func__, sector, count);

	ret = cdev_read(priv->cdev, buf, count << 9, (loff_t)sector * 512, 0);
	if (ret != count << 9)
		return ret;

	return 0;
}

DRESULT disk_write(FATFS *fat, const BYTE *buf, DWORD sector, BYTE count)
{
	struct fat_priv *priv = fat->userdata;
	int ret;

	debug("%s: buf: %p sector: %ld count: %d\n",
			__func__, buf, sector, count);

	ret = cdev_write(priv->cdev, buf, count << 9, (loff_t)sector * 512, 0);
	if (ret != count << 9)
		return ret;

	return 0;
}

/* ---------------------------------------------------------------*/

#ifdef CONFIG_FS_FAT_WRITE
static int fat_create(struct device *dev, const char *pathname, mode_t mode)
{
	struct fat_priv *priv = dev->priv;
	FIL f_file;
	int ret;

	ret = f_open(&priv->fat, &f_file, pathname, FA_OPEN_ALWAYS);
	if (ret)
		return -EINVAL;

	f_close(&f_file);

	return 0;
}

static int fat_unlink(struct device *dev, const char *pathname)
{
	struct fat_priv *priv = dev->priv;
	int ret;

	ret = f_unlink(&priv->fat, pathname);
	if (ret)
		return ret;

	cdev_flush(priv->cdev);

	return 0;
}

static int fat_mkdir(struct device *dev, const char *pathname)
{
	struct fat_priv *priv = dev->priv;
	int ret;

	ret = f_mkdir(&priv->fat, pathname);
	if (ret)
		return ret;

	cdev_flush(priv->cdev);

	return 0;
}

static int fat_rmdir(struct device *dev, const char *pathname)
{
	struct fat_priv *priv = dev->priv;
	int ret;

	ret = f_unlink(&priv->fat, pathname);
	if (ret)
		return ret;

	cdev_flush(priv->cdev);

	return 0;
}

static int fat_write(struct file *f, const void *buf, size_t insize)
{
	FIL *f_file = f->private_data;
	int outsize;
	int ret;

	ret = f_write(f_file, buf, insize, &outsize);

	debug("%s: %d %zd %d %p\n", __func__, ret, insize, outsize, f_file);

	if (ret)
		return ret;
	if (!outsize)
		return -ENOSPC;

	return outsize;
}

static int fat_truncate(struct file *f, loff_t size)
{
	FIL *f_file = f->private_data;
	unsigned long lastofs;
	int ret;

	lastofs = f_file->fptr;

	ret = f_lseek(f_file, size);
	if (ret)
		return ret;

	ret = f_truncate(f_file);
	if (ret)
		return ret;

	ret = f_lseek(f_file, lastofs);
	if (ret)
		return ret;

	return 0;
}
#endif /* CONFIG_FS_FAT_WRITE */

static int fat_open(struct device *dev, struct file *file, const char *filename)
{
	struct fat_priv *priv = dev->priv;
	FIL *f_file;
	int ret;
	unsigned long flags = 0;

	f_file = xzalloc(sizeof(*f_file));

	switch (file->f_flags & O_ACCMODE) {
	case O_RDONLY:
		flags = FA_READ;
		break;
	case O_WRONLY:
		flags = FA_WRITE;
		break;
	case O_RDWR:
		flags = FA_READ | FA_WRITE;
		break;
	}

	ret = f_open(&priv->fat, f_file, filename, flags);
	if (ret) {
		free(f_file);
		return -EINVAL;
	}

	if (file->f_flags & O_APPEND) {
		ret = f_lseek(f_file, f_file->fsize);
		if (ret) {
			f_close(f_file);
			free(f_file);
			return -EINVAL;
		}
	}

	file->private_data = f_file;
	file->f_size = f_file->fsize;

	return 0;
}

static int fat_close(struct device *dev, struct file *f)
{
	struct fat_priv *priv = dev->priv;
	FIL *f_file = f->private_data;

	f_close(f_file);

	free(f_file);

	cdev_flush(priv->cdev);

	return 0;
}

static int fat_read(struct file *f, void *buf, size_t insize)
{
	int ret;
	FIL *f_file = f->private_data;
	int outsize;

	ret = f_read(f_file, buf, insize, &outsize);

	debug("%s: %d %zd %d %p\n", __func__, ret, insize, outsize, f_file);

	if (ret)
		return ret;

	return outsize;
}

static int fat_lseek(struct file *f, loff_t pos)
{
	FIL *f_file = f->private_data;
	int ret;

	ret = f_lseek(f_file, pos);
	if (ret)
		return ret;

	return 0;
}

static DIR* fat_opendir(struct device *dev, const char *pathname)
{
	struct fat_priv *priv = dev->priv;
	DIR *dir;
	FF_DIR *ff_dir;
	int ret;

	debug("%s: %s\n", __func__, pathname);

	ff_dir = xzalloc(sizeof(*ff_dir));
	if (pathname)
		ret = f_opendir(&priv->fat, ff_dir, pathname);
	else
		ret = f_opendir(&priv->fat, ff_dir, "/");

	if (ret)
		return NULL;

	dir = xzalloc(sizeof(*dir));

	dir->priv = ff_dir;

	return dir;
}

static struct dirent* fat_readdir(struct device *dev, DIR *dir)
{
	FF_DIR *ff_dir = dir->priv;
	FILINFO finfo;
	int ret;
#ifdef CONFIG_FS_FAT_LFN
	char name[PATH_MAX];
#endif
	memset(&finfo, 0, sizeof(finfo));
#ifdef CONFIG_FS_FAT_LFN
	finfo.lfname = name;
	finfo.lfsize = PATH_MAX;
#endif
	ret = f_readdir(ff_dir, &finfo);
	if (ret)
		return NULL;

	if (finfo.fname[0] == '\0')
		return NULL;

#ifdef CONFIG_FS_FAT_LFN
	if (*finfo.lfname)
		strcpy(dir->d.d_name, finfo.lfname);
	else
#endif
		strcpy(dir->d.d_name, finfo.fname);

	return &dir->d;
}

static int fat_closedir(struct device *dev, DIR *dir)
{
	FF_DIR *ff_dir = dir->priv;

	free(ff_dir);
	free(dir);

	return 0;
}

static int fat_stat(struct device *dev, const char *filename, struct stat *s)
{
	struct fat_priv *priv = dev->priv;
	FILINFO finfo;
	int ret;

	memset(&finfo, 0, sizeof(FILINFO));

	ret = f_stat(&priv->fat, filename, &finfo);
	if (ret)
		return ret;

	s->st_size = finfo.fsize;
	s->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;

	if (finfo.fattrib & AM_DIR)
		s->st_mode |= S_IFDIR;
	else
		s->st_mode |= S_IFREG;

	return 0;
}

static int fat_probe(struct device *dev)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct fat_priv *priv = xzalloc(sizeof(struct fat_priv));
	int ret;

	dev->priv = priv;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_open;

	priv->cdev = fsdev->cdev;

	priv->fat.userdata = priv;
	ret = f_mount(&priv->fat);
	if (ret)
		goto err_mount;

	return 0;

err_mount:
err_open:
	free(priv);

	return ret;
}

static void fat_remove(struct device *dev)
{
	free(dev->priv);
}

static const struct fs_legacy_ops fat_ops = {
	.open      = fat_open,
	.close     = fat_close,
	.opendir   = fat_opendir,
	.readdir   = fat_readdir,
	.closedir  = fat_closedir,
	.stat      = fat_stat,
#ifdef CONFIG_FS_FAT_WRITE
	.create    = fat_create,
	.unlink    = fat_unlink,
	.mkdir     = fat_mkdir,
	.rmdir     = fat_rmdir,
	.write     = fat_write,
	.truncate  = fat_truncate,
#endif
	.read      = fat_read,
	.lseek     = fat_lseek,
};

static struct fs_driver fat_driver = {
	.legacy_ops = &fat_ops,
	.type = filetype_fat,
	.drv = {
		.probe  = fat_probe,
		.remove = fat_remove,
		.name = "fat",
	}
};

static int fat_init(void)
{
	return register_fs_driver(&fat_driver);
}

coredevice_initcall(fat_init);
