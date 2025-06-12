// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/mount.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/magic.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>
#include <net/9p/transport.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

static const struct super_operations v9fs_super_ops_dotl;

static void v9fs_devinfo(struct device *dev);

/**
 * v9fs_set_super - set the superblock
 * @s: super block
 * @data: file system specific data
 *
 */

static void v9fs_set_super(struct super_block *s, void *data)
{
	s->s_fs_info = data;
}

/**
 * v9fs_fill_super - populate superblock with info
 * @sb: superblock
 * @v9ses: session information
 *
 */

static int
v9fs_fill_super(struct super_block *sb, struct v9fs_session_info *v9ses)
{
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize_bits = fls(v9ses->maxdata - 1);
	sb->s_blocksize = 1 << sb->s_blocksize_bits;
	sb->s_magic = V9FS_MAGIC;
	sb->s_op = &v9fs_super_ops_dotl;

	return 0;
}

static void v9fs_set_rootarg(struct v9fs_session_info *v9ses,
			     struct fs_device *fsdev)
{
	const char *tag, *trans, *path;
	char *str;

	tag = v9ses->clnt->trans_tag;
	trans = v9ses->clnt->trans_mod->name;
	path = v9ses->aname;

	str = basprintf("root=%s rootfstype=9p rootflags=trans=%s,msize=%d,"
			"cache=loose,uname=%s,dfltuid=0,dfltgid=0,aname=%s",
			tag, trans, v9ses->clnt->msize, tag, path);

	fsdev_set_linux_rootarg(fsdev, str);

	free(str);
}

/**
 * v9fs_mount - mount a superblock
 * @flags: mount flags
 * @dev_name: device name that was mounted
 * @data: mount options
 *
 */

int v9fs_mount(struct device *dev)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	void *data = fsdev->options;
	struct super_block *sb = &fsdev->sb;
	struct inode *inode = NULL;
	struct dentry *root = NULL;
	struct v9fs_session_info *v9ses = NULL;
	struct p9_fid *fid;
	int retval = 0;

	p9_debug(P9_DEBUG_VFS, "\n");

	v9ses = kzalloc(sizeof(struct v9fs_session_info), GFP_KERNEL);
	if (!v9ses)
		return -ENOMEM;

	fid = v9fs_session_init(v9ses, fsdev->backingstore, data);
	if (IS_ERR(fid)) {
		retval = PTR_ERR(fid);
		goto free_session;
	}

	v9fs_set_super(sb, v9ses);

	retval = v9fs_fill_super(sb, v9ses);
	if (retval)
		goto release_sb;

	sb->s_d_op = &netfs_dentry_operations_timed;

	inode = v9fs_get_inode_from_fid(v9ses, fid, sb, true);
	if (IS_ERR(inode)) {
		retval = PTR_ERR(inode);
		goto release_sb;
	}

	root = d_make_root(inode);
	if (!root) {
		retval = -ENOMEM;
		goto release_sb;
	}
	sb->s_root = root;
	v9fs_fid_add(root, &fid);

	p9_debug(P9_DEBUG_VFS, " simple set mount, return 0\n");

	dev->priv = sb;
	devinfo_add(dev, v9fs_devinfo);

	v9fs_set_rootarg(v9ses, fsdev);

	return 0;
free_session:
	kfree(v9ses);
	return retval;

release_sb:
	/*
	 * we will do the session_close and root dentry release
	 * in the below call. But we need to clunk fid, because we haven't
	 * attached the fid to dentry so it won't get clunked
	 * automatically.
	 */
	p9_fid_put(fid);
	return retval;
}

/**
 * v9fs_kill_super - Kill Superblock
 * @s: superblock
 *
 */

static void v9fs_kill_super(struct super_block *s)
{
	struct v9fs_session_info *v9ses = s->s_fs_info;

	p9_debug(P9_DEBUG_VFS, " %p\n", s);

	v9fs_session_cancel(v9ses);
	v9fs_session_close(v9ses);
	kfree(v9ses);
	s->s_fs_info = NULL;
	p9_debug(P9_DEBUG_VFS, "exiting kill_super\n");
}

static void
v9fs_umount_begin(struct super_block *sb)
{
	struct v9fs_session_info *v9ses;

	v9ses = sb->s_fs_info;
	v9fs_session_begin_cancel(v9ses);
}

void v9fs_umount(struct device *dev)
{
	struct super_block *sb = dev->priv;

	devinfo_del(dev, v9fs_devinfo);
	v9fs_umount_begin(sb);
	v9fs_kill_super(sb);
}

static void v9fs_statfs(struct device *dev, struct dentry *dentry)
{
	struct v9fs_session_info *v9ses;
	struct p9_fid *fid;
	struct p9_rstatfs rs;
	int res;

	fid = v9fs_fid_lookup(dentry);
	if (IS_ERR(fid)) {
		res = PTR_ERR(fid);
		goto done;
	}

	v9ses = v9fs_dentry2v9ses(dentry);
	res = p9_client_statfs(fid, &rs);
	if (res == 0) {
		printf("File system stats:\n");
		printf("  type = %u\n", rs.type);
		printf("  bsize = %u\n", rs.bsize);
		printf("  blocks = %llu\n", rs.blocks);
		printf("  bfree = %llu\n", rs.bfree);
		printf("  bavail = %llu\n", rs.bavail);
		printf("  files = %llu\n", rs.files);
		printf("  ffree = %llu\n", rs.ffree);
		printf("  fsid = %llu\n", rs.fsid);
		printf("  namelen = %u\n", rs.namelen);
	}
	if (res != -ENOSYS)
		goto done;
done:
	p9_fid_put(fid);
	if (res)
		dev_warn(dev, "statfs failed; %pe\n", ERR_PTR(res));
}

static void v9fs_devinfo(struct device *dev)
{
	struct super_block *sb = dev->priv;
	struct dentry *root = sb->s_root;

	v9fs_statfs(dev, root);
}

static const struct super_operations v9fs_super_ops_dotl = {
	.alloc_inode = v9fs_alloc_inode,
	.destroy_inode = v9fs_free_inode,
};

MODULE_ALIAS_FS("9p");
