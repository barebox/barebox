// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contains vfs inode ops for the 9P2000.L protocol.
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/uidgid.h>
#include <linux/pagemap.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/kdev_t.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

static int
v9fs_vfs_mknod_dotl(struct inode *dir,
		    struct dentry *dentry, umode_t omode, dev_t rdev);

/**
 * v9fs_get_fsgid_for_create - Helper function to get the gid for a new object
 * @dir_inode: The directory inode
 *
 * Helper function to get the gid for creating a
 * new file system object. This checks the S_ISGID to determine the owning
 * group of the new file system object.
 */

static kgid_t v9fs_get_fsgid_for_create(struct inode *dir_inode)
{
	BUG_ON(dir_inode == NULL);

	if (dir_inode->i_mode & S_ISGID) {
		/* set_gid bit is set.*/
		return make_kgid(dir_inode->i_gid);
	}
	return make_kgid(0);
}

struct inode *
v9fs_fid_iget_dotl(struct super_block *sb, struct p9_fid *fid, bool new)
{
	int retval;
	struct inode *inode;
	struct p9_stat_dotl *st;
	struct v9fs_session_info *v9ses = sb->s_fs_info;

	inode = iget_locked(sb, QID2INO(&fid->qid));
	if (unlikely(!inode))
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW)) {
		if (!new)
			goto done;
		else /* deal with race condition in inode number reuse */
			return ERR_PTR(-EBUSY);
	}

	/*
	 * initialize the inode with the stat info
	 * FIXME!! we may need support for stale inodes
	 * later.
	 */
	st = p9_client_getattr_dotl(fid, P9_STATS_BASIC | P9_STATS_GEN);
	if (IS_ERR(st)) {
		retval = PTR_ERR(st);
		goto error;
	}

	retval = v9fs_init_inode(v9ses, inode, &fid->qid,
				 st->st_mode, new_decode_dev(st->st_rdev));
	v9fs_stat2inode_dotl(st, inode);
	kfree(st);
	if (retval)
		goto error;

	v9fs_set_netfs_context(inode);

done:
	return inode;
error:
	iget_failed(inode);
	return ERR_PTR(retval);
}

struct dotl_openflag_map {
	int open_flag;
	int dotl_flag;
};

static int v9fs_mapped_dotl_flags(int flags)
{
	int i;
	int rflags = 0;
	struct dotl_openflag_map dotl_oflag_map[] = {
		{ O_CREAT,	P9_DOTL_CREATE },
		{ O_EXCL,	P9_DOTL_EXCL },
	/*
	 * fs.c __write emulates O_APPEND by calling ->truncate first,
	 * so repeating append operation here, means we set an offset
	 * twice as big as intended.
	 * TODO: find out how Linux handles it, but for now skipping
	 * it seems t do the job.
	 *
	 *	{ O_APPEND,	P9_DOTL_APPEND },
	 */
		{ O_DIRECTORY,	P9_DOTL_DIRECTORY },
		{ O_NOFOLLOW,	P9_DOTL_NOFOLLOW },
	};
	for (i = 0; i < ARRAY_SIZE(dotl_oflag_map); i++) {
		if (flags & dotl_oflag_map[i].open_flag)
			rflags |= dotl_oflag_map[i].dotl_flag;
	}
	return rflags;
}

/**
 * v9fs_open_to_dotl_flags- convert Linux specific open flags to
 * plan 9 open flag.
 * @flags: flags to convert
 */
int v9fs_open_to_dotl_flags(int flags)
{
	int rflags = 0;

	/*
	 * We have same bits for P9_DOTL_READONLY, P9_DOTL_WRONLY
	 * and P9_DOTL_NOACCESS
	 */
	rflags |= flags & O_ACCMODE;
	rflags |= v9fs_mapped_dotl_flags(flags);

	return rflags;
}

/**
 * v9fs_vfs_create_dotl - VFS hook to create files for 9P2000.L protocol.
 * @dir: directory inode that is being created
 * @dentry:  dentry that is being deleted
 * @omode: create permissions
 * @excl: True if the file must not yet exist
 *
 */
static __maybe_unused int
v9fs_vfs_create_dotl(struct inode *dir,
		     struct dentry *dentry, umode_t omode)
{
	return v9fs_vfs_mknod_dotl(dir, dentry, omode, 0);
}

/**
 * v9fs_vfs_mkdir_dotl - VFS mkdir hook to create a directory
 * @dir:  inode that is being unlinked
 * @dentry: dentry that is being unlinked
 * @omode: mode for new directory
 *
 */

