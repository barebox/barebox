// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Zodiac Inflight Innovations
 */

#define pr_fmt(fmt) "ubootvarfs: " fmt

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <string.h>
#include <errno.h>
#include <linux/stat.h>
#include <xfuncs.h>
#include <fcntl.h>
#include <efi.h>
#include <wchar.h>
#include <linux/err.h>
#include <linux/ctype.h>

/**
 * Some theory of operation:
 *
 * U-Boot environment variable data is expected to be presented as a
 * single blob containing an arbitrary number "<key>=<value>\0" pairs
 * without any other auxiliary information (accomplished by ubootvar
 * driver)
 *
 * Filesystem driver code in this file parses above data an creates a
 * linked list of all of the "variables" found (see @ubootvarfs_var to
 * what information is recorded).
 *
 * With that in place reading or writing file data becomes as trivial
 * as looking up a variable in the linked list by name and then
 * memcpy()-ing bytes from its value region.
 *
 * The only moderately tricky part is re-sizing a given file/variable
 * since, given the underlying data format, it requires us to move all
 * of the key/value data that comes after the given file/variable as
 * well as to adjust all of the cached offsets stored in variable
 * linked list. See ubootvarfs_adjust() for the implementation
 * details.
 */

/**
 * struct ubootvarfs_var - U-Boot environment key-value pair
 *
 * @list:	Linked list head
 * @name:	Pointer to memory containing key string (variable name)
 * @name_len:	Variable name's (above) length
 * @start:	Start of value in memory
 * @end:	End of value in memory
 */
struct ubootvarfs_var {
	struct list_head list;
	char *name;
	size_t name_len;
	char *start;
	char *end;
};

/**
 * struct ubootvarfs_data - U-Boot environment data
 *
 * @var_list:	Linked list of all of the parsed variables
 * @fd:		File descriptor of underlying ubootvar device
 * @end:	End of U-boot environment
 * @limit:	U-boot environment limit (can't grow to go past the limit)
 */
struct ubootvarfs_data {
	struct list_head var_list;
	int fd;
	char *end;
	const char *limit;
};

struct ubootvarfs_inode {
	struct inode inode;
	struct ubootvarfs_var *var;
	struct ubootvarfs_data *data;
};

static struct ubootvarfs_inode *inode_to_node(struct inode *inode)
{
	return container_of(inode, struct ubootvarfs_inode, inode);
}

static const struct inode_operations ubootvarfs_file_inode_operations;
static const struct file_operations ubootvarfs_dir_operations;
static const struct inode_operations ubootvarfs_dir_inode_operations;
static const struct file_operations ubootvarfs_file_operations;

static struct inode *ubootvarfs_get_inode(struct super_block *sb,
					  const struct inode *dir,
					  umode_t mode,
					  struct ubootvarfs_var *var)
{
	struct inode *inode = new_inode(sb);
	struct ubootvarfs_inode *node;

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode->i_mode = mode;
	if (var)
		inode->i_size = var->end - var->start;

	node = inode_to_node(inode);
	node->var = var;

	switch (mode & S_IFMT) {
	default:
		return NULL;
	case S_IFREG:
		inode->i_op = &ubootvarfs_file_inode_operations;
		inode->i_fop = &ubootvarfs_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &ubootvarfs_dir_inode_operations;
		inode->i_fop = &ubootvarfs_dir_operations;
		inc_nlink(inode);
		break;
	}

	return inode;
}

static struct ubootvarfs_var *
ubootvarfs_var_by_name(struct ubootvarfs_data *data, const char *name)
{
	struct ubootvarfs_var *var;
	const size_t len = strlen(name);

	list_for_each_entry(var, &data->var_list, list) {
		if (len == var->name_len &&
		    !memcmp(name, var->name, var->name_len))
			return var;
	}

	return NULL;
}

static struct dentry *ubootvarfs_lookup(struct inode *dir,
					struct dentry *dentry,
					unsigned int flags)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	struct ubootvarfs_data *data = fsdev->dev.priv;
	struct ubootvarfs_var *var;
	struct inode *inode;

	var = ubootvarfs_var_by_name(data, dentry->name);
	if (!var)
		return NULL;

	inode = ubootvarfs_get_inode(dir->i_sb, dir, S_IFREG | 0777, var);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	d_add(dentry, inode);

	return NULL;
}

