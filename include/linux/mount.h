/*
 *
 * Definitions for mount interface. This describes the in the kernel build
 * linkedlist with mounted filesystems.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 *
 */
#ifndef _LINUX_MOUNT_H
#define _LINUX_MOUNT_H

#include <linux/dcache.h>
#include <linux/fs.h>

struct vfsmount {
	struct dentry *mnt_root;	/* root of the mounted tree */
	struct dentry *mountpoint;	/* where it's mounted (barebox specific, no support */
	struct vfsmount *parent;	/* for bind mounts and the like) */
	struct super_block *mnt_sb;	/* pointer to superblock */
	int mnt_flags;
	int ref;
};

#endif /* _LINUX_MOUNT_H */
