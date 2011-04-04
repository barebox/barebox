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
#include <ioctl.h>
#include <nand.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/mtd-abi.h>
#include <partition.h>

static LIST_HEAD(cdev_list);

struct cdev *cdev_by_name(const char *filename)
{
	struct cdev *cdev;

	list_for_each_entry(cdev, &cdev_list, list) {
		if (!strcmp(cdev->name, filename))
			return cdev;
	}
	return NULL;
}

ssize_t cdev_read(struct cdev *cdev, void *buf, size_t count, ulong offset, ulong flags)
{
	if (!cdev->ops->read)
		return -ENOSYS;

	return cdev->ops->read(cdev, buf, count, cdev->offset +offset, flags);
}

ssize_t cdev_write(struct cdev *cdev, const void *buf, size_t count, ulong offset, ulong flags)
{
	if (!cdev->ops->write)
		return -ENOSYS;

	return cdev->ops->write(cdev, buf, count, cdev->offset + offset, flags);
}

static int devfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct cdev *cdev = f->inode;

	return cdev_read(cdev, buf, size, f->pos, f->flags);
}

static int devfs_write(struct device_d *_dev, FILE *f, const void *buf, size_t size)
{
	struct cdev *cdev = f->inode;

	return cdev_write(cdev, buf, size, f->pos, f->flags);
}

static off_t devfs_lseek(struct device_d *_dev, FILE *f, off_t pos)
{
	struct cdev *cdev = f->inode;
	off_t ret = -1;

	if (cdev->ops->lseek)
		ret = cdev->ops->lseek(cdev, pos + cdev->offset);

	if (ret != -1)
		f->pos = pos;

	return ret - cdev->offset;
}

static int devfs_erase(struct device_d *_dev, FILE *f, size_t count, unsigned long offset)
{
	struct cdev *cdev = f->inode;

	if (!cdev->ops->erase)
		return -ENOSYS;

	return cdev->ops->erase(cdev, count, offset + cdev->offset);
}

static int devfs_protect(struct device_d *_dev, FILE *f, size_t count, unsigned long offset, int prot)
{
	struct cdev *cdev = f->inode;

	if (!cdev->ops->protect)
		return -ENOSYS;

	return cdev->ops->protect(cdev, count, offset + cdev->offset, prot);
}

static int devfs_memmap(struct device_d *_dev, FILE *f, void **map, int flags)
{
	struct cdev *cdev = f->inode;
	int ret = -ENOSYS;

	if (!cdev->ops->memmap)
		return -EINVAL;

	ret = cdev->ops->memmap(cdev, map, flags);

	if (!ret)
		*map = (void *)((unsigned long)*map + cdev->offset);

	return ret;
}

static int devfs_open(struct device_d *_dev, FILE *f, const char *filename)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_by_name(filename + 1);

	if (!cdev)
		return -ENOENT;

	f->size = cdev->size;
	f->inode = cdev;

	if (cdev->ops->open) {
		ret = cdev->ops->open(cdev, f);
		if (ret)
			return ret;
	}

	cdev->open++;

	return 0;
}

static int devfs_close(struct device_d *_dev, FILE *f)
{
	struct cdev *cdev = f->inode;
	int ret;

	if (cdev->ops->close) {
		ret = cdev->ops->close(cdev, f);
		if (ret)
			return ret;
	}

	cdev->open--;

	return 0;
}

static int partition_ioctl(struct cdev *cdev, int request, void *buf)
{
	size_t offset;
	struct mtd_info_user *user = buf;

	switch (request) {
	case MEMSETBADBLOCK:
	case MEMGETBADBLOCK:
		offset = (off_t)buf;
		offset += cdev->offset;
		return cdev->ops->ioctl(cdev, request, (void *)offset);
	case MEMGETINFO:
		if (cdev->mtd) {
			user->type	= cdev->mtd->type;
			user->flags	= cdev->mtd->flags;
			user->size	= cdev->mtd->size;
			user->erasesize	= cdev->mtd->erasesize;
			user->oobsize	= cdev->mtd->oobsize;
			user->mtd	= cdev->mtd;
			/* The below fields are obsolete */
			user->ecctype	= -1;
			user->eccsize	= 0;
			return 0;
		}
		if (!cdev->ops->ioctl)
			return -EINVAL;
		return cdev->ops->ioctl(cdev, request, buf);
	default:
		return -EINVAL;
	}
}

static int devfs_ioctl(struct device_d *_dev, FILE *f, int request, void *buf)
{
	struct cdev *cdev = f->inode;

	if (cdev->flags & DEVFS_IS_PARTITION)
		return partition_ioctl(cdev, request, buf);

	if (!cdev->ops->ioctl)
		return -EINVAL;

	return cdev->ops->ioctl(cdev, request, buf);
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

	cdev = cdev_by_name(filename + 1);
	if (!cdev)
		return -ENOENT;

	s->st_mode = S_IFCHR;
	s->st_size = cdev->size;
	if (cdev->ops->write)
		s->st_mode |= S_IWUSR;
	if (cdev->ops->read)
		s->st_mode |= S_IRUSR;

	return 0;
}

static int devfs_probe(struct device_d *dev)
{
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
	.ioctl     = devfs_ioctl,
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
		.probe  = devfs_probe,
		.remove = devfs_delete,
		.name = "devfs",
		.type_data = &devfs_driver,
	}
};

static int devfs_init(void)
{
	return register_fs_driver(&devfs_driver);
}

coredevice_initcall(devfs_init);

int devfs_create(struct cdev *new)
{
	struct cdev *cdev;

	cdev = cdev_by_name(new->name);
	if (cdev)
		return -EEXIST;

	list_add_tail(&new->list, &cdev_list);
	if (new->dev)
		list_add_tail(&new->devices_list, &new->dev->cdevs);

	return 0;
}

int devfs_remove(struct cdev *cdev)
{
	if (cdev->open)
		return -EBUSY;

	list_del(&cdev->list);
	if (cdev->dev)
		list_del(&cdev->devices_list);

	return 0;
}

int devfs_add_partition(const char *devname, unsigned long offset, size_t size,
		int flags, const char *name)
{
	struct cdev *cdev, *new;

	cdev = cdev_by_name(name);
	if (cdev)
		return -EEXIST;

	cdev = cdev_by_name(devname);
	if (!cdev)
		return -ENOENT;

	if (offset + size > cdev->size)
		return -EINVAL;

	new = xzalloc(sizeof (*new));
	new->name = strdup(name);
	new->ops = cdev->ops;
	new->priv = cdev->priv;
	new->size = size;
	new->offset = offset + cdev->offset;
	new->dev = cdev->dev;
	new->flags = flags | DEVFS_IS_PARTITION;

#ifdef CONFIG_PARTITION_NEED_MTD
	if (cdev->mtd) {
		new->mtd = mtd_add_partition(cdev->mtd, offset, size, flags, name);
		if (IS_ERR(new->mtd)) {
			int ret = PTR_ERR(new->mtd);
			free(new);
			return ret;
		}
	}
#endif

	devfs_create(new);

	return 0;
}

int devfs_del_partition(const char *name)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_by_name(name);
	if (!cdev)
		return -ENOENT;

	if (!(cdev->flags & DEVFS_IS_PARTITION))
		return -EINVAL;
	if (cdev->flags & DEVFS_PARTITION_FIXED)
		return -EPERM;

#ifdef CONFIG_PARTITION_NEED_MTD
	if (cdev->mtd)
		mtd_del_partition(cdev->mtd);
#endif

	ret = devfs_remove(cdev);
	if (ret)
		return ret;

	free(cdev->name);
	free(cdev);

	return 0;
}

