/*
 * ramfs.c - a malloc based filesystem
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

#define CHUNK_SIZE	(4096 * 2)

struct ramfs_chunk {
	char *data;
	struct ramfs_chunk *next;
};

struct ramfs_inode {
	struct inode inode;
	char *name;
	struct ramfs_inode *parent;
	struct ramfs_inode *next;
	struct ramfs_inode *child;
	char *symlink;
	ulong mode;

	struct handle_d *handle;

	ulong size;
	struct ramfs_chunk *data;

	/* Points to recently used chunk */
	int recent_chunk;
	struct ramfs_chunk *recent_chunkp;
};

static inline struct ramfs_inode *to_ramfs_inode(struct inode *inode)
{
	return container_of(inode, struct ramfs_inode, inode);
}

struct ramfs_priv {
	struct ramfs_inode root;
};

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

static int chunks = 0;

static struct ramfs_chunk *ramfs_get_chunk(void)
{
	struct ramfs_chunk *data = malloc(sizeof(struct ramfs_chunk));
	if (!data)
		return NULL;

	data->data = calloc(CHUNK_SIZE, 1);
	if (!data->data) {
		free(data);
		return NULL;
	}
	data->next = NULL;
	chunks++;

	return data;
}

static void ramfs_put_chunk(struct ramfs_chunk *data)
{
	free(data->data);
	free(data);
	chunks--;
}

/* ---------------------------------------------------------------*/

static int
ramfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct inode *inode = ramfs_get_inode(dir->i_sb, dir, mode);

	if (!inode)
		return -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);   /* Extra count - pin the dentry in core */
	}

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
	struct inode *inode = d_inode(dentry);

	if (inode) {
		struct ramfs_inode *node = to_ramfs_inode(inode);
		struct ramfs_chunk *chunk = node->data;

		node->data = NULL;

		while (chunk) {
			struct ramfs_chunk *tmp = chunk;

			chunk = chunk->next;

			ramfs_put_chunk(tmp);
		}
	}

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

static struct ramfs_chunk *ramfs_find_chunk(struct ramfs_inode *node, int chunk)
{
	struct ramfs_chunk *data;
	int left = chunk;

	if (chunk == 0)
		return node->data;

	if (node->recent_chunk == chunk)
		return node->recent_chunkp;

	if (node->recent_chunk < chunk && node->recent_chunk != 0) {
		/* Start at last known chunk */
		data = node->recent_chunkp;
		left -= node->recent_chunk;
	} else {
		/* Start at first chunk */
		data = node->data;
	}

	while (left--)
		data = data->next;

	node->recent_chunkp = data;
	node->recent_chunk = chunk;

	return data;
}

static int ramfs_read(struct device_d *_dev, FILE *f, void *buf, size_t insize)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	int chunk;
	struct ramfs_chunk *data;
	int ofs;
	int now;
	int pos = f->pos;
	int size = insize;

	chunk = pos / CHUNK_SIZE;
	debug("%s: reading from chunk %d\n", __FUNCTION__, chunk);

	/* Position ourself in stream */
	data = ramfs_find_chunk(node, chunk);
	ofs = pos % CHUNK_SIZE;

	/* Read till end of current chunk */
	if (ofs) {
		now = min(size, CHUNK_SIZE - ofs);
		debug("Reading till end of node. size: %d\n", size);
		memcpy(buf, data->data + ofs, now);
		size -= now;
		pos += now;
		buf += now;
		if (pos > node->size)
			node->size = now;
		data = data->next;
	}

	/* Do full chunks */
	while (size >= CHUNK_SIZE) {
		debug("do full chunk. size: %d\n", size);
		memcpy(buf, data->data, CHUNK_SIZE);
		data = data->next;
		size -= CHUNK_SIZE;
		pos += CHUNK_SIZE;
		buf += CHUNK_SIZE;
	}

	/* And the rest */
	if (size) {
		debug("do rest. size: %d\n", size);
		memcpy(buf, data->data, size);
	}

	return insize;
}

static int ramfs_write(struct device_d *_dev, FILE *f, const void *buf, size_t insize)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	int chunk;
	struct ramfs_chunk *data;
	int ofs;
	int now;
	int pos = f->pos;
	int size = insize;

	chunk = f->pos / CHUNK_SIZE;
	debug("%s: writing to chunk %d\n", __FUNCTION__, chunk);

	/* Position ourself in stream */
	data = ramfs_find_chunk(node, chunk);
	ofs = f->pos % CHUNK_SIZE;

	/* Write till end of current chunk */
	if (ofs) {
		now = min(size, CHUNK_SIZE - ofs);
		debug("writing till end of node. size: %d\n", size);
		memcpy(data->data + ofs, buf, now);
		size -= now;
		pos += now;
		buf += now;
		if (pos > node->size)
			node->size = now;
		data = data->next;
	}

	/* Do full chunks */
	while (size >= CHUNK_SIZE) {
		debug("do full chunk. size: %d\n", size);
		memcpy(data->data, buf, CHUNK_SIZE);
		data = data->next;
		size -= CHUNK_SIZE;
		pos += CHUNK_SIZE;
		buf += CHUNK_SIZE;
	}

	/* And the rest */
	if (size) {
		debug("do rest. size: %d\n", size);
		memcpy(data->data, buf, size);
	}

	return insize;
}

static loff_t ramfs_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	f->pos = pos;
	return f->pos;
}

static int ramfs_truncate(struct device_d *dev, FILE *f, ulong size)
{
	struct inode *inode = f->f_inode;
	struct ramfs_inode *node = to_ramfs_inode(inode);
	int oldchunks, newchunks;
	struct ramfs_chunk *data = node->data;

	newchunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
	oldchunks = (node->size + CHUNK_SIZE - 1) / CHUNK_SIZE;

	if (newchunks < oldchunks) {
		if (!newchunks)
			node->data = NULL;
		while (newchunks--)
			data = data->next;
		while (data) {
			struct ramfs_chunk *tmp;
			tmp = data->next;
			ramfs_put_chunk(data);
			data = tmp;
		}
		if (node->recent_chunk > newchunks)
			node->recent_chunk = 0;
	}

	if (newchunks > oldchunks) {
		if (!data) {
			node->data = ramfs_get_chunk();
			if (!node->data)
				return -ENOMEM;
			data = node->data;
		}

		newchunks--;
		while (data->next) {
			newchunks--;
			data = data->next;
		}

		while (newchunks--) {
			data->next = ramfs_get_chunk();
			if (!data->next)
				return -ENOMEM;
			data = data->next;
		}
	}
	node->size = size;
	return 0;
}

static struct inode *ramfs_alloc_inode(struct super_block *sb)
{
	struct ramfs_inode *node;

	node = xzalloc(sizeof(*node));
	if (!node)
		return NULL;

	return &node->inode;
}

static const struct super_operations ramfs_ops = {
	.alloc_inode = ramfs_alloc_inode,
};

static int ramfs_probe(struct device_d *dev)
{
	struct inode *inode;
	struct ramfs_priv *priv = xzalloc(sizeof(struct ramfs_priv));
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;

	dev->priv = priv;

	priv->root.name = "/";
	priv->root.mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
	priv->root.parent = &priv->root;

	sb->s_op = &ramfs_ops;

	inode = ramfs_get_inode(sb, NULL, S_IFDIR);
	sb->s_root = d_make_root(inode);

	return 0;
}

static void ramfs_remove(struct device_d *dev)
{
	free(dev->priv);
}

static struct fs_driver_d ramfs_driver = {
	.read      = ramfs_read,
	.write     = ramfs_write,
	.lseek     = ramfs_lseek,
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
