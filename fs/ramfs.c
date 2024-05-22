/*
 * ramfs.c - a malloc based filesystem
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <xfuncs.h>
#include <linux/sizes.h>

#define CHUNK_SIZE	(4096 * 2)

struct ramfs_chunk {
	unsigned long ofs;
	int size;
	struct list_head list;
	char data[];
};

struct ramfs_inode {
	struct inode inode;
	char *name;
	char *symlink;
	ulong mode;

	/* bytes used in this inode */
	unsigned long size;
	/* bytes currently allocated for this inode */
	unsigned long alloc_size;

	struct list_head data;

	struct ramfs_chunk *current_chunk;
};

static inline struct ramfs_inode *to_ramfs_inode(struct inode *inode)
{
	return container_of(inode, struct ramfs_inode, inode);
}

/* ---------------------------------------------------------------*/

static const struct super_operations ramfs_ops;
static const struct inode_operations ramfs_dir_inode_operations;
static const struct inode_operations ramfs_file_inode_operations;
static const struct inode_operations ramfs_symlink_inode_operations;
static const struct file_operations ramfs_file_operations;

static struct inode *ramfs_get_inode(struct super_block *sb, const struct inode *dir,
				     umode_t mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode->i_mode = mode;

	switch (mode & S_IFMT) {
	default:
		return NULL;
	case S_IFREG:
		inode->i_op = &ramfs_file_inode_operations;
		inode->i_fop = &ramfs_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &ramfs_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;
		inc_nlink(inode);
		break;
	case S_IFLNK:
		inode->i_op = &ramfs_symlink_inode_operations;
		break;
	}

	return inode;
}

#define MIN_SIZE SZ_8K

static struct ramfs_chunk *ramfs_get_chunk(unsigned long size)
{
	struct ramfs_chunk *data;

	if (size < MIN_SIZE)
		size = MIN_SIZE;

	data = calloc(struct_size(data, data, size), 1);
	if (!data)
		return NULL;

	data->size = size;

	return data;
}

static void ramfs_put_chunk(struct ramfs_chunk *data)
{
	free(data);
}

/* ---------------------------------------------------------------*/

static int
ramfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct inode *inode = ramfs_get_inode(dir->i_sb, dir, mode);

	if (!inode)
		return -ENOSPC;

	d_instantiate(dentry, inode);
	dget(dentry);   /* Extra count - pin the dentry in core */

	return 0;
}

static int ramfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int ret;

	ret = ramfs_mknod(dir, dentry, mode | S_IFDIR);
	if (!ret)
		inc_nlink(dir);

	return ret;
}

static int ramfs_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	return ramfs_mknod(dir, dentry, mode | S_IFREG);
}

static int ramfs_symlink(struct inode *dir, struct dentry *dentry,
			 const char *symname)
{
	struct inode *inode;

	inode = ramfs_get_inode(dir->i_sb, dir, S_IFLNK | S_IRWXG);
	if (!inode)
		return -ENOSPC;

	inode->i_link = xstrdup(symname);
	d_instantiate(dentry, inode);

	return 0;
}

static int ramfs_unlink(struct inode *dir, struct dentry *dentry)
{
	return simple_unlink(dir, dentry);
}

static const char *ramfs_get_link(struct dentry *dentry, struct inode *inode)
{
	return inode->i_link;
}

static const struct inode_operations ramfs_symlink_inode_operations =
{
	.get_link = ramfs_get_link,
};

static const struct inode_operations ramfs_dir_inode_operations =
{
	.lookup = simple_lookup,
	.symlink = ramfs_symlink,
	.mkdir = ramfs_mkdir,
	.rmdir = simple_rmdir,
	.unlink = ramfs_unlink,
	.create = ramfs_create,
};

static struct ramfs_chunk *ramfs_find_chunk(struct ramfs_inode *node,
					    unsigned long pos, int *ofs, int *len)
{
	struct ramfs_chunk *data, *cur = node->current_chunk;

	if (cur && pos >= cur->ofs)
		data = cur;
	else
		data = list_first_entry(&node->data, struct ramfs_chunk, list);

	list_for_each_entry_from(data, &node->data, list) {
		if (data->ofs + data->size > pos) {
			*ofs = pos - data->ofs;
			*len = data->ofs + data->size - pos;

			node->current_chunk = data;

			return data;
		}
	}

	pr_err("%s: no chunk for pos %ld found\n", __func__, pos);

	return NULL;
}

static int ramfs_read(struct device *_dev, FILE *f, void *buf, size_t insize)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	struct ramfs_chunk *data;
	int ofs, len, now;
	unsigned long pos = f->pos;
	int size = insize;

	debug("%s: %p %zu @ %lld\n", __func__, node, insize, f->pos);

	while (size) {
		data = ramfs_find_chunk(node, pos, &ofs, &len);
		if (!data)
			return -EINVAL;

		debug("%s: pos: %lu ofs: %d len: %d\n", __func__, pos, ofs, len);

		now = min(size, len);

		memcpy(buf, data->data + ofs, now);

		size -= now;
		buf += now;
		pos += now;
	}

	return insize;
}