static __maybe_unused
int v9fs_vfs_mkdir_dotl(struct inode *dir, struct dentry *dentry,
			umode_t omode)
{
	int err;
	struct p9_fid *fid = NULL, *dfid = NULL;
	kgid_t gid;
	const unsigned char *name;
	umode_t mode;
	struct inode *inode;
	struct p9_qid qid;

	p9_debug(P9_DEBUG_VFS, "name %s\n", dentry->d_name.name);

	omode |= S_IFDIR;
	if (dir->i_mode & S_ISGID)
		omode |= S_ISGID;

	dfid = v9fs_parent_fid(dentry);
	if (IS_ERR(dfid)) {
		err = PTR_ERR(dfid);
		p9_debug(P9_DEBUG_VFS, "fid lookup failed %d\n", err);
		goto error;
	}

	gid = v9fs_get_fsgid_for_create(dir);
	mode = omode;
	name = dentry->d_name.name;
	err = p9_client_mkdir_dotl(dfid, name, mode, gid, &qid);
	if (err < 0)
		goto error;
	fid = p9_client_walk(dfid, 1, &name, 1);
	if (IS_ERR(fid)) {
		err = PTR_ERR(fid);
		p9_debug(P9_DEBUG_VFS, "p9_client_walk failed %d\n",
			 err);
		goto error;
	}

	/* instantiate inode and assign the unopened fid to the dentry */
	inode = v9fs_fid_iget_dotl(dir->i_sb, fid, true);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		p9_debug(P9_DEBUG_VFS, "inode creation failed %d\n",
			 err);
		goto error;
	}
	v9fs_fid_add(dentry, &fid);
	d_instantiate(dentry, inode);
	err = 0;
	inc_nlink(dir);
	v9fs_invalidate_inode_attr(dir);
error:
	p9_fid_put(fid);
	p9_fid_put(dfid);
	return err;
}

/*
 * Attribute flags.
 */
#define P9_ATTR_MODE		(1 << 0)
#define P9_ATTR_UID		(1 << 1)
#define P9_ATTR_GID		(1 << 2)
#define P9_ATTR_SIZE		(1 << 3)
#define P9_ATTR_ATIME		(1 << 4)
#define P9_ATTR_MTIME		(1 << 5)
#define P9_ATTR_CTIME		(1 << 6)
#define P9_ATTR_ATIME_SET	(1 << 7)
#define P9_ATTR_MTIME_SET	(1 << 8)

struct dotl_iattr_map {
	int iattr_valid;
	int p9_iattr_valid;
};

static int v9fs_mapped_iattr_valid(int iattr_valid)
{
	int i;
	int p9_iattr_valid = 0;
	struct dotl_iattr_map dotl_iattr_map[] = {
		{ ATTR_SIZE,		P9_ATTR_SIZE },
	};
	for (i = 0; i < ARRAY_SIZE(dotl_iattr_map); i++) {
		if (iattr_valid & dotl_iattr_map[i].iattr_valid)
			p9_iattr_valid |= dotl_iattr_map[i].p9_iattr_valid;
	}
	return p9_iattr_valid;
}

/**
 * v9fs_vfs_setattr_dotl - set file metadata
 * @dentry: file whose metadata to set
 * @iattr: metadata assignment structure
 *
 */

static int v9fs_vfs_setattr_dotl(struct dentry *dentry, struct iattr *iattr)
{
	int retval, use_dentry = 0;
	struct inode *inode = d_inode(dentry);
	struct p9_fid *fid = NULL;
	struct p9_iattr_dotl p9attr = {
		.uid = INVALID_UID,
		.gid = INVALID_GID,
	};

	p9_debug(P9_DEBUG_VFS, "\n");

	p9attr.valid = v9fs_mapped_iattr_valid(iattr->ia_valid);
	if (iattr->ia_valid & ATTR_SIZE)
		p9attr.size = iattr->ia_size;

	if (iattr->ia_valid & ATTR_FILE) {
		fid = iattr->ia_file->private_data;
		WARN_ON(!fid);
	}
	if (!fid) {
		fid = v9fs_fid_lookup(dentry);
		use_dentry = 1;
	}
	if (IS_ERR(fid))
		return PTR_ERR(fid);

	retval = p9_client_setattr(fid, &p9attr);
	if (retval < 0) {
		if (use_dentry)
			p9_fid_put(fid);
		return retval;
	}

	if ((iattr->ia_valid & ATTR_SIZE) && iattr->ia_size !=
		 i_size_read(inode)) {
		truncate_setsize(inode, iattr->ia_size);
	}

	v9fs_invalidate_inode_attr(inode);
	mark_inode_dirty(inode);
	if (use_dentry)
		p9_fid_put(fid);

	return 0;
}

