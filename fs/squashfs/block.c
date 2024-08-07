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
 * block.c
 */

/*
 * This file implements the low-level routines to read and decompress
 * datablocks and metadata blocks.
 */

#include <linux/fs.h>
#include <linux/string.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs.h"
#include "decompressor.h"
#include "page_actor.h"

/*
 * Read the metadata block length, this is stored in the first two
 * bytes of the metadata block.
 */
static char *get_block_length(struct super_block *sb,
			u64 *cur_index, int *offset, int *length)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	char *buf;

	buf = squashfs_devread(msblk,
			 *cur_index * msblk->devblksize,
			 msblk->devblksize);
	if (buf == NULL)
		return NULL;

	if (msblk->devblksize - *offset == 1) {
		*length = (unsigned char) buf[*offset];
		free(buf);
		buf = squashfs_devread(msblk,
				 ++(*cur_index) * msblk->devblksize,
				 msblk->devblksize);
		if (buf == NULL)
			return NULL;
		*length |= (unsigned char) buf[0] << 8;
		*offset = 1;
	} else {
		*length = (unsigned char) buf[*offset] |
			(unsigned char) buf[*offset + 1] << 8;
		*offset += 2;

		if (*offset == msblk->devblksize) {
			free(buf);
			buf = squashfs_devread(msblk,
					 ++(*cur_index) * msblk->devblksize,
					 msblk->devblksize);
			if (buf == NULL)
				return NULL;
			*offset = 0;
		}
	}

	return buf;
}


/*
 * Read and decompress a metadata block or datablock.  Length is non-zero
 * if a datablock is being read (the size is stored elsewhere in the
 * filesystem), otherwise the length is obtained from the first two bytes of
 * the metadata block.  A bit in the length field indicates if the block
 * is stored uncompressed in the filesystem (usually because compression
 * generated a larger block - this does occasionally happen with compression
 * algorithms).
 */
int squashfs_read_data(struct super_block *sb, u64 index, int length,
		u64 *next_index, struct squashfs_page_actor *output)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	char **buf;
	int offset = index & ((1 << msblk->devblksize_log2) - 1);
	u64 cur_index = index >> msblk->devblksize_log2;
	int bytes, compressed, b = 0, k = 0, avail;

	buf = calloc(((output->length + msblk->devblksize - 1)
			>> msblk->devblksize_log2) + 1, sizeof(*buf));
	if (buf == NULL)
		return -ENOMEM;

	if (length) {
		/*
		 * Datablock.
		 */
		bytes = -offset;
		compressed = SQUASHFS_COMPRESSED_BLOCK(length);
		length = SQUASHFS_COMPRESSED_SIZE_BLOCK(length);
		if (next_index)
			*next_index = index + length;

		TRACE("Block @ 0x%llx, %scompressed size %d, src size %d\n",
			index, compressed ? "" : "un", length, output->length);

		if (length < 0 || length > output->length ||
				(index + length) > msblk->bytes_used) {
			goto read_failure;
		}

		for (b = 0; bytes < length; b++, cur_index++) {
			buf[b] = squashfs_devread(msblk,
					 cur_index * msblk->devblksize,
					 msblk->devblksize);
			if (buf[b] == NULL)
				goto block_release;

			bytes += msblk->devblksize;
		}

	} else {
		/*
		 * Metadata block.
		 */
		if ((index + 2) > msblk->bytes_used)
			goto read_failure;

		buf[0] = get_block_length(sb, &cur_index, &offset, &length);
		if (buf[0] == NULL)
			goto read_failure;
		b = 1;

		bytes = msblk->devblksize - offset;
		compressed = SQUASHFS_COMPRESSED(length);
		length = SQUASHFS_COMPRESSED_SIZE(length);
		if (next_index)
			*next_index = index + length + 2;

		TRACE("Block Meta @ 0x%llx, %scompressed size %d\n", index,
				compressed ? "" : "un", length);

		if (length < 0 || length > output->length ||
					(index + length) > msblk->bytes_used)
			goto block_release;

		for (; bytes < length; b++) {
			buf[b] = squashfs_devread(msblk,
					 ++cur_index * msblk->devblksize,
					 msblk->devblksize);
			if (buf[b] == NULL)
				goto block_release;
			bytes += msblk->devblksize;
		}
	}

	if (compressed) {
		if (!msblk->stream)
			goto read_failure;
		length = squashfs_decompress(msblk, buf, b, offset, length,
			output);
		if (length < 0)
			goto read_failure;
	} else {
		/*
		 * Block is uncompressed.
		 */
		int pg_offset = 0;
		void *data = squashfs_first_page(output);

		for (bytes = length; k < b; k++) {
			int in = min(bytes, msblk->devblksize - offset);
			bytes -= in;
			while (in) {
				if (pg_offset == PAGE_CACHE_SIZE) {
					data = squashfs_next_page(output);
					pg_offset = 0;
				}
				avail = min_t(int, in, PAGE_CACHE_SIZE -
						pg_offset);
				memcpy(data + pg_offset, buf[k] + offset,
						avail);
				in -= avail;
				pg_offset += avail;
				offset += avail;
			}
			offset = 0;
			kfree(buf[k]);
		}
		squashfs_finish_page(output);
	}

	kfree(buf);
	return length;

block_release:
	for (; k < b; k++)
		kfree(buf[k]);

read_failure:
	ERROR("squashfs_read_data failed to read block 0x%llx\n",
					(unsigned long long) index);
	kfree(buf);
	return -EIO;
}
