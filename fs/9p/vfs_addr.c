// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contians vfs address (mmap) ops for 9P2000.
 *
 *  Copyright (C) 2005 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include <linux/uio.h>
#include <linux/netfs.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

/**
 * v9fs_init_request - Initialise a request
 * @rreq: The read request
 * @file: The file being read from
 */
static int v9fs_init_request(struct netfs_io_request *rreq, struct file *file)
{
	struct p9_fid *fid;
	bool writing = rreq->origin == NETFS_WRITETHROUGH;

	if (file) {
		fid = file->private_data;
		if (!fid)
			goto no_fid;
		p9_fid_get(fid);
	} else {
		fid = v9fs_fid_find_inode(rreq->inode, writing, INVALID_UID, true);
		if (!fid)
			goto no_fid;
	}

	rreq->netfs_priv = fid;
	return 0;

no_fid:
	WARN_ONCE(1, "folio expected an open fid inode->i_ino=%lx\n",
		  rreq->inode->i_ino);
	return -EINVAL;
}

/**
 * v9fs_free_request - Cleanup request initialized by v9fs_init_rreq
 * @rreq: The I/O request to clean up
 */
static void v9fs_free_request(struct netfs_io_request *rreq)
{
	struct p9_fid *fid = rreq->netfs_priv;

	p9_fid_put(fid);
}

int v9fs_read(struct device *dev, struct file *f, void *buf,
	      size_t insize)
{
	struct netfs_io_request rreq = {
		.origin = NETFS_READPAGE,
		.inode = f->f_inode,
	};
	struct kvec kv = {.iov_base = buf, .iov_len = insize};
	struct iov_iter to;
	int len, err;

	err = v9fs_init_request(&rreq, f);
	if (err)
		return err;

	iov_iter_kvec(&to, ITER_DEST, &kv, 1, insize);

	len = p9_client_read(rreq.netfs_priv, f->f_pos, &to, &err);

	v9fs_free_request(&rreq);
	return len ?: err;
}

int v9fs_write(struct device *dev, struct file *f, const void *buf,
	       size_t insize)
{
	struct netfs_io_request rreq = {
		.origin = NETFS_WRITETHROUGH,
		.inode = f->f_inode,
	};
	struct kvec kv = {.iov_base = (void *)buf, .iov_len = insize};
	struct iov_iter from;
	int len, err;

	err = v9fs_init_request(&rreq, f);
	if (err)
		return err;

	iov_iter_kvec(&from, ITER_SOURCE, &kv, 1, insize);

	len = p9_client_write(rreq.netfs_priv, f->f_pos, &from, &err);

	v9fs_free_request(&rreq);
	return len ?: err;
}