static __maybe_unused int
v9fs_truncate(struct device *dev, struct file *f, loff_t size)
{
	struct iattr iattr = {
		.ia_valid = ATTR_SIZE | ATTR_FILE,
		.ia_size = size,
		.ia_file = f,
	};

	return v9fs_vfs_setattr_dotl(f->f_path.dentry, &iattr);
}

/**
 * v9fs_stat2inode_dotl - populate an inode structure with stat info
 * @stat: stat structure
 * @inode: inode to populate
 *
 */

void
v9fs_stat2inode_dotl(struct p9_stat_dotl *stat, struct inode *inode)
{
	umode_t mode;

	if ((stat->st_result_mask & P9_STATS_BASIC) == P9_STATS_BASIC) {
		inode->i_uid = from_kuid(stat->st_uid);
		inode->i_gid = from_kgid(stat->st_gid);
		set_nlink(inode, stat->st_nlink);

		mode = stat->st_mode & S_IALLUGO;
		mode |= inode->i_mode & ~S_IALLUGO;
		inode->i_mode = mode;

		v9fs_i_size_write(inode, stat->st_size);
		inode->i_blocks = stat->st_blocks;
	} else {
		if (stat->st_result_mask & P9_STATS_UID)
			inode->i_uid = from_kuid(stat->st_uid);
		if (stat->st_result_mask & P9_STATS_GID)
			inode->i_gid = from_kgid(stat->st_gid);
		if (stat->st_result_mask & P9_STATS_NLINK)
			set_nlink(inode, stat->st_nlink);
		if (stat->st_result_mask & P9_STATS_MODE) {
			mode = stat->st_mode & S_IALLUGO;
			mode |= inode->i_mode & ~S_IALLUGO;
			inode->i_mode = mode;
		}
		if (stat->st_result_mask & P9_STATS_SIZE) {
			v9fs_i_size_write(inode, stat->st_size);
		}
		if (stat->st_result_mask & P9_STATS_BLOCKS)
			inode->i_blocks = stat->st_blocks;
	}
	if (stat->st_result_mask & P9_STATS_GEN)
		inode->i_generation = stat->st_gen;
}

static __maybe_unused int
v9fs_vfs_symlink_dotl(struct inode *dir,
		      struct dentry *dentry, const char *symname)
{
	int err;
	kgid_t gid;
	const unsigned char *name;
	struct p9_qid qid;
	struct p9_fid *dfid;
	struct p9_fid *fid = NULL;

	name = dentry->d_name.name;
	p9_debug(P9_DEBUG_VFS, "%lu,%s,%s\n", dir->i_ino, name, symname);

	dfid = v9fs_parent_fid(dentry);
	if (IS_ERR(dfid)) {
		err = PTR_ERR(dfid);
		p9_debug(P9_DEBUG_VFS, "fid lookup failed %d\n", err);
		return err;
	}

	gid = v9fs_get_fsgid_for_create(dir);

	/* Server doesn't alter fid on TSYMLINK. Hence no need to clone it. */
	err = p9_client_symlink(dfid, name, symname, gid, &qid);

	if (err < 0) {
		p9_debug(P9_DEBUG_VFS, "p9_client_symlink failed %d\n", err);
		goto error;
	}

	v9fs_invalidate_inode_attr(dir);

error:
	p9_fid_put(fid);
	p9_fid_put(dfid);
	return err;
}

/**
 * v9fs_vfs_link_dotl - create a hardlink for dotl
 * @old_dentry: dentry for file to link to
 * @dir: inode destination for new link
 * @dentry: dentry for link
 *
 */

static __maybe_unused int
v9fs_vfs_link_dotl(struct dentry *old_dentry, struct inode *dir,
		struct dentry *dentry)
{
	int err;
	struct p9_fid *dfid, *oldfid;
	struct v9fs_session_info *v9ses;

	p9_debug(P9_DEBUG_VFS, "dir ino: %lu, old_name: %s, new_name: %s\n",
		 dir->i_ino, old_dentry->d_name.name, dentry->d_name.name);

	v9ses = v9fs_inode2v9ses(dir);
	dfid = v9fs_parent_fid(dentry);
	if (IS_ERR(dfid))
		return PTR_ERR(dfid);

