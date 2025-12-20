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
#include <string.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/mtd/mtd.h>
#include <unistd.h>
#include <range.h>
#include <fs.h>
#include <spec/dps.h>

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

struct cdev *cdev_by_name(const char *filename)
{
	struct cdev *cdev;
	struct cdev_alias *alias;

	for_each_cdev(cdev) {
		if (!strcmp(cdev->name, filename))
			return cdev;
		cdev_for_each_alias(alias, cdev)
			if (!strcmp(alias->name, filename))
				return cdev;
	}
	return NULL;
}

struct cdev *cdev_by_device_node(struct device_node *node)
{
	struct cdev *cdev;

	if (!node)
		return NULL;

	for_each_cdev(cdev) {
		struct cdev_alias *alias;

		if (cdev_of_node(cdev) == node)
			return cdev;

		cdev_for_each_alias(alias, cdev)
			if (alias->device_node == node)
				return cdev;
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

struct cdev *
cdev_find_child_by_gpt_typeuuid(struct cdev *cdev, const guid_t *typeuuid)
{
	struct cdev *partcdev;

	if (!cdev_is_gpt_partitioned(cdev))
		return ERR_PTR(-EINVAL);

	for_each_cdev_partition(partcdev, cdev) {
		if (!guid_equal(&partcdev->typeuuid, typeuuid))
			continue;
		if (cdev->typeflags & DPS_TYPE_FLAG_NO_AUTO) {
			dev_dbg(cdev->dev, "auto discovery skipped\n");
			continue;
		}

		return partcdev;
	}

	return ERR_PTR(-ENOENT);
}

static bool cdev_has_partname_alias(struct cdev *cdev, const char *partname)
{
	char *fullname;
	struct cdev_alias *alias;
	bool ret = false;

	if (!cdev->master)
		return false;

	fullname = xasprintf("%s.%s", cdev->master->name, partname);

	cdev_for_each_alias(alias, cdev) {
		if (streq_ptr(alias->name, fullname)) {
			ret = true;
			break;
		}
	}

	free(fullname);

	return ret;
}

/**
 * cdev_find_partition - find a partition belonging to a physical device
 *
 * @cdev: the cdev which should be searched for partitions
 * @name: the partition name
 */
struct cdev *cdev_find_partition(struct cdev *cdevm, const char *name)
{
	struct cdev *partcdev;

	for_each_cdev_partition(partcdev, cdevm) {
		if (streq_ptr(partcdev->partname, name))
			return partcdev;
		if (cdev_has_partname_alias(partcdev, name))
			return partcdev;
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
		if (streq_ptr(cdev->partname, name))
			return cdev;

		if (cdev_has_partname_alias(cdev, name))
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

static struct cdev *cdev_get_master(struct cdev *cdev)
{
	/* mtd devices handle partitions themselves */
	if (cdev->mtd)
		return cdev;

	while (cdev && cdev_is_partition(cdev))
		cdev = cdev->master;

	return cdev;
}

int cdev_open(struct cdev *cdev, unsigned long flags)
{
	struct cdev *master = cdev_get_master(cdev);
	int ret;

	if (cdev->ops->open) {
		ret = cdev->ops->open(master, flags);
		if (ret)
			return ret;
	}

	cdev->open++;

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

	path = xasprintf("/dev/%s", cdev->name);

	fd = open(path, flags);

	free(path);
	return fd;
}

struct cdev *cdev_open_by_name(const char *name, unsigned long flags)
{
	struct cdev *cdev;
	int ret;

	cdev = cdev_by_name(name);
	if (!cdev)
		return NULL;

	ret = cdev_open(cdev, flags);
	if (ret)
		return NULL;

	return cdev;
}

struct cdev *cdev_open_by_path_name(const char *name, unsigned long flags)
{
	char *canon = canonicalize_path(AT_FDCWD, name);
	struct cdev *cdev;

	if (!canon)
		return cdev_open_by_name(name, flags);

	cdev = cdev_open_by_name(devpath_to_name(canon), flags);

	free(canon);

	return cdev;
}

int cdev_close(struct cdev *cdev)
{
	struct cdev *master = cdev_get_master(cdev);

	if (cdev->ops->close) {
		int ret = cdev->ops->close(master);
		if (ret)
			return ret;
	}

	cdev->open--;

	return 0;
}

ssize_t cdev_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->read)
		return -ENOSYS;

	return cdev->ops->read(master, buf, count, cdev->offset +offset, flags);
}

ssize_t cdev_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->write)
		return -ENOSYS;

