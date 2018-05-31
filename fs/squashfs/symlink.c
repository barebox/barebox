/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * symlink.c
 */

/*
 * This file implements code to handle symbolic links.
 *
 * The data contents of symbolic links are stored inside the symbolic
 * link inode within the inode table.  This allows the normally small symbolic
 * link to be compressed as part of the inode table, achieving much greater
 * compression than if the symbolic link was compressed individually.
 */

#include <malloc.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/pagemap.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs_fs_i.h"
#include "squashfs.h"

static const char *squashfs_get_link(struct dentry *dentry, struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	int index = 0;
	u64 block = squashfs_i(inode)->start;
	int offset = squashfs_i(inode)->offset;
	int length = min_t(int, i_size_read(inode) - index, PAGE_SIZE);
	int bytes;
	unsigned char *symlink;

	TRACE("Entered squashfs_symlink_readpage, start block "
			"%llx, offset %x\n", block, offset);

	symlink = malloc(length + 1);
	if (!symlink)
		return NULL;

	symlink[length] = 0;

	bytes = squashfs_read_metadata(sb, symlink, &block, &offset, length);
	if (bytes < 0) {
		ERROR("Unable to read symlink [%llx:%x]\n",
			squashfs_i(inode)->start,
			squashfs_i(inode)->offset);
		goto error_out;
	}

	inode->i_link = symlink;

	return inode->i_link;

error_out:
	free(symlink);

	return NULL;
}

const struct inode_operations squashfs_symlink_inode_ops = {
	.get_link = squashfs_get_link,
};