	oldfid = v9fs_fid_lookup(old_dentry);
	if (IS_ERR(oldfid)) {
		p9_fid_put(dfid);
		return PTR_ERR(oldfid);
	}

	err = p9_client_link(dfid, oldfid, dentry->d_name.name);

	p9_fid_put(dfid);
	p9_fid_put(oldfid);
	if (err < 0) {
		p9_debug(P9_DEBUG_VFS, "p9_client_link failed %d\n", err);
		return err;
	}

	v9fs_invalidate_inode_attr(dir);
	ihold(d_inode(old_dentry));
	d_instantiate(dentry, d_inode(old_dentry));

	return err;
}

/**
 * v9fs_vfs_mknod_dotl - create a special file
 * @dir: inode destination for new link
 * @dentry: dentry for file
 * @omode: mode for creation
 * @rdev: device associated with special file
 *
 */
static int
v9fs_vfs_mknod_dotl(struct inode *dir,
		    struct dentry *dentry, umode_t omode, dev_t rdev)
{
	int err;
	kgid_t gid;
	const unsigned char *name;
	umode_t mode;
	struct p9_fid *fid = NULL, *dfid = NULL;
	struct inode *inode;
	struct p9_qid qid;

	p9_debug(P9_DEBUG_VFS, " %lu,%s mode: %x MAJOR: %u MINOR: %u\n",
		 dir->i_ino, dentry->d_name.name, omode,
		 MAJOR(rdev), MINOR(rdev));

	dfid = v9fs_parent_fid(dentry);
	if (IS_ERR(dfid)) {
		err = PTR_ERR(dfid);
		p9_debug(P9_DEBUG_VFS, "fid lookup failed %d\n", err);
		goto error;
	}

	gid = v9fs_get_fsgid_for_create(dir);
	mode = omode;
	name = dentry->d_name.name;

	err = p9_client_mknod_dotl(dfid, name, mode, rdev, gid, &qid);
	if (err < 0)
		goto error;

	v9fs_invalidate_inode_attr(dir);
	fid = p9_client_walk(dfid, 1, &name, 1);
	if (IS_ERR(fid)) {
		err = PTR_ERR(fid);
		p9_debug(P9_DEBUG_VFS, "p9_client_walk failed %d\n",
			 err);
		goto error;
	}
	inode = v9fs_fid_iget_dotl(dir->i_sb, fid, true);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		p9_debug(P9_DEBUG_VFS, "inode creation failed %d\n",
			 err);
		goto error;
	}
	v9fs_fid_add(dentry, &fid);
	d_instantiate(dentry, inode);
	err = 0;
error:
	p9_fid_put(fid);
	p9_fid_put(dfid);

	return err;
}

/**
 * v9fs_vfs_get_link_dotl - follow a symlink path
 * @dentry: dentry for symlink
 * @inode: inode for symlink
 * @done: destructor for return value
 */

static const char *
v9fs_vfs_get_link_dotl(struct dentry *dentry,
		       struct inode *inode)
{
	struct p9_fid *fid;
	char *target;
	int retval;

	if (!dentry)
		return ERR_PTR(-ECHILD);

	p9_debug(P9_DEBUG_VFS, "%s\n", dentry->d_name.name);

	fid = v9fs_fid_lookup(dentry);
	if (IS_ERR(fid))
		return ERR_CAST(fid);
	retval = p9_client_readlink(fid, &target);
	p9_fid_put(fid);
	if (retval)
		return ERR_PTR(retval);
	return target;
}

const struct inode_operations v9fs_dir_inode_operations_dotl = {
	.lookup = v9fs_vfs_lookup,
#ifdef CONFIG_9P_FS_WRITE
	.create = v9fs_vfs_create_dotl,
	.link = v9fs_vfs_link_dotl,
	.symlink = v9fs_vfs_symlink_dotl,
	.unlink = v9fs_vfs_unlink,
	.mkdir = v9fs_vfs_mkdir_dotl,
	.rmdir = v9fs_vfs_rmdir,
#endif
};

const struct inode_operations v9fs_file_inode_operations_dotl;

const struct inode_operations v9fs_symlink_inode_operations_dotl = {
	.get_link = v9fs_vfs_get_link_dotl,
};

struct fs_driver v9fs_driver = {
	.read      = v9fs_read,
#ifdef CONFIG_9P_FS_WRITE
	.write     = v9fs_write,
	.truncate  = v9fs_truncate,
	.flush     = v9fs_file_fsync_dotl,
#endif
	.drv = {
		.probe  = v9fs_mount,
		.remove = v9fs_umount,
		.name = "9p",
	}
};
