// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contains vfs inode ops for the 9P2000 protocol.
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/netfs.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

/**
 * v9fs_alloc_inode - helper function to allocate an inode
 * @sb: The superblock to allocate the inode from
 */
struct inode *v9fs_alloc_inode(struct super_block *sb)
{
	struct v9fs_inode *v9inode;

	v9inode = xzalloc(sizeof(*v9inode));

	mutex_init(&v9inode->v_mutex);
	return &v9inode->netfs.inode;
}

/**
 * v9fs_free_inode - destroy an inode
 * @inode: The inode to be freed
 */

void v9fs_free_inode(struct inode *inode)
{
	struct v9fs_inode *v9inode = V9FS_I(inode);

	free(v9inode);
}

/*
 * Set parameters for the netfs library
 */
void v9fs_set_netfs_context(struct inode *inode)
{
	struct v9fs_inode *v9inode = V9FS_I(inode);
	netfs_inode_init(&v9inode->netfs);
}

int v9fs_init_inode(struct v9fs_session_info *v9ses,
		    struct inode *inode, struct p9_qid *qid, umode_t mode, dev_t rdev)
{
	int err = 0;
	struct v9fs_inode *v9inode = V9FS_I(inode);

	v9inode->qid = *qid;

	inode_init_owner(inode, NULL, mode);
	inode->i_blocks = 0;
	inode->i_private = NULL;

	switch (mode & S_IFMT) {
	case S_IFIFO:
	case S_IFBLK:
	case S_IFCHR:
	case S_IFSOCK:
	case S_IFREG:
		inode->i_op = &v9fs_file_inode_operations_dotl;
		inode->i_fop = &v9fs_file_operations_dotl;

		break;
	case S_IFLNK:
		inode->i_op = &v9fs_symlink_inode_operations_dotl;

		break;
	case S_IFDIR:
		inc_nlink(inode);
		inode->i_op = &v9fs_dir_inode_operations_dotl;
		inode->i_fop = &v9fs_dir_operations_dotl;

		break;
	default:
		p9_debug(P9_DEBUG_ERROR, "BAD mode 0x%hx S_IFMT 0x%x\n",
			 mode, mode & S_IFMT);
		err = -EINVAL;
		goto error;
	}
error:
	return err;

}

/**
 * v9fs_at_to_dotl_flags- convert Linux specific AT flags to
 * plan 9 AT flag.
 * @flags: flags to convert
 */
static int v9fs_at_to_dotl_flags(int flags)
{
	int rflags = 0;

	if (flags & AT_REMOVEDIR)
		rflags |= P9_DOTL_AT_REMOVEDIR;

	return rflags;
}

/**
 * v9fs_dec_count - helper functon to drop i_nlink.
 *
 * If a directory had nlink <= 2 (including . and ..), then we should not drop
 * the link count, which indicates the underlying exported fs doesn't maintain
 * nlink accurately. e.g.
 * - overlayfs sets nlink to 1 for merged dir
 * - ext4 (with dir_nlink feature enabled) sets nlink to 1 if a dir has more
 *   than EXT4_LINK_MAX (65000) links.
 *
 * @inode: inode whose nlink is being dropped
 */
static void v9fs_dec_count(struct inode *inode)
{
	if (!S_ISDIR(inode->i_mode) || inode->i_nlink > 2) {
		if (inode->i_nlink) {
			drop_nlink(inode);
		} else {
			p9_debug(P9_DEBUG_VFS,
						"WARNING: unexpected i_nlink zero %d inode %ld\n",
						inode->i_nlink, inode->i_ino);
		}
	}
}

/**
 * v9fs_remove - helper function to remove files and directories
 * @dir: directory inode that is being deleted
 * @dentry:  dentry that is being deleted
 * @flags: removing a directory
 *
 */

