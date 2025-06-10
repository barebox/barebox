// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contains vfs directory ops for the 9P2000 protocol.
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uio.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

/**
 * struct p9_rdir - readdir accounting
 * @head: start offset of current dirread buffer
 * @tail: end offset of current dirread buffer
 * @buf: dirread buffer
 *
 * private structure for keeping track of readdir
 * allocated on demand
 */

struct p9_rdir {
	int head;
	int tail;
	uint8_t buf[];
};

/**
 * dt_type - return file type
 * @mistat: mistat structure
 *
 */

static inline int dt_type(struct p9_wstat *mistat)
{
	unsigned long perm = mistat->mode;
	int rettype = DT_REG;

	if (perm & P9_DMDIR)
		rettype = DT_DIR;
	if (perm & P9_DMSYMLINK)
		rettype = DT_LNK;

	return rettype;
}

/**
 * v9fs_alloc_rdir_buf - Allocate buffer used for read and readdir
 * @filp: opened file structure
 * @buflen: Length in bytes of buffer to allocate
 *
 */

static struct p9_rdir *v9fs_alloc_rdir_buf(struct file *filp, int buflen)
{
	struct p9_fid *fid = filp->private_data;

	if (!fid->rdir)
		fid->rdir = kzalloc(sizeof(struct p9_rdir) + buflen, GFP_KERNEL);
	return fid->rdir;
}

/**
 * v9fs_dir_readdir_dotl - iterate through a directory
 * @file: opened file structure
 * @ctx: actor we feed the entries to
 *
 */
static int v9fs_dir_readdir_dotl(struct file *file, struct dir_context *ctx)
{
	int err = 0;
	struct p9_fid *fid;
	int buflen;
	struct p9_rdir *rdir;
	struct p9_dirent curdirent;

	p9_debug(P9_DEBUG_VFS, "name %s\n", file->path);
	fid = file->private_data;

	buflen = fid->clnt->msize - P9_READDIRHDRSZ;

	rdir = v9fs_alloc_rdir_buf(file, buflen);
	if (!rdir)
		return -ENOMEM;

	while (1) {
		if (rdir->tail == rdir->head) {
			err = p9_client_readdir(fid, rdir->buf, buflen,
						ctx->pos);
			if (err <= 0)
				return err;

			rdir->head = 0;
			rdir->tail = err;
		}

		while (rdir->head < rdir->tail) {
			err = p9dirent_read(fid->clnt, rdir->buf + rdir->head,
					    rdir->tail - rdir->head,
					    &curdirent);
			if (err < 0) {
				p9_debug(P9_DEBUG_VFS, "returned %d\n", err);
				return -EIO;
			}

			dir_emit(ctx, curdirent.d_name,
				 strlen(curdirent.d_name),
				 QID2INO(&curdirent.qid),
				 curdirent.d_type);

			ctx->pos = curdirent.d_off;
			rdir->head += err;
		}
	}
}


/**
 * v9fs_dir_release - close a directory or a file
 * @inode: inode of the directory or file
 * @filp: file pointer to a directory or file
 *
 */

int v9fs_dir_release(struct inode *inode, struct file *filp)
{
	struct p9_fid *fid;
	int retval = 0, put_err;

	fid = filp->private_data;
	p9_debug(P9_DEBUG_VFS, "inode: %p filp: %p fid: %d\n",
		 inode, filp, fid ? fid->fid : -1);

	if (fid) {
		spin_lock(&inode->i_lock);
		hlist_del(&fid->ilist);
		spin_unlock(&inode->i_lock);
		put_err = p9_fid_put(fid);
		retval = retval < 0 ? retval : put_err;
	}

	return retval;
}

int v9fs_file_fsync_dotl(struct device *dev, struct file *filp)
{
        struct p9_fid *fid;

        p9_debug(P9_DEBUG_VFS, "filp %p datasync\n", filp);

        fid = filp->private_data;

        return p9_client_fsync(fid, true);
}

const struct file_operations v9fs_dir_operations_dotl = {
	.iterate = v9fs_dir_readdir_dotl,
	.open = v9fs_file_open,
	.release = v9fs_dir_release,
};
