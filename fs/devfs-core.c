/*
 * devfs.c - a device file system for barebox
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
#include <complete.h>
#include <driver.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <ioctl.h>
#include <nand.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/mtd/mtd.h>
#include <unistd.h>
#include <fs.h>

LIST_HEAD(cdev_list);

#ifdef CONFIG_AUTO_COMPLETE
int devfs_partition_complete(struct string_list *sl, char *instr)
{
	struct cdev *cdev;
	int len;

	len = strlen(instr);

	for_each_cdev(cdev) {
		if (cdev_is_partition(cdev) &&
		    !strncmp(instr, cdev->name, len)) {
			string_list_add_asprintf(sl, "%s ", cdev->name);
		}
	}
	return COMPLETE_CONTINUE;
}
#endif

struct cdev *cdev_readlink(struct cdev *cdev)
{
	if (!cdev)
		return NULL;

	if (cdev->link)
		cdev = cdev->link;

	/* links to links are not allowed */
	BUG_ON(cdev->link);

	return cdev;
}

struct cdev *lcdev_by_name(const char *filename)
{
	struct cdev *cdev;

	for_each_cdev(cdev) {
		if (!strcmp(cdev->name, filename))
			return cdev;
	}
	return NULL;
}

struct cdev *cdev_by_name(const char *filename)
{
	struct cdev *cdev;

	cdev = lcdev_by_name(filename);
	if (!cdev)
		return NULL;

	return cdev_readlink(cdev);
}

struct cdev *cdev_by_device_node(struct device_node *node)
{
	struct cdev *cdev;

	if (!node)
		return NULL;

	for_each_cdev(cdev) {
		if (cdev->device_node == node)
			return cdev_readlink(cdev);
	}
	return NULL;
}

struct cdev *cdev_by_partuuid(const char *partuuid)
{
	struct cdev *cdev;

	if (!partuuid)
		return NULL;

	for_each_cdev(cdev) {
		if (cdev_is_partition(cdev) && !strcasecmp(cdev->partuuid, partuuid))
			return cdev;
	}
	return NULL;
}

struct cdev *cdev_by_diskuuid(const char *diskuuid)
{
	struct cdev *cdev;

	if (!diskuuid)
		return NULL;

