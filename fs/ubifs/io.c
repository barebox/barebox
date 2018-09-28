/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 * Copyright (C) 2006, 2007 University of Szeged, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Artem Bityutskiy (Битюцкий Артём)
 *          Adrian Hunter
 *          Zoltan Sogor
 */

/*
 * This file implements UBIFS I/O subsystem which provides various I/O-related
 * helper functions (reading/writing/checking/validating nodes) and implements
 * write-buffering support. Write buffers help to save space which otherwise
 * would have been wasted for padding to the nearest minimal I/O unit boundary.
 * Instead, data first goes to the write-buffer and is flushed when the
 * buffer is full or when it is not used for some time (by timer). This is
 * similar to the mechanism is used by JFFS2.
 *
 * UBIFS distinguishes between minimum write size (@c->min_io_size) and maximum
 * write size (@c->max_write_size). The latter is the maximum amount of bytes
 * the underlying flash is able to program at a time, and writing in
 * @c->max_write_size units should presumably be faster. Obviously,
 * @c->min_io_size <= @c->max_write_size. Write-buffers are of
 * @c->max_write_size bytes in size for maximum performance. However, when a
 * write-buffer is flushed, only the portion of it (aligned to @c->min_io_size
 * boundary) which contains data is written, not the whole write-buffer,
 * because this is more space-efficient.
 *
 * This optimization adds few complications to the code. Indeed, on the one
 * hand, we want to write in optimal @c->max_write_size bytes chunks, which
 * also means aligning writes at the @c->max_write_size bytes offsets. On the
 * other hand, we do not want to waste space when synchronizing the write
 * buffer, so during synchronization we writes in smaller chunks. And this makes
 * the next write offset to be not aligned to @c->max_write_size bytes. So the
 * have to make sure that the write-buffer offset (@wbuf->offs) becomes aligned
 * to @c->max_write_size bytes again. We do this by temporarily shrinking
 * write-buffer size (@wbuf->size).
 *
 * Write-buffers are defined by 'struct ubifs_wbuf' objects and protected by
 * mutexes defined inside these objects. Since sometimes upper-level code
 * has to lock the write-buffer (e.g. journal space reservation code), many
 * functions related to write-buffers have "nolock" suffix which means that the
 * caller has to lock the write-buffer before calling this function.
 *
 * UBIFS stores nodes at 64 bit-aligned addresses. If the node length is not
 * aligned, UBIFS starts the next node from the aligned address, and the padded
 * bytes may contain any rubbish. In other words, UBIFS does not put padding
 * bytes in those small gaps. Common headers of nodes store real node lengths,
 * not aligned lengths. Indexing nodes also store real lengths in branches.
 *
 * UBIFS uses padding when it pads to the next min. I/O unit. In this case it
 * uses padding nodes or padding bytes, if the padding node does not fit.
 *
 * All UBIFS nodes are protected by CRC checksums and UBIFS checks CRC when
 * they are read from the flash media.
 */

#include <linux/err.h>
#include "ubifs.h"

/**
 * ubifs_ro_mode - switch UBIFS to read read-only mode.
 * @c: UBIFS file-system description object
 * @err: error code which is the reason of switching to R/O mode
 */
void ubifs_ro_mode(struct ubifs_info *c, int err)
{
	if (!c->ro_error) {
		c->ro_error = 1;
		c->no_chk_data_crc = 0;
		c->vfs_sb->s_flags |= SB_RDONLY;
		ubifs_warn(c, "switched to read-only mode, error %d", err);
		dump_stack();
	}
}

/*
 * Below are simple wrappers over UBI I/O functions which include some
 * additional checks and UBIFS debugging stuff. See corresponding UBI function
 * for more information.
 */

int ubifs_leb_read(const struct ubifs_info *c, int lnum, void *buf, int offs,
		   int len, int even_ebadmsg)
{
	int err;

	err = ubi_read(c->ubi, lnum, buf, offs, len);
	/*
	 * In case of %-EBADMSG print the error message only if the
	 * @even_ebadmsg is true.
	 */
	if (err && (err != -EBADMSG || even_ebadmsg)) {
		ubifs_err(c, "reading %d bytes from LEB %d:%d failed, error %d",
			  len, lnum, offs, err);
		dump_stack();
	}
	return err;
}

