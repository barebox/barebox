/*
 * devfs.c - a device file system for U-Boot
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
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <command.h>
#include <errno.h>
#include <xfuncs.h>
#include <linux/stat.h>

static int devfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct device_d *dev = f->inode;

	return dev_read(dev, buf, size, f->pos, f->flags);
}

static int devfs_write(struct device_d *_dev, FILE *f, const void *buf, size_t size)
{
	struct device_d *dev = f->inode;

	return dev_write(dev, buf, size, f->pos, f->flags);
}

static int devfs_erase(struct device_d *_dev, FILE *f, size_t count, unsigned long offset)
{
	struct device_d *dev = f->inode;

	return dev_erase(dev, count, offset);
}

static int devfs_protect(struct device_d *_dev, FILE *f, size_t count, unsigned long offset, int prot)
{
	struct device_d *dev = f->inode;

	return dev_protect(dev, count, offset, prot);
}

static int devfs_memmap(struct device_d *_dev, FILE *f, void **map, int flags)
{
	struct device_d *dev = f->inode;

	return dev_memmap(dev, map, flags);
}

static int devfs_open(struct device_d *_dev, FILE *file, const char *filename)
{
	struct device_d *dev = get_device_by_id(filename + 1);

	if (!dev)
		return -ENOENT;

	file->size = dev->size;
	file->inode = dev;
	return 0;
}

static int devfs_close(struct device_d *dev, FILE *f)
{
	return 0;
}

static int devfs_truncate(struct device_d *dev, FILE *f, ulong size)
{
	if (size > f->dev->size)
		return -ENOSPC;
	return 0;
}

static DIR* devfs_opendir(struct device_d *dev, const char *pathname)
{
	DIR *dir;

	dir = xzalloc(sizeof(DIR));

	if (!list_empty(&device_list))
		dir->priv = list_first_entry(&device_list, struct device_d, list);

	return dir;
}

static struct dirent* devfs_readdir(struct device_d *_dev, DIR *dir)
{
	struct device_d *dev = dir->priv;

	if (!dev)
		return NULL;

	list_for_each_entry_from(dev, &device_list, list) {
		if (!*dev->id)
			continue;
		if (!dev->driver)
			continue;
		strcpy(dir->d.d_name, dev->id);
		dir->priv = list_entry(dev->list.next, struct device_d, list);
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
	struct device_d *dev;

	dev = get_device_by_id(filename + 1);
	if (!dev)
		return -ENOENT;

	if (!dev->driver)
		return -ENXIO;

	s->st_mode = S_IFCHR;
	s->st_size = dev->size;
	if (dev->driver->write)
		s->st_mode |= S_IWUSR;
	if (dev->driver->read)
		s->st_mode |= S_IRUSR;

	return 0;
}

static int devfs_probe(struct device_d *dev)
{
	return 0;
}

static struct fs_driver_d devfs_driver = {
	.type      = FS_TYPE_DEVFS,
	.read      = devfs_read,
	.write     = devfs_write,
	.open      = devfs_open,
	.close     = devfs_close,
	.opendir   = devfs_opendir,
	.readdir   = devfs_readdir,
	.truncate  = devfs_truncate,
	.closedir  = devfs_closedir,
	.stat      = devfs_stat,
	.erase     = devfs_erase,
	.protect   = devfs_protect,
	.memmap    = devfs_memmap,
	.flags     = FS_DRIVER_NO_DEV,
	.drv = {
		.type   = DEVICE_TYPE_FS,
		.probe  = devfs_probe,
		.name = "devfs",
		.type_data = &devfs_driver,
	}
};

static int devfs_init(void)
{
	return register_driver(&devfs_driver.drv);
}

device_initcall(devfs_init);

