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
#include <nand.h>
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
			user->subpagesize = cdev->mtd->writesize >> cdev->mtd->subpage_sft;
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

static struct cdev *__devfs_add_partition(struct cdev *cdev,
		const struct devfs_partition *partinfo, loff_t *end)
{
	loff_t offset, size;
	static struct cdev *new;

	if (cdev_by_name(partinfo->name))
		return ERR_PTR(-EEXIST);

	if (partinfo->offset > 0)
		offset = partinfo->offset;
	else if (partinfo->offset == 0)
		/* append to previous partition */
		offset = *end;
	else
		/* relative to end of cdev */
		offset = cdev->size + partinfo->offset;

	if (partinfo->size > 0)
		size = partinfo->size;
	else
		size = cdev->size + partinfo->size - offset;

	if (offset >= 0 && offset < *end)
		pr_debug("partition %s not after previous partition\n",
				partinfo->name);

	*end = offset + size;

	if (offset < 0 || *end > cdev->size) {
		pr_warn("partition %s not completely inside device %s\n",
				partinfo->name, cdev->name);
		return ERR_PTR(-EINVAL);
	}

	if (IS_ENABLED(CONFIG_PARTITION_NEED_MTD) && cdev->mtd) {
		struct mtd_info *mtd;

		mtd = mtd_add_partition(cdev->mtd, offset, size,
				partinfo->flags, partinfo->name);
		if (IS_ERR(mtd))
			return (void *)mtd;
		return 0;
	}

	new = xzalloc(sizeof(*new));
	new->name = strdup(partinfo->name);
	if (!strncmp(cdev->name, partinfo->name, strlen(cdev->name)))
		new->partname = xstrdup(partinfo->name + strlen(cdev->name) + 1);

	new->ops = cdev->ops;
	new->priv = cdev->priv;
	new->size = size;
	new->offset = cdev->offset + offset;

	new->dev = cdev->dev;
	new->flags = partinfo->flags | DEVFS_IS_PARTITION;

	devfs_create(new);

	return new;
}

struct cdev *devfs_add_partition(const char *devname, loff_t offset,
		loff_t size, unsigned int flags, const char *name)
{
	struct cdev *cdev;
	loff_t end = 0;
	const struct devfs_partition partinfo = {
		.offset = offset,
		.size = size,
		.flags = flags,
		.name = name,
	};

	cdev = cdev_by_name(devname);
	if (!cdev)
		return ERR_PTR(-ENOENT);

	return __devfs_add_partition(cdev, &partinfo, &end);
}

int devfs_del_partition(const char *name)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_by_name(name);
	if (!cdev)
		return -ENOENT;

	if (IS_ENABLED(CONFIG_PARTITION_NEED_MTD) && cdev->mtd) {
		ret = mtd_del_partition(cdev->mtd);
		return ret;
	}

	if (!(cdev->flags & DEVFS_IS_PARTITION))
		return -EINVAL;
	if (cdev->flags & DEVFS_PARTITION_FIXED)
		return -EPERM;

	ret = devfs_remove(cdev);
	if (ret)
		return ret;

	free(cdev->name);
	free(cdev->partname);
	free(cdev);

	return 0;
}

int devfs_create_partitions(const char *devname,
		const struct devfs_partition partinfo[])
{
	loff_t offset = 0;
	struct cdev *cdev;

	cdev = cdev_by_name(devname);
	if (!cdev)
		return -ENOENT;

	for (; partinfo->name; ++partinfo) {
		struct cdev *new;

		new = __devfs_add_partition(cdev, partinfo, &offset);
		if (IS_ERR(new))
			return PTR_ERR(new);

		if (partinfo->bbname)
			dev_add_bb_dev(partinfo->name, partinfo->bbname);
	}

	return 0;
}