static int ubootvarfs_iterate(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = d_inode(dentry);
	struct ubootvarfs_inode *node = inode_to_node(inode);
	struct ubootvarfs_data *data = node->data;
	struct ubootvarfs_var *var;

	dir_emit_dots(file, ctx);

	list_for_each_entry(var, &data->var_list, list)
		dir_emit(ctx, var->name, var->name_len, 0, DT_REG);

	return 0;
}

static const struct file_operations ubootvarfs_dir_operations = {
	.iterate = ubootvarfs_iterate,
};

/**
 * ubootvarfs_relocate_tail() - Move all of the data after given inode by delta
 *
 * @node:	Inode marking the start of the data
 * @delta:	Offset to move the data by
 *
 * This function move all of the environment data that starts after
 * the given @node by @delta bytes. In case the data is moved towards
 * the start of the environment data blob trailing leftover data is
 * zeroed out
 */
static void ubootvarfs_relocate_tail(struct ubootvarfs_inode *node,
				     int delta)
{
	struct ubootvarfs_var *var = node->var;
	struct ubootvarfs_data *data = node->data;
	const size_t n = data->end - var->start;
	void *src = var->end + 1;

	memmove(src + delta, src, n);

	data->end += delta;

	if (delta < 0) {
		/*
		 * Remove all of the trailing leftovers
		 */
		memset(data->end, '\0', -delta);
	}
}

/**
 * ubootvarfs_adjust() - Adjust the size of a variable blob
 *
 * @node:	Inode marking where to start adjustement from
 * @delta:	Offset to adjust by
 *
 * This function move all of the environment data that starts after
 * the given @node by @delta bytes and updates all of the affected
 * ubootvarfs_var's in varaible linked list
 */
static void ubootvarfs_adjust(struct ubootvarfs_inode *node,
			      int delta)
{
	struct ubootvarfs_var *var = node->var;
	struct ubootvarfs_data *data = node->data;

	ubootvarfs_relocate_tail(node, delta);

	list_for_each_entry_continue(var, &data->var_list, list) {
		var->name += delta;
		var->start += delta;
		var->end += delta;
	}
}

static int ubootvarfs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);

	if (inode) {
		struct ubootvarfs_inode *node = inode_to_node(inode);
		struct ubootvarfs_var *var = node->var;
		/*
		 * -1 at the end is to account for '\0' at the end
		 * that needs to be removed as well
		 */
		const int delta = var->name - var->end - 1;

		ubootvarfs_adjust(node, delta);

		list_del(&var->list);
		free(var);
		node->var = NULL;
	}

	return simple_unlink(dir, dentry);
}

static int ubootvarfs_create(struct inode *dir, struct dentry *dentry,
			     umode_t mode)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	struct ubootvarfs_data *data = fsdev->dev.priv;
	struct inode *inode;
	struct ubootvarfs_var *var;
	size_t len = strlen(dentry->name);
	/*
	 * We'll be adding <varname>=\0\0 to the end of our data, so
	 * we need to make sure there's enough room for it. Note that
	 * + 3 is to accoutn for '=', and two '\0' from above
	 */
	if (data->end + len + 3 > data->limit)
		return -ENOSPC;

	var = xmalloc(sizeof(*var));

	var->name = data->end;
	memcpy(var->name, dentry->name, len);
	var->name_len = len;
	var->start = var->name + len;
	*var->start++ = '=';
	*var->start = '\0';
	var->end = var->start;
	data->end = var->end + 1;
	*data->end = '\0';

	list_add_tail(&var->list, &data->var_list);

	inode = ubootvarfs_get_inode(sb, dir, mode, var);
	d_instantiate(dentry, inode);

	return 0;
}

static const struct inode_operations ubootvarfs_dir_inode_operations = {
	.lookup = ubootvarfs_lookup,
	.unlink = ubootvarfs_unlink,
	.create = ubootvarfs_create,
};

static struct inode *ubootvarfs_alloc_inode(struct super_block *sb)
{
	struct ubootvarfs_inode *node;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	struct ubootvarfs_data *data = fsdev->dev.priv;

	node = xzalloc(sizeof(*node));
	node->data = data;

	return &node->inode;
}

static void ubootvarfs_destroy_inode(struct inode *inode)
{
	struct ubootvarfs_inode *node = inode_to_node(inode);

	free(node->var);
	free(node);
}