static int v9fs_remove(struct inode *dir, struct dentry *dentry, int flags)
{
	struct inode *inode;
	int retval = -EOPNOTSUPP;
	struct p9_fid *v9fid, *dfid;
	struct v9fs_session_info *v9ses;

	p9_debug(P9_DEBUG_VFS, "inode: %p dentry: %p rmdir: %x\n",
		 dir, dentry, flags);

	v9ses = v9fs_inode2v9ses(dir);
	inode = d_inode(dentry);
	dfid = v9fs_parent_fid(dentry);
	if (IS_ERR(dfid)) {
		retval = PTR_ERR(dfid);
		p9_debug(P9_DEBUG_VFS, "fid lookup failed %d\n", retval);
		return retval;
	}
	retval = p9_client_unlinkat(dfid, dentry->d_name.name,
				    v9fs_at_to_dotl_flags(flags));
	p9_fid_put(dfid);
	if (retval == -EOPNOTSUPP) {
		/* Try the one based on path */
		v9fid = v9fs_fid_clone(dentry);
		if (IS_ERR(v9fid))
			return PTR_ERR(v9fid);
		retval = p9_client_remove(v9fid);
	}
	if (!retval) {
		/*
		 * directories on unlink should have zero
		 * link count
		 */
		if (flags & AT_REMOVEDIR) {
			clear_nlink(inode);
			v9fs_dec_count(dir);
		} else
			v9fs_dec_count(inode);

		v9fs_invalidate_inode_attr(inode);
		v9fs_invalidate_inode_attr(dir);
	}
	return retval;
}

/**
 * v9fs_vfs_lookup - VFS lookup hook to "walk" to a new inode
 * @dir:  inode that is being walked from
 * @dentry: dentry that is being walked to?
 * @flags: lookup flags (unused)
 *
 */

struct dentry *v9fs_vfs_lookup(struct inode *dir, struct dentry *dentry,
			       unsigned int flags)
{
	struct v9fs_session_info *v9ses;
	struct p9_fid *dfid, *fid;
	struct inode *inode;
	const unsigned char *name;

	p9_debug(P9_DEBUG_VFS, "dir: %p dentry: (%s) %p flags: %x\n",
		 dir, dentry->d_name.name, dentry, flags);

	if (dentry->d_name.len > NAME_MAX)
		return ERR_PTR(-ENAMETOOLONG);

	v9ses = v9fs_inode2v9ses(dir);
	/* We can walk d_parent because we hold the dir->i_mutex */
	dfid = v9fs_parent_fid(dentry);
	if (IS_ERR(dfid))
		return ERR_CAST(dfid);

	/*
	 * Make sure we don't use a wrong inode due to parallel
	 * unlink. For cached mode create calls request for new
	 * inode. But with cache disabled, lookup should do this.
	 */
	name = dentry->d_name.name;
	fid = p9_client_walk(dfid, 1, &name, 1);
	p9_fid_put(dfid);
	if (fid == ERR_PTR(-ENOENT))
		inode = NULL;
	else if (IS_ERR(fid))
		inode = ERR_CAST(fid);
	else
		inode = v9fs_get_inode_from_fid(v9ses, fid, dir->i_sb, false);
	/*
	 * If we had a rename on the server and a parallel lookup
	 * for the new name, then make sure we instantiate with
	 * the new name. ie look up for a/b, while on server somebody
	 * moved b under k and client parallely did a lookup for
	 * k/b.
	 */
	if (!IS_ERR(inode)) {
		d_add(dentry, inode);
		if (!IS_ERR(fid))
			v9fs_fid_add(dentry, &fid);
	}

	return NULL;
}

/**
 * v9fs_vfs_unlink - VFS unlink hook to delete an inode
 * @i:  inode that is being unlinked
 * @d: dentry that is being unlinked
 *
 */

int v9fs_vfs_unlink(struct inode *i, struct dentry *d)
{
	return v9fs_remove(i, d, 0);
}

/**
 * v9fs_vfs_rmdir - VFS unlink hook to delete a directory
 * @i:  inode that is being unlinked
 * @d: dentry that is being unlinked
 *
 */

int v9fs_vfs_rmdir(struct inode *i, struct dentry *d)
{
	return v9fs_remove(i, d, AT_REMOVEDIR);
}