/*
 * removed in barebox
int ubifs_leb_write(struct ubifs_info *c, int lnum, const void *buf, int offs,
		    int len)
 */

/*
 * removed in barebox
int ubifs_leb_change(struct ubifs_info *c, int lnum, const void *buf, int len)
 */

/*
 * removed in barebox
int ubifs_leb_unmap(struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
int ubifs_leb_map(struct ubifs_info *c, int lnum)
 */

int ubifs_is_mapped(const struct ubifs_info *c, int lnum)
{
	int err;

	err = ubi_is_mapped(c->ubi, lnum);
	if (err < 0) {
		ubifs_err(c, "ubi_is_mapped failed for LEB %d, error %d",
			  lnum, err);
		dump_stack();
	}
	return err;
}

/**
 * ubifs_check_node - check node.
 * @c: UBIFS file-system description object
 * @buf: node to check
 * @lnum: logical eraseblock number
 * @offs: offset within the logical eraseblock
 * @quiet: print no messages
 * @must_chk_crc: indicates whether to always check the CRC
 *
 * This function checks node magic number and CRC checksum. This function also
 * validates node length to prevent UBIFS from becoming crazy when an attacker
 * feeds it a file-system image with incorrect nodes. For example, too large
 * node length in the common header could cause UBIFS to read memory outside of
 * allocated buffer when checking the CRC checksum.
 *
 * This function may skip data nodes CRC checking if @c->no_chk_data_crc is
 * true, which is controlled by corresponding UBIFS mount option. However, if
 * @must_chk_crc is true, then @c->no_chk_data_crc is ignored and CRC is
 * checked. Similarly, if @c->mounting or @c->remounting_rw is true (we are
 * mounting or re-mounting to R/W mode), @c->no_chk_data_crc is ignored and CRC
 * is checked. This is because during mounting or re-mounting from R/O mode to
 * R/W mode we may read journal nodes (when replying the journal or doing the
 * recovery) and the journal nodes may potentially be corrupted, so checking is
 * required.
 *
 * This function returns zero in case of success and %-EUCLEAN in case of bad
 * CRC or magic.
 */
int ubifs_check_node(const struct ubifs_info *c, const void *buf, int lnum,
		     int offs, int quiet, int must_chk_crc)
{
	int err = -EINVAL, type, node_len;
	uint32_t crc, node_crc, magic;
	const struct ubifs_ch *ch = buf;

	ubifs_assert(c, lnum >= 0 && lnum < c->leb_cnt && offs >= 0);
	ubifs_assert(c, !(offs & 7) && offs < c->leb_size);

	magic = le32_to_cpu(ch->magic);
	if (magic != UBIFS_NODE_MAGIC) {
		if (!quiet)
			ubifs_err(c, "bad magic %#08x, expected %#08x",
				  magic, UBIFS_NODE_MAGIC);
		err = -EUCLEAN;
		goto out;
	}

	type = ch->node_type;
	if (type < 0 || type >= UBIFS_NODE_TYPES_CNT) {
		if (!quiet)
			ubifs_err(c, "bad node type %d", type);
		goto out;
	}

	node_len = le32_to_cpu(ch->len);
	if (node_len + offs > c->leb_size)
		goto out_len;

	if (c->ranges[type].max_len == 0) {
		if (node_len != c->ranges[type].len)
			goto out_len;
	} else if (node_len < c->ranges[type].min_len ||
		   node_len > c->ranges[type].max_len)
		goto out_len;

	if (!must_chk_crc && type == UBIFS_DATA_NODE && !c->mounting &&
	    !c->remounting_rw && c->no_chk_data_crc)
		return 0;

	crc = crc32(UBIFS_CRC32_INIT, buf + 8, node_len - 8);
	node_crc = le32_to_cpu(ch->crc);
	if (crc != node_crc) {
		if (!quiet)
			ubifs_err(c, "bad CRC: calculated %#08x, read %#08x",
				  crc, node_crc);
		err = -EUCLEAN;
		goto out;
	}

	return 0;

out_len:
	if (!quiet)
		ubifs_err(c, "bad node length %d", node_len);
out:
	if (!quiet) {
		ubifs_err(c, "bad node at LEB %d:%d", lnum, offs);
		ubifs_dump_node(c, buf);
		dump_stack();
	}
	return err;
}

