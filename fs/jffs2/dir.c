// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 */
#define pr_fmt(fmt) "jffs2: " fmt
#include <common.h>
#include <linux/fs.h>
#include <linux/hash.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/jffs2.h>
#include "jffs2_fs_i.h"
#include "jffs2_fs_sb.h"
#include <linux/time.h>
#include "nodelist.h"

static int jffs2_readdir (struct file *, struct dir_context *);

static struct dentry *jffs2_lookup (struct inode *,struct dentry *,
				    unsigned int);

const struct file_operations jffs2_dir_operations =
{
	.iterate = jffs2_readdir,
};

const struct inode_operations jffs2_dir_inode_operations =
{
	.lookup      =	jffs2_lookup,
	.create      = NULL, /* not present in barebox */
	.link        = NULL, /* not present in barebox */
	.symlink     = NULL, /* not present in barebox */
	.unlink      = NULL, /* not present in barebox */
	.mkdir       = NULL, /* not present in barebox */
	.rmdir       = NULL, /* not present in barebox */
};

/***********************************************************************/

/* We keep the dirent list sorted in increasing order of name hash,
   and we use the same hash function as the dentries. Makes this
   nice and simple
*/
static struct dentry *jffs2_lookup(struct inode *dir_i, struct dentry *target,
				   unsigned int flags)
{
	struct jffs2_inode_info *dir_f;
	struct jffs2_full_dirent *fd = NULL, *fd_list;
	uint32_t ino = 0;
	struct inode *inode = NULL;
	unsigned int nhash;

	jffs2_dbg(1, "jffs2_lookup()\n");

	if (target->d_name.len > JFFS2_MAX_NAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	dir_f = JFFS2_INODE_INFO(dir_i);

	/* The 'nhash' on the fd_list is not the same as the dentry hash */
	nhash = full_name_hash(NULL, target->d_name.name, target->d_name.len);

	mutex_lock(&dir_f->sem);

	/* NB: The 2.2 backport will need to explicitly check for '.' and '..' here */
	for (fd_list = dir_f->dents; fd_list && fd_list->nhash <= nhash; fd_list = fd_list->next) {
		if (fd_list->nhash == nhash &&
		    (!fd || fd_list->version > fd->version) &&
		    strlen(fd_list->name) == target->d_name.len &&
		    !strncmp(fd_list->name, target->d_name.name, target->d_name.len)) {
			fd = fd_list;
		}
	}
	if (fd)
		ino = fd->ino;
	mutex_unlock(&dir_f->sem);
	if (ino) {
		inode = jffs2_iget(dir_i->i_sb, ino);
		if (IS_ERR(inode))
			pr_warn("iget() failed for ino #%u\n", ino);
	}

	if (IS_ERR(inode))
		return ERR_CAST(inode);
	d_add(target, inode);
	return NULL;
}

/***********************************************************************/


static int jffs2_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);
	struct jffs2_inode_info *f = JFFS2_INODE_INFO(inode);
	struct jffs2_full_dirent *fd;
	unsigned long curofs = 1;

	jffs2_dbg(1, "jffs2_readdir() for dir_i #%lu\n", inode->i_ino);

	if (!dir_emit_dots(file, ctx))
		return 0;

	mutex_lock(&f->sem);
	for (fd = f->dents; fd; fd = fd->next) {
		curofs++;
		/* First loop: curofs = 2; pos = 2 */
		if (curofs < ctx->pos) {
			jffs2_dbg(2, "Skipping dirent: \"%s\", ino #%u, type %d, because curofs %ld < offset %ld\n",
				  fd->name, fd->ino, fd->type, curofs, (unsigned long)ctx->pos);
			continue;
		}
		if (!fd->ino) {
			jffs2_dbg(2, "Skipping deletion dirent \"%s\"\n",
				  fd->name);
			ctx->pos++;
			continue;
		}
		jffs2_dbg(2, "Dirent %ld: \"%s\", ino #%u, type %d\n",
			  (unsigned long)ctx->pos, fd->name, fd->ino, fd->type);
		if (!dir_emit(ctx, fd->name, strlen(fd->name), fd->ino, fd->type))
			break;
		ctx->pos++;
	}
	mutex_unlock(&f->sem);
	return 0;
}
