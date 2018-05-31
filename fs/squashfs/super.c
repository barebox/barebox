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
 * super.c
 */

/*
 * This file implements code to read the superblock, read and initialise
 * in-memory structures at mount time, and all the VFS glue code to register
 * the filesystem.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/magic.h>
#include <linux/bitops.h>

#include "page_actor.h"
#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs_fs_i.h"
#include "squashfs.h"
#include "decompressor.h"

static const struct squashfs_decompressor *supported_squashfs_filesystem(short
	major, short minor, short id)
{
	const struct squashfs_decompressor *decompressor;

	if (major < SQUASHFS_MAJOR) {
		ERROR("Major/Minor mismatch, older Squashfs %d.%d "
			"filesystems are unsupported\n", major, minor);
		return NULL;
	} else if (major > SQUASHFS_MAJOR || minor > SQUASHFS_MINOR) {
		ERROR("Major/Minor mismatch, trying to mount newer "
			"%d.%d filesystem\n", major, minor);
		ERROR("Please update your kernel\n");
		return NULL;
	}

	decompressor = squashfs_lookup_decompressor(id);
	if (!decompressor->supported) {
		ERROR("Filesystem uses \"%s\" compression. This is not "
			"supported\n", decompressor->name);
		return NULL;
	}

	return decompressor;
}

void squashfs_put_super(struct super_block *sb)
{
	if (sb->s_fs_info) {
		struct squashfs_sb_info *sbi = sb->s_fs_info;
		squashfs_cache_delete(sbi->block_cache);
		squashfs_cache_delete(sbi->fragment_cache);
		squashfs_cache_delete(sbi->read_page);
		squashfs_decompressor_destroy(sbi);
		kfree(sbi->id_table);
		kfree(sbi->fragment_index);
		kfree(sbi->meta_index);
		kfree(sbi->inode_lookup_table);
		kfree(sbi->xattr_id_table);
		kfree(sb->s_fs_info);
		sb->s_fs_info = NULL;
	}

	if (sb->s_root) {
		kfree(squashfs_i(sb->s_root->d_inode));
		kfree(sb->s_root);
		sb->s_root = NULL;
	}
}

static int squashfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct squashfs_sb_info *msblk;
	struct fs_device_d *fsdev = (struct fs_device_d *)data;
	struct squashfs_super_block *sblk = NULL;
	struct inode *root;
	long long root_inode;
	unsigned short flags;
	unsigned int fragments;
	u64 lookup_table_start, next_table;
	int err;

	TRACE("Entered squashfs_fill_superblock\n");

	sb->s_fs_info = kzalloc(sizeof(*msblk), GFP_KERNEL);
	if (sb->s_fs_info == NULL) {
		ERROR("Failed to allocate squashfs_sb_info\n");
		return -ENOMEM;
	}
	msblk = sb->s_fs_info;
	msblk->cdev = fsdev->cdev;
	msblk->dev = &fsdev->dev;

	msblk->devblksize = 1024;
	msblk->devblksize_log2 = ffz(~msblk->devblksize);

	mutex_init(&msblk->meta_index_mutex);
	/*
	 * msblk->bytes_used is checked in squashfs_read_table to ensure reads
	 * are not beyond filesystem end.  But as we're using
	 * squashfs_read_table here to read the superblock (including the value
	 * of bytes_used) we need to set it to an initial sensible dummy value
	 */
	msblk->bytes_used = sizeof(*sblk);
	sblk = squashfs_read_table(sb, SQUASHFS_START, sizeof(*sblk));

	if (IS_ERR(sblk)) {
		ERROR("unable to read squashfs_super_block\n");
		err = PTR_ERR(sblk);
		sblk = NULL;
		goto failed_mount;
	}

	err = -EINVAL;

	/* Check it is a SQUASHFS superblock */
	sb->s_magic = le32_to_cpu(sblk->s_magic);
	if (sb->s_magic != SQUASHFS_MAGIC) {
		if (!silent)
			ERROR("Can't find a SQUASHFS superblock on %pg\n",
						sb->s_bdev);
		goto failed_mount;
	}

	/* Check the MAJOR & MINOR versions and lookup compression type */
	msblk->decompressor = supported_squashfs_filesystem(
			le16_to_cpu(sblk->s_major),
			le16_to_cpu(sblk->s_minor),
			le16_to_cpu(sblk->compression));
	if (msblk->decompressor == NULL)
		goto failed_mount;

	/* Check the filesystem does not extend beyond the end of the
	   block device */
	msblk->bytes_used = le64_to_cpu(sblk->bytes_used);
	if (msblk->bytes_used < 0 || msblk->bytes_used >
			msblk->cdev->size)
		goto failed_mount;

	/* Check block size for sanity */
	msblk->block_size = le32_to_cpu(sblk->block_size);
	if (msblk->block_size > SQUASHFS_FILE_MAX_SIZE)
		goto failed_mount;

	/*
	 * Check the system page size is not larger than the filesystem
	 * block size (by default 128K).  This is currently not supported.
	 */
	if (PAGE_CACHE_SIZE > msblk->block_size) {
		ERROR("Page size > filesystem block size (%d).  This is "
			"currently not supported!\n", msblk->block_size);
		goto failed_mount;
	}

	/* Check block log for sanity */
	msblk->block_log = le16_to_cpu(sblk->block_log);
	if (msblk->block_log > SQUASHFS_FILE_MAX_LOG)
		goto failed_mount;

	/* Check that block_size and block_log match */
	if (msblk->block_size != (1 << msblk->block_log))
		goto failed_mount;

	/* Check the root inode for sanity */
	root_inode = le64_to_cpu(sblk->root_inode);
	if (SQUASHFS_INODE_OFFSET(root_inode) > SQUASHFS_METADATA_SIZE)
		goto failed_mount;

	msblk->inode_table = le64_to_cpu(sblk->inode_table_start);
	msblk->directory_table = le64_to_cpu(sblk->directory_table_start);
	msblk->inodes = le32_to_cpu(sblk->inodes);
	flags = le16_to_cpu(sblk->flags);

	TRACE("Found valid superblock on %pg\n", sb->s_bdev);
	TRACE("Inodes are %scompressed\n", SQUASHFS_UNCOMPRESSED_INODES(flags)
				? "un" : "");
	TRACE("Data is %scompressed\n", SQUASHFS_UNCOMPRESSED_DATA(flags)
				? "un" : "");
	TRACE("Filesystem size %lld bytes\n", msblk->bytes_used);
	TRACE("Block size %d\n", msblk->block_size);
	TRACE("Number of inodes %d\n", msblk->inodes);
	TRACE("Number of fragments %d\n", le32_to_cpu(sblk->fragments));
	TRACE("Number of ids %d\n", le16_to_cpu(sblk->no_ids));
	TRACE("sblk->inode_table_start %llx\n", msblk->inode_table);
	TRACE("sblk->directory_table_start %llx\n", msblk->directory_table);
	TRACE("sblk->fragment_table_start %llx\n",
		(u64) le64_to_cpu(sblk->fragment_table_start));
	TRACE("sblk->id_table_start %llx\n",
		(u64) le64_to_cpu(sblk->id_table_start));

	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_flags |= MS_RDONLY;

	err = -ENOMEM;

	msblk->block_cache = squashfs_cache_init("metadata",
			SQUASHFS_CACHED_BLKS, SQUASHFS_METADATA_SIZE);
	if (msblk->block_cache == NULL)
		goto failed_mount;

	/* Allocate read_page block */
	msblk->read_page = squashfs_cache_init("data",
		squashfs_max_decompressors(), msblk->block_size);
	if (msblk->read_page == NULL) {
		ERROR("Failed to allocate read_page block\n");
		goto failed_mount;
	}

	msblk->stream = squashfs_decompressor_setup(sb, flags);
	if (IS_ERR(msblk->stream)) {
		err = PTR_ERR(msblk->stream);
		msblk->stream = NULL;
		goto failed_mount;
	}

	next_table = msblk->bytes_used;

	/* Allocate and read id index table */
	msblk->id_table = squashfs_read_id_index_table(sb,
		le64_to_cpu(sblk->id_table_start), next_table,
		le16_to_cpu(sblk->no_ids));
	if (IS_ERR(msblk->id_table)) {
		ERROR("unable to read id index table\n");
		err = PTR_ERR(msblk->id_table);
		msblk->id_table = NULL;
		goto failed_mount;
	}
	next_table = le64_to_cpu(msblk->id_table[0]);

	/* Handle inode lookup table */
	lookup_table_start = le64_to_cpu(sblk->lookup_table_start);
	if (lookup_table_start == SQUASHFS_INVALID_BLK)
		goto handle_fragments;

