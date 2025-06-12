// SPDX-License-Identifier: GPL-2.0-only

#include <linux/dcache.h>
#include <clock.h>
#include <linux/netfs.h>


static int netfs_lookup_revalidate(struct dentry *dentry, unsigned int flags)
{
	struct netfs_inode *node;

	if (!dentry->d_inode)
		return 0;

	node = netfs_inode(dentry->d_inode);

	if (is_timeout(node->time, NETFS_INODE_VALID_TIME))
		return 0;

	return 1;
}

const struct dentry_operations netfs_dentry_operations_timed = {
	.d_revalidate = netfs_lookup_revalidate,
};