static int ramfs_write(struct device *_dev, FILE *f, const void *buf,
		       size_t insize)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	struct ramfs_chunk *data;
	int ofs, len, now;
	unsigned long pos = f->pos;
	int size = insize;

	debug("%s: %p %zu @ %lld\n", __func__, node, insize, f->pos);

	while (size) {
		data = ramfs_find_chunk(node, pos, &ofs, &len);
		if (!data)
			return -EINVAL;

		debug("%s: pos: %lu ofs: %d len: %d\n", __func__, pos, ofs, len);

		now = min(size, len);

		memcpy(data->data + ofs, buf, now);

		size -= now;
		buf += now;
		pos += now;
	}

	return insize;
}

static void ramfs_truncate_down(struct ramfs_inode *node, unsigned long size)
{
	struct ramfs_chunk *data, *tmp;

	list_for_each_entry_safe(data, tmp, &node->data, list) {
		if (data->ofs >= size) {
			list_del(&data->list);
			node->alloc_size -= data->size;
			ramfs_put_chunk(data);
		}
	}

	node->current_chunk = NULL;
}

static int ramfs_truncate_up(struct ramfs_inode *node, unsigned long size)
{
	struct ramfs_chunk *data, *tmp;
	LIST_HEAD(list);
	unsigned long add = size - node->alloc_size;
	unsigned long chunksize = add;
	unsigned long alloc_size = 0;

	if (node->alloc_size >= size)
		return 0;

	/*
	 * We first try to allocate all space we need in a single chunk.
	 * This may fail because of fragmented memory, so in case we cannot
	 * allocate memory we successively decrease the chunk size until
	 * we have enough allocations made.
	 */
	while (1) {
		unsigned long now = min(chunksize, add);

		data = ramfs_get_chunk(now);
		if (!data) {
			/* No luck, try with smaller chunk size */
			chunksize >>= 1;

			/* If we do not have even 128KiB then go out */
			if (chunksize < SZ_128K)
				goto out;

			continue;
		}

		data->ofs = node->alloc_size + alloc_size;

		alloc_size += data->size;

		list_add_tail(&data->list, &list);

		if (add <= data->size)
			break;

		add -= data->size;
	}

	list_splice_tail(&list, &node->data);

	node->alloc_size += alloc_size;

	return 0;

out:
	list_for_each_entry_safe(data, tmp, &list, list) {
		list_del(&data->list);
		ramfs_put_chunk(data);
	}

	return -ENOSPC;
}

static int ramfs_truncate(struct device *dev, FILE *f, loff_t size)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	int ret;

	/*
	 * This is a malloc based filesystem, no need to support more
	 * memory than fits into unsigned long.
	 */
	if (size > ULONG_MAX)
		return -ENOSPC;

	debug("%s: %p cur: %ld new: %lld alloc: %ld\n", __func__, node,
	       node->size, size, node->alloc_size);

	if (size == node->size)
		return 0;

	if (size < node->size) {
		ramfs_truncate_down(node, size);
	} else {
		ret = ramfs_truncate_up(node, size);
		if (ret)
			return ret;
	}

	node->size = size;

	return 0;
}

static int ramfs_memmap(struct device *_dev, FILE *f, void **map, int flags)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	struct ramfs_chunk *data;

	if (list_empty(&node->data))
		return -EINVAL;

	if (!list_is_singular(&node->data))
		return -EINVAL;

	data = list_first_entry(&node->data, struct ramfs_chunk, list);

	*map = data->data;

	return 0;
}

static struct inode *ramfs_alloc_inode(struct super_block *sb)
{
	struct ramfs_inode *node;

	node = xzalloc(sizeof(*node));

	INIT_LIST_HEAD(&node->data);

	return &node->inode;
}

static void ramfs_destroy_inode(struct inode *inode)
{
	struct ramfs_inode *node = to_ramfs_inode(inode);

	ramfs_truncate_down(node, 0);

	free(node);
}

static const struct super_operations ramfs_ops = {
	.alloc_inode = ramfs_alloc_inode,
	.destroy_inode = ramfs_destroy_inode,
};

static int ramfs_probe(struct device *dev)
{
	struct inode *inode;
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;

	sb->s_op = &ramfs_ops;

	inode = ramfs_get_inode(sb, NULL, S_IFDIR);
	sb->s_root = d_make_root(inode);

	return 0;
}

static void ramfs_remove(struct device *dev)
{
}

static struct fs_driver ramfs_driver = {
	.read      = ramfs_read,
	.write     = ramfs_write,
	.memmap    = ramfs_memmap,
	.truncate  = ramfs_truncate,
	.flags     = FS_DRIVER_NO_DEV,
	.drv = {
		.probe  = ramfs_probe,
		.remove = ramfs_remove,
		.name = "ramfs",
	}
};

static int ramfs_init(void)
{
	return register_fs_driver(&ramfs_driver);
}

coredevice_initcall(ramfs_init);