/**
 * ubifs_pad - pad flash space.
 * @c: UBIFS file-system description object
 * @buf: buffer to put padding to
 * @pad: how many bytes to pad
 *
 * The flash media obliges us to write only in chunks of %c->min_io_size and
 * when we have to write less data we add padding node to the write-buffer and
 * pad it to the next minimal I/O unit's boundary. Padding nodes help when the
 * media is being scanned. If the amount of wasted space is not enough to fit a
 * padding node which takes %UBIFS_PAD_NODE_SZ bytes, we write padding bytes
 * pattern (%UBIFS_PADDING_BYTE).
 *
 * Padding nodes are also used to fill gaps when the "commit-in-gaps" method is
 * used.
 */
void ubifs_pad(const struct ubifs_info *c, void *buf, int pad)
{
	uint32_t crc;

	ubifs_assert(c, pad >= 0 && !(pad & 7));

	if (pad >= UBIFS_PAD_NODE_SZ) {
		struct ubifs_ch *ch = buf;
		struct ubifs_pad_node *pad_node = buf;

		ch->magic = cpu_to_le32(UBIFS_NODE_MAGIC);
		ch->node_type = UBIFS_PAD_NODE;
		ch->group_type = UBIFS_NO_NODE_GROUP;
		ch->padding[0] = ch->padding[1] = 0;
		ch->sqnum = 0;
		ch->len = cpu_to_le32(UBIFS_PAD_NODE_SZ);
		pad -= UBIFS_PAD_NODE_SZ;
		pad_node->pad_len = cpu_to_le32(pad);
		crc = crc32(UBIFS_CRC32_INIT, buf + 8, UBIFS_PAD_NODE_SZ - 8);
		ch->crc = cpu_to_le32(crc);
		memset(buf + UBIFS_PAD_NODE_SZ, 0, pad);
	} else if (pad > 0)
		/* Too little space, padding node won't fit */
		memset(buf, UBIFS_PADDING_BYTE, pad);
}

/*
 * removed in barebox
static unsigned long long next_sqnum(struct ubifs_info *c)
 */

/*
 * removed in barebox
void ubifs_prepare_node(struct ubifs_info *c, void *node, int len, int pad)
 */

/*
 * removed in barebox
void ubifs_prep_grp_node(struct ubifs_info *c, void *node, int len, int last)
 */

/*
 * removed in barebox
static enum hrtimer_restart wbuf_timer_callback_nolock(struct hrtimer *timer)
 */

/*
 * removed in barebox
static void new_wbuf_timer_nolock(struct ubifs_info *c, struct ubifs_wbuf *wbuf)
 */

/*
 * removed in barebox
static void cancel_wbuf_timer_nolock(struct ubifs_wbuf *wbuf)
 */

/*
 * removed in barebox
int ubifs_wbuf_sync_nolock(struct ubifs_wbuf *wbuf)
 */

/*
 * removed in barebox
int ubifs_wbuf_seek_nolock(struct ubifs_wbuf *wbuf, int lnum, int offs)
 */

/*
 * removed in barebox
int ubifs_bg_wbufs_sync(struct ubifs_info *c)
 */

/*
 * removed in barebox
int ubifs_wbuf_write_nolock(struct ubifs_wbuf *wbuf, void *buf, int len)
 */

/*
 * removed in barebox
int ubifs_write_node(struct ubifs_info *c, void *buf, int len, int lnum,
		     int offs)
 */

/*
 * removed in barebox
int ubifs_read_node_wbuf(struct ubifs_wbuf *wbuf, void *buf, int type, int len,
			 int lnum, int offs)
 */