	for_each_cdev(cdev) {
		if (!cdev_is_partition(cdev) && !strcasecmp(cdev->diskuuid, diskuuid))
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
struct cdev *device_find_partition(struct device *dev, const char *name)
{
	struct cdev *cdev;
	struct device *child;

	list_for_each_entry(cdev, &dev->cdevs, devices_list) {
		struct cdev *cdevl;

		if (!cdev->partname)
			continue;
		if (!strcmp(cdev->partname, name))
			return cdev;

		list_for_each_entry(cdevl, &cdev->links, link_entry) {
			if (!strcmp(cdevl->partname, name))
				return cdev_readlink(cdevl);
		}
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

int cdev_open(struct cdev *cdev, unsigned long flags)
{
	if (cdev->ops->open)
		return cdev->ops->open(cdev, flags);

	return 0;
}

int cdev_fdopen(struct cdev *cdev, unsigned long flags)
{
	char *path;
	int fd;

	if (!cdev)
		return -ENODEV;
	if (IS_ERR(cdev))
		return PTR_ERR(cdev);

	path = basprintf("/dev/%s", cdev->name);
	if (!path)
		return -ENOMEM;

	fd = open(path, flags);

	free(path);
	return fd;
}

struct cdev *cdev_open_by_name(const char *name, unsigned long flags)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_by_name(devpath_to_name(name));
	if (!cdev)
		return NULL;

	ret = cdev_open(cdev, flags);
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

int cdev_ioctl(struct cdev *cdev, int request, void *buf)
{
	if (!cdev->ops->ioctl)
		return -EINVAL;

	return cdev->ops->ioctl(cdev, request, buf);
}

int cdev_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	if (!cdev->ops->erase)
		return -ENOSYS;

	return cdev->ops->erase(cdev, count, cdev->offset + offset);
}

int cdev_lseek(struct cdev *cdev, loff_t pos)
{
	int ret;

	if (cdev->ops->lseek) {
		ret = cdev->ops->lseek(cdev, pos + cdev->offset);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int cdev_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
{
	if (!cdev->ops->protect)
		return -ENOSYS;

	return cdev->ops->protect(cdev, count, offset + cdev->offset, prot);
}

int cdev_discard_range(struct cdev *cdev, loff_t count, loff_t offset)
{
	if (!cdev->ops->discard_range)
		return -ENOSYS;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	if (offset >= cdev->size)
		return 0;

	if (count + offset > cdev->size)
		count = cdev->size - offset;

	return cdev->ops->discard_range(cdev, count, offset + cdev->offset);
}

int cdev_memmap(struct cdev *cdev, void **map, int flags)
{
	int ret = -ENOSYS;

	if (!cdev->ops->memmap)
		return -EINVAL;

	ret = cdev->ops->memmap(cdev, map, flags);

	if (!ret)
		*map = (void *)((unsigned long)*map + (unsigned long)cdev->offset);

	return ret;
}

int cdev_truncate(struct cdev *cdev, size_t size)
{
	if (cdev->ops->truncate)
		return cdev->ops->truncate(cdev, size);

	return -EPERM;
}

static struct cdev *cdev_alloc(const char *name)
{
	struct cdev *new;

	new = xzalloc(sizeof(*new));
	new->name = xstrdup(name);

	return new;
}

int devfs_create(struct cdev *new)
{
	struct cdev *cdev;

	cdev = cdev_by_name(new->name);
	if (cdev)
		return -EEXIST;

	INIT_LIST_HEAD(&new->links);
	INIT_LIST_HEAD(&new->partitions);

	list_add_tail(&new->list, &cdev_list);
	if (new->dev) {
		list_add_tail(&new->devices_list, &new->dev->cdevs);
		if (!new->device_node)
			new->device_node = new->dev->of_node;
	}

	return 0;
}

int devfs_create_link(struct cdev *cdev, const char *name)
{
	struct cdev *new;

	if (cdev_by_name(name))
		return -EEXIST;

	/*
	 * Create a link to the real cdev instead of creating
	 * a link to a link.
	 */
	cdev = cdev_readlink(cdev);

	new = cdev_alloc(name);
	new->link = cdev;

	if (cdev->partname) {
		size_t partnameoff = 0;

		if (cdev->master) {
			size_t masterlen = strlen(cdev->master->name);

			if (!strncmp(name, cdev->master->name, masterlen))
				partnameoff += masterlen + 1;
		}

		new->partname = xstrdup(name + partnameoff);
	}

	INIT_LIST_HEAD(&new->links);
	INIT_LIST_HEAD(&new->partitions);
	list_add_tail(&new->list, &cdev_list);
	list_add_tail(&new->link_entry, &cdev->links);

	return 0;
}

int devfs_remove(struct cdev *cdev)
{
	struct cdev *c, *tmp;

	if (cdev->open)
		return -EBUSY;

	list_del(&cdev->list);

	if (cdev->dev)
		list_del(&cdev->devices_list);

	list_for_each_entry_safe(c, tmp, &cdev->links, link_entry)
		devfs_remove(c);

	list_for_each_entry_safe(c, tmp, &cdev->partitions, partition_entry)
		cdevfs_del_partition(c);

	if (cdev_is_partition(cdev))
		list_del(&cdev->partition_entry);

	if (cdev->link)
		free(cdev);

	return 0;
}

static bool region_identical(loff_t starta, loff_t lena,
			     loff_t startb, loff_t lenb)
{
	return starta == startb && lena == lenb;
}

static bool region_overlap(loff_t starta, loff_t lena,
			   loff_t startb, loff_t lenb)
{
	if (starta + lena <= startb)
		return 0;
	if (startb + lenb <= starta)
		return 0;
	return 1;
}

/**
 * check_overlap() - check overlap with existing partitions
 * @cdev: parent cdev
 * @name: partition name for informational purposes on conflict
 * @offset: offset of new partition to be added
 * @size: size of new partition to be added
 *
 * Return: NULL if no overlapping partition found or overlapping
 *         partition if and only if it's identical in offset and size
 *         to an existing partition. Otherwise, PTR_ERR(-EINVAL).
 */
static struct cdev *check_overlap(struct cdev *cdev, const char *name, loff_t offset, loff_t size)
{
	struct cdev *cpart;
	loff_t cpart_offset;
	int ret;

	list_for_each_entry(cpart, &cdev->partitions, partition_entry) {
		cpart_offset = cpart->offset;

		/*
		 * An mtd partition is represented by a separate cdev and its
		 * cpart is relative to this one. So its .offset is 0 and we
		 * have to consult .master_offset to get its offset.
		 */
		if (cpart->mtd)
			cpart_offset = cpart->mtd->master_offset;

		if (region_identical(cpart_offset, cpart->size, offset, size)) {
			ret = 0;
			goto identical;
		}

		if (region_overlap(cpart_offset, cpart->size, offset, size)) {
			ret = -EINVAL;
			goto conflict;
		}
	}

	return NULL;

identical:
conflict:
	__pr_printk(ret ? MSG_WARNING : MSG_DEBUG,
		    "New partition %s (0x%08llx-0x%08llx) on %s "
		    "%s with partition %s (0x%08llx-0x%08llx), not creating it\n",
		    name, offset, offset + size - 1, cdev->name,
		    ret ? "conflicts" : "identical",
		    cpart->name, cpart_offset, cpart_offset + cpart->size - 1);

	return ret ? ERR_PTR(ret) : cpart;
}

static struct cdev *__devfs_add_partition(struct cdev *cdev,
		const struct devfs_partition *partinfo, loff_t *end)
{
	loff_t offset, size;
	loff_t _end = end ? *end : 0;
	static struct cdev *new;
	struct cdev *overlap;

	if (cdev_by_name(partinfo->name))
		return ERR_PTR(-EEXIST);

	if (partinfo->offset > 0)
		offset = partinfo->offset;
	else if (partinfo->offset == 0)
		/* append to previous partition */
		offset = _end;
	else
		/* relative to end of cdev */
		offset = cdev->size + partinfo->offset;

	if (partinfo->size > 0)
		size = partinfo->size;
	else
		size = cdev->size + partinfo->size - offset;

	if (offset >= 0 && offset < _end)
		pr_debug("partition %s not after previous partition\n",
				partinfo->name);

	_end = offset + size;
	if (end)
		*end = _end;

	if (offset < 0 || _end > cdev->size) {
		pr_warn("partition %s not completely inside device %s\n",
				partinfo->name, cdev->name);
		return ERR_PTR(-EINVAL);
	}

	overlap = check_overlap(cdev, partinfo->name, offset, size);
	if (overlap) {
		if (!IS_ERR(overlap)) {
			/* only fails with -EEXIST, which is fine */
			(void)devfs_create_link(overlap, partinfo->name);
		}

		return overlap;
	}

	if (IS_ENABLED(CONFIG_MTD) && cdev->mtd) {
		struct mtd_info *mtd;

		mtd = mtd_add_partition(cdev->mtd, offset, size,
				partinfo->flags, partinfo->name);
		if (IS_ERR(mtd))
			return (void *)mtd;

		list_add_tail(&mtd->cdev.partition_entry, &cdev->partitions);
		return &mtd->cdev;
	}

	new = cdev_alloc(partinfo->name);
	if (!strncmp(cdev->name, partinfo->name, strlen(cdev->name)))
		new->partname = xstrdup(partinfo->name + strlen(cdev->name) + 1);

	new->ops = cdev->ops;
	new->priv = cdev->priv;
	new->size = size;
	new->offset = cdev->offset + offset;

	new->dev = cdev->dev;
	new->master = cdev;

	list_add_tail(&new->partition_entry, &cdev->partitions);

	devfs_create(new);

	cdev_create_default_automount(new);

	return new;
}

struct cdev *cdevfs_add_partition(struct cdev *cdev,
				  const struct devfs_partition *partinfo)
{
	return __devfs_add_partition(cdev, partinfo, NULL);
}

struct cdev *devfs_add_partition(const char *devname, loff_t offset,
		loff_t size, unsigned int flags, const char *name)
{
	struct cdev *cdev;
	const struct devfs_partition partinfo = {
		.offset = offset,
		.size = size,
		.flags = flags,
		.name = name,
	};

	cdev = cdev_by_name(devname);
	if (!cdev)
		return ERR_PTR(-ENOENT);

	return cdevfs_add_partition(cdev, &partinfo);
}

int cdevfs_del_partition(struct cdev *cdev)
{
	int ret;

	if (cdev->flags & DEVFS_PARTITION_FIXED)
		return -EPERM;

	if (IS_ENABLED(CONFIG_MTD) && cdev->mtd) {
		ret = mtd_del_partition(cdev->mtd);
		return ret;
	}

	ret = devfs_remove(cdev);
	if (ret)
		return ret;

	free(cdev->name);
	free(cdev->partname);
	free(cdev);

	return 0;
}

int devfs_del_partition(const char *name)
{
	struct cdev *cdev;

	cdev = cdev_by_name(name);
	if (!cdev)
		return -ENOENT;

	if (!cdev_is_partition(cdev))
		return -EINVAL;

	return cdevfs_del_partition(cdev);
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

struct loop_priv {
	int fd;
};

static ssize_t loop_read(struct cdev *cdev, void *buf, size_t count,
		loff_t offset, ulong flags)
{
	struct loop_priv *priv = cdev->priv;
	loff_t ofs;

	ofs = lseek(priv->fd, offset, SEEK_SET);
	if (ofs < 0)
		return ofs;

	return read(priv->fd, buf, count);
}

static ssize_t loop_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t offset, ulong flags)
{
	struct loop_priv *priv = cdev->priv;
	loff_t ofs;

	ofs = lseek(priv->fd, offset, SEEK_SET);
	if (ofs < 0)
		return ofs;

	return write(priv->fd, buf, count);
}

static const struct cdev_operations loop_ops = {
	.read = loop_read,
	.write = loop_write,
	.memmap = generic_memmap_rw,
};

struct cdev *cdev_create_loop(const char *path, ulong flags, loff_t offset)
{
	char str[16];
	struct cdev *new;
	struct loop_priv *priv;
	static int loopno;
	loff_t ofs;

	priv = xzalloc(sizeof(*priv));

	priv->fd = open(path, flags);
	if (priv->fd < 0) {
		free(priv);
		return NULL;
	}

	snprintf(str, sizeof(str), "loop%u", loopno++);

	new = cdev_alloc(str);
	new->ops = &loop_ops;
	new->priv = priv;

	ofs = lseek(priv->fd, 0, SEEK_END);
	if (ofs < 0 || ofs <= offset) {
		free(new);
		free(priv);
		return NULL;
	}
	lseek(priv->fd, offset, SEEK_SET);

	new->size = ofs;
	new->offset = offset;
	new->dev = NULL;
	new->flags = 0;

	devfs_create(new);

	return new;
}

void cdev_remove_loop(struct cdev *cdev)
{
	struct loop_priv *priv = cdev->priv;

	devfs_remove(cdev);
	close(priv->fd);
	free(priv);
	free(cdev->name);
	free(cdev);
}

ssize_t mem_copy(struct device *dev, void *dst, const void *src,
			resource_size_t count, resource_size_t offset,
			unsigned long flags)
{
	ssize_t size;
	int rwsize = flags & O_RWSIZE_MASK;

	if (!dev || dev->num_resources < 1)
		return -1;

	if (resource_size(&dev->resource[0]) > 0 || offset != 0)
		count = min(count, resource_size(&dev->resource[0]) - offset);

	size = count;

	/* no rwsize specification given. Do whatever memcpy likes best */
	if (!rwsize) {
		memcpy(dst, src, count);
		goto out;
	}

	rwsize = rwsize >> O_RWSIZE_SHIFT;

	count = ALIGN_DOWN(count, rwsize);

	while (count) {
		switch (rwsize) {
		case 1:
			*((u8 *)dst) = *((u8 *)src);
			break;
		case 2:
			*((u16 *)dst) = *((u16 *)src);
			break;
		case 4:
			*((u32  *)dst) = *((u32  *)src);
			break;
		case 8:
			*((u64  *)dst) = *((u64  *)src);
			break;
		}
		dst += rwsize;
		src += rwsize;
		count -= rwsize;
	}
out:
	return size;
}

ssize_t mem_read(struct cdev *cdev, void *buf, size_t count, loff_t offset,
		 unsigned long flags)
{
	struct device *dev = cdev->dev;

	if (!dev)
		return -1;

	return mem_copy(dev, buf, dev_get_mem_region(dev, 0) + offset,
			count, offset, flags);
}
EXPORT_SYMBOL(mem_read);

ssize_t mem_write(struct cdev *cdev, const void *buf, size_t count,
		  loff_t offset, unsigned long flags)
{
	struct device *dev = cdev->dev;

	if (!dev)
		return -1;

	return mem_copy(dev, dev_get_mem_region(dev, 0) + offset, buf,
			count, offset, flags);
}
EXPORT_SYMBOL(mem_write);
