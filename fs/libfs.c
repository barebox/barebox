/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */
#include <common.h>
#include <fs.h>
#include <linux/fs.h>

/*
 * Lookup the data. This is trivial - if the dentry didn't already
 * exist, we know it is negative.  Set d_op to delete negative dentries.
 */
struct dentry *simple_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	return NULL;
}

/* Relationship between i_mode and the DT_xxx types */
static inline unsigned char dt_type(struct inode *inode)
{
	return (inode->i_mode >> 12) & 15;
}

int dcache_readdir(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct dentry *d;

	dir_emit_dots(file, ctx);

	list_for_each_entry(d, &dentry->d_subdirs, d_child) {
		if (d_is_negative(d))
			continue;
		dir_emit(ctx, d->d_name.name, d->d_name.len,
				d_inode(d)->i_ino, dt_type(d_inode(d)));
	}

        return 0;
}

const struct file_operations simple_dir_operations = {
	.iterate = dcache_readdir,
};

int simple_empty(struct dentry *dentry)
{
	struct dentry *child;
	int ret = 0;

	list_for_each_entry(child, &dentry->d_subdirs, d_child) {
		if (d_is_positive(child))
			goto out;
	}
	ret = 1;
out:
	return ret;
}

int simple_unlink(struct inode *dir, struct dentry *dentry)
{
	struct inode *inode = d_inode(dentry);

	drop_nlink(inode);
	dput(dentry);

	return 0;
}

int simple_rmdir(struct inode *dir, struct dentry *dentry)
{
	if (IS_ROOT(dentry))
		return -EBUSY;

	if (!simple_empty(dentry))
		return -ENOTEMPTY;

	drop_nlink(d_inode(dentry));
	simple_unlink(dir, dentry);
	drop_nlink(dir);

	return 0;
}

const char *simple_get_link(struct dentry *dentry, struct inode *inode)
{
	return inode->i_link;
}

const struct inode_operations simple_symlink_inode_operations = {
	.get_link = simple_get_link,
};
