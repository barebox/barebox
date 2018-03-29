/* * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 * Copyright (C) 2006, 2007 University of Szeged, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Artem Bityutskiy (Битюцкий Артём)
 *          Adrian Hunter
 *          Zoltan Sogor
 */

/*
 * This file implements directory operations.
 *
 * All FS operations in this file allocate budget before writing anything to the
 * media. If they fail to allocate it, the error is returned. The only
 * exceptions are 'ubifs_unlink()' and 'ubifs_rmdir()' which keep working even
 * if they unable to allocate the budget, because deletion %-ENOSPC failure is
 * not what users are usually ready to get. UBIFS budgeting subsystem has some
 * space reserved for these purposes.
 *
 * All operations in this file write all inodes which they change straight
 * away, instead of marking them dirty. For example, 'ubifs_link()' changes
 * @i_size of the parent inode and writes the parent inode together with the
 * target inode. This was done to simplify file-system recovery which would
 * otherwise be very difficult to do. The only exception is rename which marks
 * the re-named inode dirty (because its @i_ctime is updated) but does not
 * write it, but just marks it as dirty.
 */

#include "ubifs.h"

static int dbg_check_name(const struct ubifs_info *c,
			  const struct ubifs_dent_node *dent,
			  const struct qstr *nm)
{
	if (!dbg_is_chk_gen(c))
		return 0;
	if (le16_to_cpu(dent->nlen) != nm->len)
		return -EINVAL;
	if (memcmp(dent->name, nm->name, nm->len))
		return -EINVAL;
	return 0;
}

static struct dentry *ubifs_lookup(struct inode *dir, struct dentry *dentry,
				   unsigned int flags)
{
	int err;
	union ubifs_key key;
	struct inode *inode = NULL;
	struct ubifs_dent_node *dent;
	struct ubifs_info *c = dir->i_sb->s_fs_info;

	dbg_gen("'%pd' in dir ino %lu", dentry, dir->i_ino);

	if (dentry->d_name.len > UBIFS_MAX_NLEN)
		return ERR_PTR(-ENAMETOOLONG);

	dent = kmalloc(UBIFS_MAX_DENT_NODE_SZ, GFP_NOFS);
	if (!dent)
		return ERR_PTR(-ENOMEM);

	dent_key_init(c, &key, dir->i_ino, &dentry->d_name);

	err = ubifs_tnc_lookup_nm(c, &key, dent, &dentry->d_name);
	if (err) {
		if (err == -ENOENT) {
			dbg_gen("not found");
			goto done;
		}
		goto out;
	}

	if (dbg_check_name(c, dent, &dentry->d_name)) {
		err = -EINVAL;
		goto out;
	}

	inode = ubifs_iget(dir->i_sb, le64_to_cpu(dent->inum));
	if (IS_ERR(inode)) {
		/*
		 * This should not happen. Probably the file-system needs
		 * checking.
		 */
		err = PTR_ERR(inode);
		ubifs_err(c, "dead directory entry '%pd', error %d",
			dentry, err);
		ubifs_ro_mode(c, err);
		goto out;
	}

done:
	kfree(dent);
	/*
	 * Note, d_splice_alias() would be required instead if we supported
	 * NFS.
	 */
	d_add(dentry, inode);
	return NULL;

out:
	kfree(dent);
	return ERR_PTR(err);
}

/**
 * vfs_dent_type - get VFS directory entry type.
 * @type: UBIFS directory entry type
 *
 * This function converts UBIFS directory entry type into VFS directory entry
 * type.
 */
static unsigned int vfs_dent_type(uint8_t type)
{
	switch (type) {
	case UBIFS_ITYPE_REG:
		return DT_REG;
	case UBIFS_ITYPE_DIR:
		return DT_DIR;
	case UBIFS_ITYPE_LNK:
		return DT_LNK;
	case UBIFS_ITYPE_BLK:
		return DT_BLK;
	case UBIFS_ITYPE_CHR:
		return DT_CHR;
	case UBIFS_ITYPE_FIFO:
		return DT_FIFO;
	case UBIFS_ITYPE_SOCK:
		return DT_SOCK;
	default:
		BUG();
	}
	return 0;
}

