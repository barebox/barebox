/*
 * smhfs.c -- Driver implementing pseudo FS interface on top of ARM
 *            semihosting
 *
 * Copyright (c) 2015 Zodiac Inflight Innovations
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <fs.h>
#include <errno.h>
#include <linux/stat.h>
#include <asm/semihosting.h>

static int file_to_fd(const struct file *f)
{
	return (int)(uintptr_t)f->private_data;
}

static int smhfs_create(struct device __always_unused *dev,
			const char __always_unused *pathname,
			mode_t __always_unused mode)
{
	return 0;
}

static int smhfs_mkdir(struct device __always_unused *dev,
		       const char __always_unused *pathname)
{
	return -ENOSYS;
}

static int smhfs_rm(struct device __always_unused *dev,
		    const char *pathname)
{
	/* Get rid of leading '/' */
	pathname = &pathname[1];

	return semihosting_remove(pathname);
}

static int smhfs_truncate(struct device __always_unused *dev,
			  struct file __always_unused *f,
			  loff_t __always_unused size)
{
	return 0;
}

static int smhfs_open(struct device __always_unused *dev,
		      struct file *file, const char *filename)
{
	int fd;
	/* Get rid of leading '/' */
	filename = &filename[1];

	fd = semihosting_open(filename, file->f_flags);
	if (fd < 0)
		return fd;

	file->private_data = (void *)(uintptr_t)fd;
	file->f_size = semihosting_flen(fd);
	if (file->f_size < 0)
		return file->f_size;

	return 0;
}

static int smhfs_close(struct device __always_unused *dev,
		       struct file *f)
{
	return semihosting_close(file_to_fd(f));
}

static int smhfs_write(struct device __always_unused *dev,
		       struct file *f, const void *inbuf, size_t insize)
{
	long ret = semihosting_write(file_to_fd(f), inbuf, insize);
	return ret < 0 ? ret : insize;
}

static int smhfs_read(struct device __always_unused *dev,
		      struct file *f, void *buf, size_t insize)
{
	long ret = semihosting_read(file_to_fd(f), buf, insize);
	return ret < 0 ? ret : insize;
}

static int smhfs_lseek(struct device __always_unused *dev,
			  struct file *f, loff_t pos)
{
	return semihosting_seek(file_to_fd(f), pos);
}

static DIR* smhfs_opendir(struct device __always_unused *dev,
			  const char __always_unused *pathname)
{
	return NULL;
}

static int smhfs_stat(struct device __always_unused *dev,
		      const char *filename, struct stat *s)
{
	struct file file;

	if (smhfs_open(NULL, &file, filename) == 0) {
		s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		s->st_size = file.f_size;
	}
	smhfs_close(NULL, &file);

	return 0;
}

static int smhfs_probe(struct device __always_unused *dev)
{
	/* TODO: Add provisions to detect if debugger is connected */
	return 0;
}

static void smhfs_remove(struct device __always_unused *dev)
{
}

static const struct fs_legacy_ops smhfs_ops = {
	.opendir   = smhfs_opendir,
	.stat      = smhfs_stat,
	.create    = smhfs_create,
	.unlink    = smhfs_rm,
	.mkdir     = smhfs_mkdir,
	.rmdir     = smhfs_rm,
};

static struct fs_driver smhfs_driver = {
	.open      = smhfs_open,
	.close     = smhfs_close,
	.read      = smhfs_read,
	.lseek     = smhfs_lseek,
	.write     = smhfs_write,
	.truncate  = smhfs_truncate,
	.legacy_ops = &smhfs_ops,
	.flags     = FS_DRIVER_NO_DEV,
	.drv = {
		.probe  = smhfs_probe,
		.remove = smhfs_remove,
		.name = "smhfs",
	}
};

static int smhfs_init(void)
{
	return register_fs_driver(&smhfs_driver);
}
coredevice_initcall(smhfs_init);
