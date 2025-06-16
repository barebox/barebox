/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * V9FS VFS extensions.
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */
#ifndef FS_9P_V9FS_VFS_H
#define FS_9P_V9FS_VFS_H

#include <fs.h>
#include <linux/netfs.h>

/* plan9 semantics are that created files are implicitly opened.
 * But linux semantics are that you call create, then open.
 * the plan9 approach is superior as it provides an atomic
 * open.
 * we track the create fid here. When the file is opened, if fidopen is
 * non-zero, we use the fid and can skip some steps.
 * there may be a better way to do this, but I don't know it.
 * one BAD way is to clunk the fid on create, then open it again:
 * you lose the atomicity of file open
 */

extern struct fs_driver v9fs_driver;
struct v9fs_session_info;
struct p9_qid;
struct p9_fid;
struct p9_stat_dotl;
struct p9_wstat;

extern const struct file_operations v9fs_file_operations_dotl;
extern const struct file_operations v9fs_dir_operations_dotl;

struct super_block;
struct inode *v9fs_alloc_inode(struct super_block *sb);
void v9fs_free_inode(struct inode *inode);
void v9fs_set_netfs_context(struct inode *inode);
int v9fs_mount(struct device *dev);
void v9fs_umount(struct device *dev);
int v9fs_init_inode(struct v9fs_session_info *v9ses,
		    struct inode *inode, struct p9_qid *qid, umode_t mode, dev_t rdev);
#if (BITS_PER_LONG == 32)
#define QID2INO(q) ((ino_t) (((q)->path+2) ^ (((q)->path) >> 32)))
#else
#define QID2INO(q) ((ino_t) ((q)->path+2))
#endif

void v9fs_stat2inode_dotl(struct p9_stat_dotl *stat, struct inode *inode);
int v9fs_dir_release(struct inode *inode, struct file *file);
int v9fs_file_open(struct inode *inode, struct file *file);

int v9fs_file_fsync_dotl(struct device *dev, struct file *filp);
static inline void v9fs_invalidate_inode_attr(struct inode *inode)
{
	netfs_invalidate_inode_attr(netfs_inode(inode));
}

int v9fs_open_to_dotl_flags(int flags);

static inline void v9fs_i_size_write(struct inode *inode, loff_t i_size)
{
	/*
	 * 32-bit need the lock, concurrent updates could break the
	 * sequences and make i_size_read() loop forever.
	 * 64-bit updates are atomic and can skip the locking.
	 */
	if (sizeof(i_size) > sizeof(long))
		spin_lock(&inode->i_lock);
	i_size_write(inode, i_size);
	if (sizeof(i_size) > sizeof(long))
		spin_unlock(&inode->i_lock);
}
#endif