/**
 * ubifs_read_node - read node.
 * @c: UBIFS file-system description object
 * @buf: buffer to read to
 * @type: node type
 * @len: node length (not aligned)
 * @lnum: logical eraseblock number
 * @offs: offset within the logical eraseblock
 *
 * This function reads a node of known type and and length, checks it and
 * stores in @buf. Returns zero in case of success, %-EUCLEAN if CRC mismatched
 * and a negative error code in case of failure.
 */
int ubifs_read_node(const struct ubifs_info *c, void *buf, int type, int len,
		    int lnum, int offs)
{
	int err, l;
	struct ubifs_ch *ch = buf;

	dbg_io("LEB %d:%d, %s, length %d", lnum, offs, dbg_ntype(type), len);
	ubifs_assert(c, lnum >= 0 && lnum < c->leb_cnt && offs >= 0);
	ubifs_assert(c, len >= UBIFS_CH_SZ && offs + len <= c->leb_size);
	ubifs_assert(c, !(offs & 7) && offs < c->leb_size);
	ubifs_assert(c, type >= 0 && type < UBIFS_NODE_TYPES_CNT);

	err = ubifs_leb_read(c, lnum, buf, offs, len, 0);
	if (err && err != -EBADMSG)
		return err;

	if (type != ch->node_type) {
		ubifs_errc(c, "bad node type (%d but expected %d)",
			   ch->node_type, type);
		goto out;
	}

	err = ubifs_check_node(c, buf, lnum, offs, 0, 0);
	if (err) {
		ubifs_errc(c, "expected node type %d", type);
		return err;
	}

	l = le32_to_cpu(ch->len);
	if (l != len) {
		ubifs_errc(c, "bad node length %d, expected %d", l, len);
		goto out;
	}

	return 0;

out:
	ubifs_errc(c, "bad node at LEB %d:%d, LEB mapping status %d", lnum,
		   offs, ubi_is_mapped(c->ubi, lnum));
	if (!c->probing) {
		ubifs_dump_node(c, buf);
		dump_stack();
	}
	return -EINVAL;
}

/**
 * ubifs_wbuf_init - initialize write-buffer.
 * @c: UBIFS file-system description object
 * @wbuf: write-buffer to initialize
 *
 * This function initializes write-buffer. Returns zero in case of success
 * %-ENOMEM in case of failure.
 */
int ubifs_wbuf_init(struct ubifs_info *c, struct ubifs_wbuf *wbuf)
{
	size_t size;

	wbuf->buf = kmalloc(c->max_write_size, GFP_KERNEL);
	if (!wbuf->buf)
		return -ENOMEM;

	size = (c->max_write_size / UBIFS_CH_SZ + 1) * sizeof(ino_t);
	wbuf->inodes = kmalloc(size, GFP_KERNEL);
	if (!wbuf->inodes) {
		kfree(wbuf->buf);
		wbuf->buf = NULL;
		return -ENOMEM;
	}

	wbuf->used = 0;
	wbuf->lnum = wbuf->offs = -1;
	/*
	 * If the LEB starts at the max. write size aligned address, then
	 * write-buffer size has to be set to @c->max_write_size. Otherwise,
	 * set it to something smaller so that it ends at the closest max.
	 * write size boundary.
	 */
	size = c->max_write_size - (c->leb_start % c->max_write_size);
	wbuf->avail = wbuf->size = size;
	wbuf->sync_callback = NULL;
	mutex_init(&wbuf->io_mutex);
	spin_lock_init(&wbuf->lock);
	wbuf->c = c;
	wbuf->next_ino = 0;

	/* hrtimer not needed in barebox */

	return 0;
}

/*
 * removed in barebox
void ubifs_wbuf_add_ino_nolock(struct ubifs_wbuf *wbuf, ino_t inum)
 */

/*
 * removed in barebox
static int wbuf_has_ino(struct ubifs_wbuf *wbuf, ino_t inum)
 */

/*
 * removed in barebox
int ubifs_sync_wbufs_by_inode(struct ubifs_info *c, struct inode *inode)
 */
