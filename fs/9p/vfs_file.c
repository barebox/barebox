// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contians vfs file ops for 9P2000.
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pagemap.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/slab.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

/**
 * v9fs_file_open - open a file (or directory)
 * @inode: inode to be opened
 * @file: file being opened
 *
 */

int v9fs_file_open(struct inode *inode, struct file *file)
{
	int err;
	struct v9fs_session_info *v9ses;
	struct p9_fid *fid;
	int omode;

	p9_debug(P9_DEBUG_VFS, "inode: %p file: %p\n", inode, file);
	v9ses = v9fs_inode2v9ses(inode);
	omode = v9fs_open_to_dotl_flags(file->f_flags);
	fid = file->private_data;

	if (!fid) {
		fid = v9fs_fid_clone(file->f_dentry);
		if (IS_ERR(fid))
			return PTR_ERR(fid);

		err = p9_client_open(fid, omode);
		if (err < 0) {
			p9_fid_put(fid);
			return err;
		}

		file->private_data = fid;
	}

	v9fs_open_fid_add(inode, &fid);
	return 0;
}

const struct file_operations v9fs_file_operations_dotl = {
	.open = v9fs_file_open,
	.release = v9fs_dir_release,
};