static const struct super_operations ubootvarfs_ops = {
	.alloc_inode = ubootvarfs_alloc_inode,
	.destroy_inode = ubootvarfs_destroy_inode,
};

static int ubootvarfs_io(struct file *f, void *buf, size_t insize, bool read)
{
	struct inode *inode = f->f_inode;
	struct ubootvarfs_inode *node = inode_to_node(inode);
	void *ptr = node->var->start + f->f_pos;

	if (read)
		memcpy(buf, ptr, insize);
	else
		memcpy(ptr, buf, insize);

	return insize;
}

static int ubootvarfs_read(struct file *f, void *buf, size_t insize)
{
	return ubootvarfs_io(f, buf, insize, true);
}

static int ubootvarfs_write(struct file *f, const void *buf, size_t insize)
{
	return ubootvarfs_io(f, (void *)buf, insize, false);
}

static int ubootvarfs_truncate(struct file *f, loff_t size)
{
	struct inode *inode = f->f_inode;
	struct ubootvarfs_inode *node = inode_to_node(inode);
	struct ubootvarfs_data *data = node->data;
	struct ubootvarfs_var *var = node->var;
	const int delta = size - inode->i_size;

	if (size == inode->i_size)
		return 0;

	if (data->end + delta >= data->limit)
		return -ENOSPC;

	ubootvarfs_adjust(node, delta);

	if (delta > 0)
		memset(var->end, '\0', delta);

	var->end += delta;
	*var->end = '\0';

	return 0;
}

static const struct file_operations ubootvarfs_file_operations = {
	.truncate = ubootvarfs_truncate,
	.read = ubootvarfs_read,
	.write = ubootvarfs_write,
};

static void ubootvarfs_parse(struct ubootvarfs_data *data, char *blob,
			     size_t size)
{
	struct ubootvarfs_var *var;
	const char *start = blob;
	size_t len;
	char *sep;

	data->limit = blob + size;
	INIT_LIST_HEAD(&data->var_list);

	while (*blob) {
		var = xmalloc(sizeof(*var));
		len = strnlen(blob, size);

		var->name = blob;
		var->end  = blob + len;

		sep = strchr(blob, '=');
		if (sep) {
			var->start = sep + 1;
			var->name_len = sep - blob;

			list_add_tail(&var->list, &data->var_list);
		} else {
			pr_err("No separator in data @ 0x%08tx. Skipped.",
			       blob - start);
			free(var);
		}

		len++; /* account for '\0' */
		size -= len;
		blob += len;
	};

	data->end = blob;
}

static int ubootvarfs_probe(struct device *dev)
{
	struct inode *inode;
	struct ubootvarfs_data *data = xzalloc(sizeof(*data));
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;
	struct stat s;
	void *map;
	int ret;

	dev->priv = data;

	data->fd = open(fsdev->backingstore, O_RDWR);
	if (data->fd < 0) {
		ret = -errno;
		goto free_data;
	}

	if (fstat(data->fd, &s) < 0) {
		ret = -errno;
		goto exit;
	}

	map = memmap(data->fd, PROT_READ | PROT_WRITE);
	if (map == MAP_FAILED) {
		ret = -errno;
		goto exit;
	}

	ubootvarfs_parse(data, map, s.st_size);

	sb->s_op = &ubootvarfs_ops;
	inode = ubootvarfs_get_inode(sb, NULL, S_IFDIR, NULL);
	sb->s_root = d_make_root(inode);

	/*
	 * We don't use cdev * directly, but this is needed for
	 * cdev_get_mount_path() to work right
	 */
	fsdev->cdev = cdev_by_name(devpath_to_name(fsdev->backingstore));

	return 0;
exit:
	close(data->fd);
free_data:
	free(data);
	return ret;
}

static void ubootvarfs_remove(struct device *dev)
{
	struct ubootvarfs_data *data = dev->priv;

	flush(data->fd);
	close(data->fd);
	free(data);
}

static struct fs_driver ubootvarfs_driver = {
	.type = filetype_ubootvar,
	.drv = {
		.probe = ubootvarfs_probe,
		.remove = ubootvarfs_remove,
		.name = "ubootvarfs",
	}
};

static int ubootvarfs_init(void)
{
	return register_fs_driver(&ubootvarfs_driver);
}
coredevice_initcall(ubootvarfs_init);
