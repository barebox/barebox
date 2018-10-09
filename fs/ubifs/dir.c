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

/*
 * removed in barebox
static int inherit_flags(const struct inode *dir, umode_t mode)
 */

/*
 * removed in barebox
struct inode *ubifs_new_inode(struct ubifs_info *c, struct inode *dir,
			      umode_t mode)
 */

static int dbg_check_name(const struct ubifs_info *c,
			  const struct ubifs_dent_node *dent,
			  const struct fscrypt_name *nm)
{
	if (!dbg_is_chk_gen(c))
		return 0;
	if (le16_to_cpu(dent->nlen) != fname_len(nm))
		return -EINVAL;
	if (memcmp(dent->name, fname_name(nm), fname_len(nm)))
		return -EINVAL;
	return 0;
}

static struct dentry *ubifs_lookup(struct inode *dir, struct dentry *dentry,
				   unsigned int flags)
{
	int err;
	union ubifs_key key;
	struct inode *inode = NULL;
	struct ubifs_dent_node *dent = NULL;
	struct ubifs_info *c = dir->i_sb->s_fs_info;
	struct fscrypt_name nm;

	dbg_gen("'%pd' in dir ino %lu", dentry, dir->i_ino);

	err = fscrypt_prepare_lookup(dir, dentry, flags);
	if (err)
		return ERR_PTR(err);

	err = fscrypt_setup_filename(dir, &dentry->d_name, 1, &nm);
	if (err)
		return ERR_PTR(err);

	if (fname_len(&nm) > UBIFS_MAX_NLEN) {
		inode = ERR_PTR(-ENAMETOOLONG);
		goto done;
	}

	dent = kmalloc(UBIFS_MAX_DENT_NODE_SZ, GFP_NOFS);
	if (!dent) {
		inode = ERR_PTR(-ENOMEM);
		goto done;
	}

	if (nm.hash) {
		ubifs_assert(c, fname_len(&nm) == 0);
		ubifs_assert(c, fname_name(&nm) == NULL);
		dent_key_init_hash(c, &key, dir->i_ino, nm.hash);
		err = ubifs_tnc_lookup_dh(c, &key, dent, nm.minor_hash);
	} else {
		dent_key_init(c, &key, dir->i_ino, &nm);
		err = ubifs_tnc_lookup_nm(c, &key, dent, &nm);
	}

	if (err) {
		if (err == -ENOENT)
			dbg_gen("not found");
		else
			inode = ERR_PTR(err);
		goto done;
	}

	if (dbg_check_name(c, dent, &nm)) {
		inode = ERR_PTR(-EINVAL);
		goto done;
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
		goto done;
	}

	if (ubifs_crypt_is_encrypted(dir) &&
	    (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode)) &&
	    !fscrypt_has_permitted_context(dir, inode)) {
		ubifs_warn(c, "Inconsistent encryption contexts: %lu/%lu",
			   dir->i_ino, inode->i_ino);
		iput(inode);
		inode = ERR_PTR(-EPERM);
	}

done:
	kfree(dent);
	fscrypt_free_filename(&nm);
	d_add(dentry, inode);
	return NULL;
}

/*
 * removed in barebox
static int ubifs_create(struct inode *dir, struct dentry *dentry, umode_t mode,
			bool excl)
 */

/*
 * removed in barebox
static int do_tmpfile(struct inode *dir, struct dentry *dentry,
		      umode_t mode, struct inode **whiteout)
 */