handle_fragments:
	fragments = le32_to_cpu(sblk->fragments);
	if (fragments == 0)
		goto check_directory_table;
	msblk->fragment_cache = squashfs_cache_init("fragment",
		SQUASHFS_CACHED_FRAGMENTS, msblk->block_size);
	if (msblk->fragment_cache == NULL) {
		err = -ENOMEM;
		goto failed_mount;
	}

	/* Allocate and read fragment index table */
	msblk->fragment_index = squashfs_read_fragment_index_table(sb,
		le64_to_cpu(sblk->fragment_table_start), next_table, fragments);
	if (IS_ERR(msblk->fragment_index)) {
		ERROR("unable to read fragment index table\n");
		err = PTR_ERR(msblk->fragment_index);
		msblk->fragment_index = NULL;
		goto failed_mount;
	}
	next_table = le64_to_cpu(msblk->fragment_index[0]);

check_directory_table:
	/* Sanity check directory_table */
	if (msblk->directory_table > next_table) {
		err = -EINVAL;
		goto failed_mount;
	}

	/* Sanity check inode_table */
	if (msblk->inode_table >= msblk->directory_table) {
		err = -EINVAL;
		goto failed_mount;
	}

	/* allocate root */
	root = squashfs_iget(sb, root_inode, 1);
	if (!root) {
		err = -ENOMEM;
		goto failed_mount;
	}

	sb->s_root = d_make_root(root);
	if (sb->s_root == NULL) {
		ERROR("Root inode create failed\n");
		err = -ENOMEM;
		goto failed_mount;
	}

	kfree(sblk);

	return 0;

failed_mount:
	squashfs_cache_delete(msblk->block_cache);
	squashfs_cache_delete(msblk->fragment_cache);
	squashfs_cache_delete(msblk->read_page);
	squashfs_decompressor_destroy(msblk);
	kfree(msblk->inode_lookup_table);
	kfree(msblk->fragment_index);
	kfree(msblk->id_table);
	kfree(msblk->xattr_id_table);
	kfree(sb->s_fs_info);
	sb->s_fs_info = NULL;
	kfree(sblk);
	return err;
}


int squashfs_mount(struct fs_device_d *fsdev, int silent)
{
	struct super_block *sb = &fsdev->sb;

	dev_dbg(&fsdev->dev, "squashfs_mount\n");

	if (squashfs_fill_super(sb, fsdev, silent))
		return -EINVAL;

	return 0;
}
