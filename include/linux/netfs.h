/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Network filesystem support services.
 *
 * Copyright (C) 2021 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * See:
 *
 *	Documentation/filesystems/netfs_library.rst
 *
 * for a description of the network filesystem interface declared here.
 */

#ifndef _LINUX_NETFS_H
#define _LINUX_NETFS_H

#include <linux/fs.h>
#include <linux/container_of.h>
#include <clock.h>

/*
 * Per-inode context.  This wraps the VFS inode.
 */
struct netfs_inode {
	struct inode		inode;		/* The VFS inode */
	u64			time;		/* Time the inode was allocated */
};

enum netfs_io_origin {
	NETFS_READPAGE,			/* This read is a synchronous read */
	NETFS_WRITETHROUGH,		/* This write was made by netfs_perform_write() */
} __mode(byte);

/*
 * Descriptor for an I/O helper request.  This is used to make multiple I/O
 * operations to a variety of data stores and then stitch the result together.
 */
struct netfs_io_request {
	struct inode		*inode;		/* The file being accessed */
	void			*netfs_priv;	/* Private data for the netfs */
	enum netfs_io_origin	origin;		/* Origin of the request */
};

/**
 * netfs_inode - Get the netfs inode context from the inode
 * @inode: The inode to query
 *
 * Get the netfs lib inode context from the network filesystem's inode.  The
 * context struct is expected to directly follow on from the VFS inode struct.
 */
static inline struct netfs_inode *netfs_inode(struct inode *inode)
{
	return container_of(inode, struct netfs_inode, inode);
}

/**
 * netfs_inode_init - Initialise a netfslib inode context
 * @ctx: The netfs inode to initialise
 *
 * Initialise the netfs library context struct.  This is expected to follow on
 * directly from the VFS inode struct.
 */
static inline void netfs_inode_init(struct netfs_inode *ctx)
{
	ctx->time = get_time_ns();
}

#define NETFS_INODE_VALID_TIME		(2 * NSEC_PER_SEC)

static inline void netfs_invalidate_inode_attr(struct netfs_inode *ctx)
{
	/* ensure that we always detect the inode to be stale */
	ctx->time = -NETFS_INODE_VALID_TIME;
}

extern const struct dentry_operations netfs_dentry_operations_timed;

#endif /* _LINUX_NETFS_H */