/*
 * removed in barebox
static int ubifs_tmpfile(struct inode *dir, struct dentry *dentry,
			 umode_t mode)
 */

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
	int fstr_real_len = 0, err = 0;
	struct fscrypt_name nm;
	struct fscrypt_str fstr = {0};
	union ubifs_key key;
	struct ubifs_dent_node *dent;
	struct inode *dir = file_inode(file);
	struct ubifs_info *c = dir->i_sb->s_fs_info;
	bool encrypted = ubifs_crypt_is_encrypted(dir);

	dbg_gen("dir ino %lu, f_pos %#llx", dir->i_ino, ctx->pos);

	if (ctx->pos > UBIFS_S_KEY_HASH_MASK || ctx->pos == 2)
		/*
		 * The directory was seek'ed to a senseless position or there
		 * are no more entries.
		 */
		return 0;

	if (encrypted) {
		err = fscrypt_get_encryption_info(dir);
		if (err && err != -ENOKEY)
			return err;

		err = fscrypt_fname_alloc_buffer(dir, UBIFS_MAX_NLEN, &fstr);
		if (err)
			return err;

		fstr_real_len = fstr.len;
	}

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
		ubifs_assert(c, !file->private_data);
		if (!dir_emit_dots(file, ctx)) {
			if (encrypted)
				fscrypt_fname_free_buffer(&fstr);
			return 0;
		}

		/* Find the first entry in TNC and save it */
		lowest_dent_key(c, &key, dir->i_ino);
		fname_len(&nm) = 0;
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
		fname_len(&nm) = 0;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}
		ctx->pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
	}

	while (1) {
		dbg_gen("ino %llu, new f_pos %#x",
			(unsigned long long)le64_to_cpu(dent->inum),
			key_hash_flash(c, &dent->key));
		ubifs_assert(c, le64_to_cpu(dent->ch.sqnum) >
			     ubifs_inode(dir)->creat_sqnum);

		fname_len(&nm) = le16_to_cpu(dent->nlen);
		fname_name(&nm) = dent->name;

		if (encrypted) {
			fstr.len = fstr_real_len;

			err = fscrypt_fname_disk_to_usr(dir, key_hash_flash(c,
							&dent->key),
							le32_to_cpu(dent->cookie),
							&nm.disk_name, &fstr);
			if (err)
				goto out;
		} else {
			fstr.len = fname_len(&nm);
			fstr.name = fname_name(&nm);
		}

		if (!dir_emit(ctx, fstr.name, fstr.len,
			       le64_to_cpu(dent->inum),
			       vfs_dent_type(dent->type))) {
			if (encrypted)
				fscrypt_fname_free_buffer(&fstr);
			return 0;
		}

		/* Switch to the next entry */
		key_read(c, &dent->key, &key);
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

	if (encrypted)
		fscrypt_fname_free_buffer(&fstr);

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

/*
 * removed in barebox
static int ubifs_dir_release(struct inode *dir, struct file *file)
 */

/*
 * removed in barebox
static void lock_2_inodes(struct inode *inode1, struct inode *inode2)
 */

/*
 * removed in barebox
static void unlock_2_inodes(struct inode *inode1, struct inode *inode2)
 */

/*
 * removed in barebox
static int ubifs_link(struct dentry *old_dentry, struct inode *dir,
		      struct dentry *dentry)
 */

/*
 * removed in barebox
static int ubifs_unlink(struct inode *dir, struct dentry *dentry)
 */

/*
 * removed in barebox
int ubifs_check_dir_empty(struct inode *dir)
 */

/*
 * removed in barebox
static int ubifs_rmdir(struct inode *dir, struct dentry *dentry)
 */

/*
 * removed in barebox
static int ubifs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
 */

/*
 * removed in barebox
static int ubifs_mknod(struct inode *dir, struct dentry *dentry,
		       umode_t mode, dev_t rdev)
 */

/*
 * removed in barebox
static int ubifs_symlink(struct inode *dir, struct dentry *dentry,
			 const char *symname)
 */

/*
 * removed in barebox
static void lock_4_inodes(struct inode *inode1, struct inode *inode2,
			  struct inode *inode3, struct inode *inode4)
 */

/*
 * removed in barebox
static void unlock_4_inodes(struct inode *inode1, struct inode *inode2,
			    struct inode *inode3, struct inode *inode4)
 */

/*
 * removed in barebox
static int do_rename(struct inode *old_dir, struct dentry *old_dentry,
		     struct inode *new_dir, struct dentry *new_dentry,
		     unsigned int flags)
 */

/*
 * removed in barebox
static int ubifs_xrename(struct inode *old_dir, struct dentry *old_dentry,
			struct inode *new_dir, struct dentry *new_dentry)
 */

/*
 * removed in barebox
static int ubifs_rename(struct inode *old_dir, struct dentry *old_dentry,
			struct inode *new_dir, struct dentry *new_dentry,
			unsigned int flags)
 */

/*
 * removed in barebox
int ubifs_getattr(const struct path *path, struct kstat *stat,
		  u32 request_mask, unsigned int flags)
 */

/*
 * removed in barebox
static int ubifs_dir_open(struct inode *dir, struct file *file)
 */

const struct inode_operations ubifs_dir_inode_operations = {
	.lookup      = ubifs_lookup,
	.create      = NULL, /* not present in barebox */
	.link        = NULL, /* not present in barebox */
	.symlink     = NULL, /* not present in barebox */
	.unlink      = NULL, /* not present in barebox */
	.mkdir       = NULL, /* not present in barebox */
	.rmdir       = NULL, /* not present in barebox */
/* 	.mknod       = NULL, not present in barebox */
	.rename      = NULL, /* not present in barebox */
/*	.setattr     = NULL, not present in barebox */
/* 	.getattr     = NULL, not present in barebox */
#ifdef CONFIG_UBIFS_FS_XATTR
	.listxattr   = NULL, /* not present in barebox */
#endif
#ifdef CONFIG_UBIFS_ATIME_SUPPORT
	.update_time = NULL, /* not present in barebox */
#endif
/*	.tmpfile     = NULL, not present in barebox */
};

const struct file_operations ubifs_dir_operations = {
/* 	.llseek         = NULL, not present in barebox */
/* 	.release        = NULL, not present in barebox */
	.read           = NULL, /* not present in barebox */
	.iterate	= ubifs_readdir,
/* 	.fsync          = NULL, not present in barebox */
/*	.unlocked_ioctl = NULL, not present in barebox */
/* 	.open		= NULL, not present in barebox */
#ifdef CONFIG_COMPAT
	.compat_ioctl   = NULL, /* not present in barebox */
#endif
};