/*
 * The classical Unix view for directory is that it is a linear array of
 * (name, inode number) entries. Linux/VFS assumes this model as well.
 * Particularly, 'readdir()' call wants us to return a directory entry offset
 * which later may be used to continue 'readdir()'ing the directory or to
 * 'seek()' to that specific direntry. Obviously UBIFS does not really fit this
 * model because directory entries are identified by keys, which may collide.
 *
 * UBIFS uses directory entry hash value for directory offsets, so
 * 'seekdir()'/'telldir()' may not always work because of possible key
 * collisions. But UBIFS guarantees that consecutive 'readdir()' calls work
 * properly by means of saving full directory entry name in the private field
 * of the file description object.
 *
 * This means that UBIFS cannot support NFS which requires full
 * 'seekdir()'/'telldir()' support.
 */
static int ubifs_readdir(struct file *file, struct dir_context *ctx)
{
	int err = 0;
	struct qstr nm;
	union ubifs_key key;
	struct ubifs_dent_node *dent;
	struct dentry *dentry = file->f_path.dentry;
	struct inode *dir = d_inode(dentry);
	struct ubifs_info *c = dir->i_sb->s_fs_info;

	dbg_gen("dir ino %lu, f_pos %#llx", dir->i_ino, ctx->pos);

	if (ctx->pos > UBIFS_S_KEY_HASH_MASK || ctx->pos == 2)
		/*
		 * The directory was seek'ed to a senseless position or there
		 * are no more entries.
		 */
		return 0;

	if (file->f_version == 0) {
		/*
		 * The file was seek'ed, which means that @file->private_data
		 * is now invalid. This may also be just the first
		 * 'ubifs_readdir()' invocation, in which case
		 * @file->private_data is NULL, and the below code is
		 * basically a no-op.
		 */
		kfree(file->private_data);
		file->private_data = NULL;
	}

	/*
	 * 'generic_file_llseek()' unconditionally sets @file->f_version to
	 * zero, and we use this for detecting whether the file was seek'ed.
	 */
	file->f_version = 1;

	/* File positions 0 and 1 correspond to "." and ".." */
	if (ctx->pos < 2) {
		ubifs_assert(!file->private_data);
		dir_emit_dots(file, ctx);

		/* Find the first entry in TNC and save it */
		lowest_dent_key(c, &key, dir->i_ino);
		nm.name = NULL;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}

		ctx->pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
	}

	dent = file->private_data;
	if (!dent) {
		/*
		 * The directory was seek'ed to and is now readdir'ed.
		 * Find the entry corresponding to @ctx->pos or the closest one.
		 */
		dent_key_init_hash(c, &key, dir->i_ino, ctx->pos);
		nm.name = NULL;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}
		ctx->pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
	}

	while (1) {
		dbg_gen("feed '%s', ino %llu, new f_pos %#x",
			dent->name, (unsigned long long)le64_to_cpu(dent->inum),
			key_hash_flash(c, &dent->key));
		ubifs_assert(le64_to_cpu(dent->ch.sqnum) >
			     ubifs_inode(dir)->creat_sqnum);

		nm.len = le16_to_cpu(dent->nlen);
		dir_emit(ctx, dent->name, nm.len,
			       le64_to_cpu(dent->inum),
			       vfs_dent_type(dent->type));

		/* Switch to the next entry */
		key_read(c, &dent->key, &key);
		nm.name = dent->name;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}

		kfree(file->private_data);
		ctx->pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
		cond_resched();
	}

out:
	kfree(file->private_data);
	file->private_data = NULL;

	if (err != -ENOENT)
		ubifs_err(c, "cannot find next direntry, error %d", err);
	else
		/*
		 * -ENOENT is a non-fatal error in this context, the TNC uses
		 * it to indicate that the cursor moved past the current directory
		 * and readdir() has to stop.
		 */
		err = 0;


	/* 2 is a special value indicating that there are no more direntries */
	ctx->pos = 2;
	return err;
}

const struct inode_operations ubifs_dir_inode_operations = {
	.lookup      = ubifs_lookup,
};

const struct file_operations ubifs_dir_operations = {
	.iterate        = ubifs_readdir,
};