	return cdev->ops->write(master, buf, count, cdev->offset + offset, flags);
}

int cdev_flush(struct cdev *cdev)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->flush)
		return 0;

	return cdev->ops->flush(master);
}

int cdev_ioctl(struct cdev *cdev, unsigned int request, void *buf)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->ioctl)
		return -EINVAL;

	return cdev->ops->ioctl(master, request, buf);
}

int cdev_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->erase)
		return -ENOSYS;

	return cdev->ops->erase(master, count, cdev->offset + offset);
}

int cdev_lseek(struct cdev *cdev, loff_t pos)
{
	struct cdev *master = cdev_get_master(cdev);
	int ret;

	if (cdev->ops->lseek) {
		ret = cdev->ops->lseek(master, pos + cdev->offset);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int cdev_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->protect)
		return -ENOSYS;

	return cdev->ops->protect(master, count, offset + cdev->offset, prot);
}

int cdev_discard_range(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct cdev *master = cdev_get_master(cdev);

	if (!cdev->ops->discard_range)
		return -ENOSYS;

	if (cdev->flags & DEVFS_PARTITION_READONLY)
		return -EPERM;

	if (offset >= cdev->size)
		return 0;

	if (count + offset > cdev->size)
		count = cdev->size - offset;

	return cdev->ops->discard_range(master, count, offset + cdev->offset);
}

int cdev_memmap(struct cdev *cdev, void **map, int flags)
{
	struct cdev *master = cdev_get_master(cdev);
	int ret = -ENOSYS;

	if (!cdev->ops->memmap)
		return -EINVAL;

	ret = cdev->ops->memmap(master, map, flags);

	if (!ret)
		*map = (void *)((unsigned long)*map + (unsigned long)cdev->offset);

	return ret;
}

int cdev_truncate(struct cdev *cdev, size_t size)
{
	struct cdev *master = cdev_get_master(cdev);

	if (cdev->ops->truncate)
		return cdev->ops->truncate(master, size);

	return -EPERM;
}

loff_t cdev_size(struct cdev *cdev)
{
	return cdev->flags & DEVFS_IS_CHARACTER_DEV ?
		FILE_SIZE_STREAM : cdev->size;
}

static struct cdev *cdev_alloc(const char *name)
{
	struct cdev *new;

	new = xzalloc(sizeof(*new));
	new->name = xstrdup(name);

	return new;
}

static void cdev_free(struct cdev *cdev)
{
	free(cdev->name);
	free(cdev->partname);
	free(cdev);
}

static bool devfs_initialized;

static int cdev_symlink(struct cdev *cdev, const char *linkname)
{
	char *path;
	int ret;

	if (!devfs_initialized)
		return 0;

	path = xasprintf("/dev/%s", linkname);
	ret = symlink(cdev->name, path);
	free(path);

	return ret;
}

static void devfs_mknod(struct cdev *cdev)
{
	char *path;
	int ret;
	struct cdev_alias *alias;

	if (!devfs_initialized)
		return;

	path = xasprintf("/dev/%s", cdev->name);

	cdev_for_each_alias(alias, cdev)
		cdev_symlink(cdev, alias->name);

	ret = mknod(path, S_IFCHR | 0600, cdev->name);

	free(path);

	if (ret) {
		pr_err("Failed to create /dev/%s: %pe\n", cdev->name, ERR_PTR(ret));
		dump_stack();
	}
}

