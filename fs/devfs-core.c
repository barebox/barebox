/*
 * devfs.c - a device file system for barebox
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <complete.h>
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <ioctl.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>

LIST_HEAD(cdev_list);

#ifdef CONFIG_AUTO_COMPLETE
int devfs_partition_complete(struct string_list *sl, char *instr)
{
	struct cdev *cdev;
	int len;

	len = strlen(instr);

	list_for_each_entry(cdev, &cdev_list, list) {
		if (cdev->flags & DEVFS_IS_PARTITION &&
		    !strncmp(instr, cdev->name, len)) {
			string_list_add_asprintf(sl, "%s ", cdev->name);
		}
	}
	return COMPLETE_CONTINUE;
}
#endif

struct cdev *cdev_by_name(const char *filename)
{
	struct cdev *cdev;

	list_for_each_entry(cdev, &cdev_list, list) {
		if (!strcmp(cdev->name, filename))
			return cdev;
	}
	return NULL;
}

/**
 * device_find_partition - find a partition belonging to a physical device
 *
 * @dev: the device which should be searched for partitions
 * @name: the partition name
 */
struct cdev *device_find_partition(struct device_d *dev, const char *name)
{
	struct cdev *cdev;
	struct device_d *child;

	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		if (!cdev->partname)
			continue;
		if (!strcmp(cdev->partname, name))
			return cdev;
	}

	device_for_each_child(dev, child) {
		cdev = device_find_partition(child, name);
		if (cdev)
			return cdev;
	}

	return NULL;
}

int cdev_find_free_index(const char *basename)
{
	int i;
	char fname[100];

	for (i = 0; i < 1000; i++) {
		snprintf(fname, sizeof(fname), "%s%d", basename, i);
		if (cdev_by_name(fname) == NULL)
			return i;
	}

	return -EBUSY;	/* all indexes are used */
}

int cdev_do_open(struct cdev *cdev, unsigned long flags)
{
	if (cdev->ops->open)
		return cdev->ops->open(cdev, flags);

	return 0;
}

struct cdev *cdev_open(const char *name, unsigned long flags)
{
	struct cdev *cdev = cdev_by_name(name);
	int ret;

	if (!cdev)
		return NULL;

	ret = cdev_do_open(cdev, flags);
	if (ret)
		return NULL;

	return cdev;
}

void cdev_close(struct cdev *cdev)
{
	if (cdev->ops->close)
		cdev->ops->close(cdev);
}

ssize_t cdev_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	if (!cdev->ops->read)
		return -ENOSYS;

	return cdev->ops->read(cdev, buf, count, cdev->offset +offset, flags);
}

ssize_t cdev_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	if (!cdev->ops->write)
		return -ENOSYS;

	return cdev->ops->write(cdev, buf, count, cdev->offset + offset, flags);
}

int cdev_flush(struct cdev *cdev)
{
	if (!cdev->ops->flush)
		return 0;

	return cdev->ops->flush(cdev);
}

static int partition_ioctl(struct cdev *cdev, int request, void *buf)
{
	int ret = 0;
	loff_t offset, *_buf = buf;
	struct mtd_info_user *user = buf;

	switch (request) {
	case MEMSETBADBLOCK:
	case MEMGETBADBLOCK:
		offset = *_buf;
		offset += cdev->offset;
		ret = cdev->ops->ioctl(cdev, request, &offset);
		break;
	case MEMGETINFO:
		if (cdev->mtd) {
			user->type	= cdev->mtd->type;
			user->flags	= cdev->mtd->flags;
			user->size	= cdev->mtd->size;
			user->erasesize	= cdev->mtd->erasesize;
			user->writesize	= cdev->mtd->writesize;
			user->oobsize	= cdev->mtd->oobsize;
			user->mtd	= cdev->mtd;
			/* The below fields are obsolete */
			user->ecctype	= -1;
			user->eccsize	= 0;
			break;
		}
		if (!cdev->ops->ioctl) {
			ret = -EINVAL;
			break;
		}
		ret = cdev->ops->ioctl(cdev, request, buf);
		break;
#if (defined(CONFIG_NAND_ECC_HW) || defined(CONFIG_NAND_ECC_SOFT))
	case ECCGETSTATS:
#endif
	case MEMERASE:
		if (!cdev->ops->ioctl) {
			ret = -EINVAL;
			break;
		}
		ret = cdev->ops->ioctl(cdev, request, buf);
		break;
#ifdef CONFIG_PARTITION
	case MEMGETREGIONINFO:
		if (cdev->mtd) {
			struct region_info_user *reg = buf;
			int erasesize_shift = ffs(cdev->mtd->erasesize) - 1;

			reg->offset = cdev->offset;
			reg->erasesize = cdev->mtd->erasesize;
			reg->numblocks = cdev->size >> erasesize_shift;
			reg->regionindex = cdev->mtd->index;
		}
	break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

int cdev_ioctl(struct cdev *cdev, int request, void *buf)
{
	if (cdev->flags & DEVFS_IS_PARTITION)
		return partition_ioctl(cdev, request, buf);

	if (!cdev->ops->ioctl)
		return -EINVAL;

	return cdev->ops->ioctl(cdev, request, buf);
}

int cdev_erase(struct cdev *cdev, size_t count, loff_t offset)
{
	if (!cdev->ops->erase)
		return -ENOSYS;

	return cdev->ops->erase(cdev, count, cdev->offset + offset);
}

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

struct cdev *devfs_add_partition(const char *devname, loff_t offset, loff_t size,
		int flags, const char *name)
{
	struct cdev *cdev, *new;

	cdev = cdev_by_name(name);
	if (cdev)
		return ERR_PTR(-EEXIST);

	cdev = cdev_by_name(devname);
	if (!cdev)
		return ERR_PTR(-ENOENT);

	if (offset + size > cdev->size)
		return ERR_PTR(-EINVAL);

	new = xzalloc(sizeof (*new));
	new->name = strdup(name);
	if (!strncmp(devname, name, strlen(devname)))
		new->partname = xstrdup(name + strlen(devname) + 1);
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
			return ERR_PTR(ret);
		}
	}
#endif

	devfs_create(new);

	return new;
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
	free(cdev->partname);
	free(cdev);

	return 0;
}
