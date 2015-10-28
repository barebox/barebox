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

static int file_to_fd(const FILE *f)
{
	return (int)f->priv;
}

static int smhfs_create(struct device_d __always_unused *dev,
			const char __always_unused *pathname,
			mode_t __always_unused mode)
{
	return 0;
}

static int smhfs_mkdir(struct device_d __always_unused *dev,
		       const char __always_unused *pathname)
{
	return -ENOSYS;
}

static int smhfs_rm(struct device_d __always_unused *dev,
		    const char *pathname)
{
	/* Get rid of leading '/' */
	pathname = &pathname[1];

	if (semihosting_remove(pathname) != 0)
		return -semihosting_errno();
	else
		return 0;
}

static int smhfs_truncate(struct device_d __always_unused *dev,
			  FILE __always_unused *f,
			  ulong __always_unused size)
{
	return -ENOSYS;
}

static int smhfs_open(struct device_d __always_unused *dev,
		      FILE *file, const char *filename)
{
	int fd;
	/* Get rid of leading '/' */
	filename = &filename[1];

	fd = semihosting_open(filename, file->flags);
	if (fd < 0)
		goto error;

	file->priv = (void *)fd;
	file->size = semihosting_flen(fd);
	if (file->size < 0)
		goto error;

	return 0;
error:
	return -semihosting_errno();
}

static int smhfs_close(struct device_d __always_unused *dev,
		       FILE *f)
{
	if (semihosting_close(file_to_fd(f)))
		return -semihosting_errno();
	else
		return 0;
}

static int smhfs_write(struct device_d __always_unused *dev,
		       FILE *f, const void *inbuf, size_t insize)
{
	if (semihosting_write(file_to_fd(f), inbuf, insize))
		return -semihosting_errno();
	else
		return insize;
}

static int smhfs_read(struct device_d __always_unused *dev,
		      FILE *f, void *buf, size_t insize)
{
	if (!semihosting_read(file_to_fd(f), buf, insize))
		return insize;
	else
		return -semihosting_errno();
}

static loff_t smhfs_lseek(struct device_d __always_unused *dev,
			  FILE *f, loff_t pos)
{
	if (semihosting_seek(file_to_fd(f), pos)) {
		return -semihosting_errno();
	} else {
		f->pos = pos;
		return f->pos;
	}
}

static DIR* smhfs_opendir(struct device_d __always_unused *dev,
			  const char __always_unused *pathname)
{
	return NULL;
}

static int smhfs_stat(struct device_d __always_unused *dev,
		      const char *filename, struct stat *s)
{
	FILE file;

	if (smhfs_open(NULL, &file, filename) == 0) {
		s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		s->st_size = file.size;
	}
	smhfs_close(NULL, &file);

	return 0;
}

static int smhfs_probe(struct device_d __always_unused *dev)
{
	/* TODO: Add provisions to detect if debugger is connected */
	return 0;
}

static void smhfs_remove(struct device_d __always_unused *dev)
{
}

static struct fs_driver_d smhfs_driver = {
	.open      = smhfs_open,
	.close     = smhfs_close,
	.read      = smhfs_read,
	.lseek     = smhfs_lseek,
	.opendir   = smhfs_opendir,
	.stat      = smhfs_stat,
	.create    = smhfs_create,
	.unlink    = smhfs_rm,
	.mkdir     = smhfs_mkdir,
	.rmdir     = smhfs_rm,
	.write     = smhfs_write,
	.truncate  = smhfs_truncate,
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