static void devfs_unlink(const char *name)
{
	char *path;

	path = xasprintf("/dev/%s", name);

	unlink(path);

	free(path);
}

void devfs_init(void)
{
	struct cdev *cdev;

	devfs_initialized = true;

	for_each_cdev(cdev)
		devfs_mknod(cdev);
}

int devfs_create(struct cdev *new)
{
	struct cdev *cdev;

	cdev = cdev_by_name(new->name);
	if (cdev)
		return -EEXIST;

	INIT_LIST_HEAD(&new->partitions);
	INIT_LIST_HEAD(&new->aliases);

	list_add_tail(&new->list, &cdev_list);
	if (new->dev) {
		list_add_tail(&new->devices_list, &new->dev->cdevs);
		if (!new->device_node)
			new->device_node = new->dev->of_node;
	}

	devfs_mknod(new);

	return 0;
}

int devfs_add_alias_node(struct cdev *cdev, const char *name, struct device_node *np)
{
	struct cdev *conflict;
	struct cdev_alias *alias;

	conflict = cdev_by_name(name);
	if (conflict)
		return -EEXIST;

	alias = xzalloc(sizeof(*alias));
	alias->name = xstrdup(name);
	alias->device_node = np;
	list_add_tail(&alias->list, &cdev->aliases);

	cdev_symlink(cdev, name);

	return 0;
}

int devfs_add_alias(struct cdev *cdev, const char *name)
{
	return devfs_add_alias_node(cdev, name, NULL);
}

static void devfs_remove_aliases(struct cdev *cdev)
{
	struct cdev_alias *alias, *tmp;

	list_for_each_entry_safe(alias, tmp, &cdev->aliases, list) {
		devfs_unlink(alias->name);
		free(alias->name);
		free(alias);
	}
}

int devfs_remove(struct cdev *cdev)
{
	struct cdev *c, *tmp;

	if (cdev->open)
		return -EBUSY;

	list_del(&cdev->list);

	if (cdev->dev)
		list_del(&cdev->devices_list);

	devfs_unlink(cdev->name);

	devfs_remove_aliases(cdev);

	list_for_each_entry_safe(c, tmp, &cdev->partitions, partition_entry)
		cdevfs_del_partition(c);

	if (cdev_is_partition(cdev))
		list_del(&cdev->partition_entry);

	return 0;
}

/**
 * check_overlap() - check overlap with existing partitions
 * @cdev: parent cdev
 * @partinfo: partition information
 * @offset: offset of new partition to be added
 * @size: size of new partition to be added
 *
 * Return: NULL if no overlapping partition found or overlapping
 *         partition if and only if it's identical in offset and size
 *         to an existing partition. Otherwise, PTR_ERR(-EINVAL).
 */
static struct cdev *check_overlap(struct cdev *cdev,
				  const struct devfs_partition *partinfo,
				  loff_t offset, loff_t size)
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

		if (region_overlap_size(cpart_offset, cpart->size, offset, size)) {
			if (partinfo->flags & DEVFS_PARTITION_CAN_OVERLAP)
				return NULL;
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
		    partinfo->name, offset, offset + size - 1, cdev->name,
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
	unsigned inherited_flags;

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

	overlap = check_overlap(cdev, partinfo, offset, size);
	if (overlap) {
		if (!IS_ERR(overlap)) {
			/* only fails with -EEXIST, which is fine */
			(void)devfs_add_alias(overlap, partinfo->name);
		}

		return overlap;
	}

	/* Filter flags that we want to pass along to children */
	inherited_flags = get_inheritable_devfs_flags(cdev);

	if (IS_ENABLED(CONFIG_MTD) && cdev->mtd) {
		struct mtd_info *mtd;

		mtd = mtd_add_partition(cdev->mtd, offset, size,
				partinfo->flags | inherited_flags, partinfo->name);
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
	new->flags = inherited_flags;

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

	cdev_free(cdev);

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
	cdev_free(cdev);
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
