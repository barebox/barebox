/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
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
 */

/*
 * This file implements UBIFS initialization and VFS superblock operations. Some
 * initialization stuff which is rather large and complex is placed at
 * corresponding subsystems, but most of it is here.
 */

#include <common.h>
#include <init.h>
#include <fs.h>
#include <malloc.h>
#include <linux/bug.h>
#include <linux/log2.h>
#include <linux/stat.h>
#include <linux/err.h>
#include "ubifs.h"
#include <mtd/ubi-user.h>

/* from include/linux/fs.h */
static inline void i_uid_write(struct inode *inode, uid_t uid)
{
	inode->i_uid = uid;
}

static inline void i_gid_write(struct inode *inode, gid_t gid)
{
	inode->i_gid = gid;
}

static void unlock_new_inode(struct inode *inode)
{
	return;
}

/*
 * Maximum amount of memory we may 'kmalloc()' without worrying that we are
 * allocating too much.
 */
#define UBIFS_KMALLOC_OK (128*1024)

/* Slab cache for UBIFS inodes */
static struct kmem_cache *ubifs_inode_slab;

/* UBIFS TNC shrinker description */
/* No shrinker in barebox */

/**
 * validate_inode - validate inode.
 * @c: UBIFS file-system description object
 * @inode: the inode to validate
 *
 * This is a helper function for 'ubifs_iget()' which validates various fields
 * of a newly built inode to make sure they contain sane values and prevent
 * possible vulnerabilities. Returns zero if the inode is all right and
 * a non-zero error code if not.
 */
static int validate_inode(struct ubifs_info *c, const struct inode *inode)
{
	int err;
	const struct ubifs_inode *ui = ubifs_inode(inode);

	if (inode->i_size > c->max_inode_sz) {
		ubifs_err(c, "inode is too large (%lld)",
			  (long long)inode->i_size);
		return 1;
	}

	if (ui->compr_type >= UBIFS_COMPR_TYPES_CNT) {
		ubifs_err(c, "unknown compression type %d", ui->compr_type);
		return 2;
	}

	if (ui->xattr_names + ui->xattr_cnt > XATTR_LIST_MAX)
		return 3;

	if (ui->data_len < 0 || ui->data_len > UBIFS_MAX_INO_DATA)
		return 4;

	if (ui->xattr && !S_ISREG(inode->i_mode))
		return 5;

	if (!ubifs_compr_present(c, ui->compr_type)) {
		ubifs_warn(c, "inode %lu uses '%s' compression, but it was not compiled in",
			   inode->i_ino, ubifs_compr_name(c, ui->compr_type));
	}

	err = 0;
	return err;
}

const struct inode_operations ubifs_file_inode_operations;
const struct file_operations ubifs_file_operations;

struct inode *ubifs_iget(struct super_block *sb, unsigned long inum)
{
	int err;
	union ubifs_key key;
	struct ubifs_ino_node *ino;
	struct ubifs_info *c = sb->s_fs_info;
	struct inode *inode;
	struct ubifs_inode *ui;

	dbg_gen("inode %lu", inum);

	inode = iget_locked(sb, inum);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;
	ui = ubifs_inode(inode);

	ino = kmalloc(UBIFS_MAX_INO_NODE_SZ, GFP_NOFS);
	if (!ino) {
		err = -ENOMEM;
		goto out;
	}

	ino_key_init(c, &key, inode->i_ino);

	err = ubifs_tnc_lookup(c, &key, ino);
	if (err)
		goto out_ino;

	inode->i_flags |= S_NOCMTIME;
#ifndef CONFIG_UBIFS_ATIME_SUPPORT
	inode->i_flags |= S_NOATIME;
#endif
	set_nlink(inode, le32_to_cpu(ino->nlink));
	i_uid_write(inode, le32_to_cpu(ino->uid));
	i_gid_write(inode, le32_to_cpu(ino->gid));
	inode->i_atime.tv_sec  = (int64_t)le64_to_cpu(ino->atime_sec);
	inode->i_atime.tv_nsec = le32_to_cpu(ino->atime_nsec);
	inode->i_mtime.tv_sec  = (int64_t)le64_to_cpu(ino->mtime_sec);
	inode->i_mtime.tv_nsec = le32_to_cpu(ino->mtime_nsec);
	inode->i_ctime.tv_sec  = (int64_t)le64_to_cpu(ino->ctime_sec);
	inode->i_ctime.tv_nsec = le32_to_cpu(ino->ctime_nsec);
	inode->i_mode = le32_to_cpu(ino->mode);
	inode->i_size = le64_to_cpu(ino->size);

	ui->data_len    = le32_to_cpu(ino->data_len);
	ui->flags       = le32_to_cpu(ino->flags);
	ui->compr_type  = le16_to_cpu(ino->compr_type);
	ui->creat_sqnum = le64_to_cpu(ino->creat_sqnum);
	ui->xattr_cnt   = le32_to_cpu(ino->xattr_cnt);
	ui->xattr_size  = le32_to_cpu(ino->xattr_size);
	ui->xattr_names = le32_to_cpu(ino->xattr_names);
	ui->synced_i_size = ui->ui_size = inode->i_size;

	ui->xattr = (ui->flags & UBIFS_XATTR_FL) ? 1 : 0;

	err = validate_inode(c, inode);
	if (err)
		goto out_invalid;

	switch (inode->i_mode & S_IFMT) {
	case S_IFREG:
		/* no address operations in barebox */
		inode->i_op = &ubifs_file_inode_operations;
		inode->i_fop = &ubifs_file_operations;
		if (ui->xattr) {
			ui->data = kmalloc(ui->data_len + 1, GFP_NOFS);
			if (!ui->data) {
				err = -ENOMEM;
				goto out_ino;
			}
			memcpy(ui->data, ino->data, ui->data_len);
			((char *)ui->data)[ui->data_len] = '\0';
		} else if (ui->data_len != 0) {
			err = 10;
			goto out_invalid;
		}
		break;
	case S_IFDIR:
		inode->i_op  = &ubifs_dir_inode_operations;
		inode->i_fop = &ubifs_dir_operations;
		if (ui->data_len != 0) {
			err = 11;
			goto out_invalid;
		}
		break;
	case S_IFLNK:
		inode->i_op = &simple_symlink_inode_operations;
		if (ui->data_len <= 0 || ui->data_len > UBIFS_MAX_INO_DATA) {
			err = 12;
			goto out_invalid;
		}
		ui->data = kmalloc(ui->data_len + 1, GFP_NOFS);
		if (!ui->data) {
			err = -ENOMEM;
			goto out_ino;
		}
		memcpy(ui->data, ino->data, ui->data_len);
		((char *)ui->data)[ui->data_len] = '\0';
		inode->i_link = ui->data;
		break;
	case S_IFBLK:
	case S_IFCHR:
	case S_IFSOCK:
	case S_IFIFO:
		/* No special files in barebox */
		break;
	default:
		err = 15;
		goto out_invalid;
	}

	kfree(ino);
	unlock_new_inode(inode);
	return inode;

out_invalid:
	ubifs_err(c, "inode %lu validation failed, error %d", inode->i_ino, err);
	ubifs_dump_node(c, ino);
	ubifs_dump_inode(c, inode);
	err = -EINVAL;
out_ino:
	kfree(ino);
out:
	ubifs_err(c, "failed to read inode %lu, error %d", inode->i_ino, err);
	iget_failed(inode);
	return ERR_PTR(err);
}

static struct inode *ubifs_alloc_inode(struct super_block *sb)
{
	struct ubifs_inode *ui;

	ui = kmem_cache_alloc(ubifs_inode_slab, GFP_NOFS);
	if (!ui)
		return NULL;

	memset((void *)ui + sizeof(struct inode), 0,
	       sizeof(struct ubifs_inode) - sizeof(struct inode));
	mutex_init(&ui->ui_mutex);
	spin_lock_init(&ui->ui_lock);
	return &ui->vfs_inode;
};

/*
 * removed in barebox
static void ubifs_i_callback(struct rcu_head *head)
 */

static void ubifs_destroy_inode(struct inode *inode)
{
	struct ubifs_inode *ui = ubifs_inode(inode);

	kfree(ui->data);
	kfree(ui);
}

/*
 * removed in barebox
static int ubifs_write_inode(struct inode *inode, struct writeback_control *wbc)
 */

/*
 * removed in barebox
static void ubifs_evict_inode(struct inode *inode)
 */

/*
 * removed in barebox
static void ubifs_dirty_inode(struct inode *inode, int flags)
 */

/*
 * removed in barebox
static int ubifs_statfs(struct dentry *dentry, struct kstatfs *buf)
 */

/*
 * removed in barebox
static int ubifs_show_options(struct seq_file *s, struct dentry *root)
 */

/*
 * removed in barebox
static int ubifs_sync_fs(struct super_block *sb, int wait)
 */

/**
 * init_constants_early - initialize UBIFS constants.
 * @c: UBIFS file-system description object
 *
 * This function initialize UBIFS constants which do not need the superblock to
 * be read. It also checks that the UBI volume satisfies basic UBIFS
 * requirements. Returns zero in case of success and a negative error code in
 * case of failure.
 */
static int init_constants_early(struct ubifs_info *c)
{
	if (c->vi.corrupted) {
		ubifs_warn(c, "UBI volume is corrupted - read-only mode");
		c->ro_media = 1;
	}

	if (c->di.ro_mode) {
		ubifs_msg(c, "read-only UBI device");
		c->ro_media = 1;
	}

	if (c->vi.vol_type == UBI_STATIC_VOLUME) {
		ubifs_msg(c, "static UBI volume - read-only mode");
		c->ro_media = 1;
	}

	c->leb_cnt = c->vi.size;
	c->leb_size = c->vi.usable_leb_size;
	c->leb_start = c->di.leb_start;
	c->half_leb_size = c->leb_size / 2;
	c->min_io_size = c->di.min_io_size;
	c->min_io_shift = fls(c->min_io_size) - 1;
	c->max_write_size = c->di.max_write_size;
	c->max_write_shift = fls(c->max_write_size) - 1;

	if (c->leb_size < UBIFS_MIN_LEB_SZ) {
		ubifs_errc(c, "too small LEBs (%d bytes), min. is %d bytes",
			   c->leb_size, UBIFS_MIN_LEB_SZ);
		return -EINVAL;
	}

	if (c->leb_cnt < UBIFS_MIN_LEB_CNT) {
		ubifs_errc(c, "too few LEBs (%d), min. is %d",
			   c->leb_cnt, UBIFS_MIN_LEB_CNT);
		return -EINVAL;
	}

	if (!is_power_of_2(c->min_io_size)) {
		ubifs_errc(c, "bad min. I/O size %d", c->min_io_size);
		return -EINVAL;
	}

	/*
	 * Maximum write size has to be greater or equivalent to min. I/O
	 * size, and be multiple of min. I/O size.
	 */
	if (c->max_write_size < c->min_io_size ||
	    c->max_write_size % c->min_io_size ||
	    !is_power_of_2(c->max_write_size)) {
		ubifs_errc(c, "bad write buffer size %d for %d min. I/O unit",
			   c->max_write_size, c->min_io_size);
		return -EINVAL;
	}

	/*
	 * UBIFS aligns all node to 8-byte boundary, so to make function in
	 * io.c simpler, assume minimum I/O unit size to be 8 bytes if it is
	 * less than 8.
	 */
	if (c->min_io_size < 8) {
		c->min_io_size = 8;
		c->min_io_shift = 3;
		if (c->max_write_size < c->min_io_size) {
			c->max_write_size = c->min_io_size;
			c->max_write_shift = c->min_io_shift;
		}
	}

	c->ref_node_alsz = ALIGN(UBIFS_REF_NODE_SZ, c->min_io_size);
	c->mst_node_alsz = ALIGN(UBIFS_MST_NODE_SZ, c->min_io_size);

	/*
	 * Initialize node length ranges which are mostly needed for node
	 * length validation.
	 */
	c->ranges[UBIFS_PAD_NODE].len  = UBIFS_PAD_NODE_SZ;
	c->ranges[UBIFS_SB_NODE].len   = UBIFS_SB_NODE_SZ;
	c->ranges[UBIFS_MST_NODE].len  = UBIFS_MST_NODE_SZ;
	c->ranges[UBIFS_REF_NODE].len  = UBIFS_REF_NODE_SZ;
	c->ranges[UBIFS_TRUN_NODE].len = UBIFS_TRUN_NODE_SZ;
	c->ranges[UBIFS_CS_NODE].len   = UBIFS_CS_NODE_SZ;

	c->ranges[UBIFS_INO_NODE].min_len  = UBIFS_INO_NODE_SZ;
	c->ranges[UBIFS_INO_NODE].max_len  = UBIFS_MAX_INO_NODE_SZ;
	c->ranges[UBIFS_ORPH_NODE].min_len =
				UBIFS_ORPH_NODE_SZ + sizeof(__le64);
	c->ranges[UBIFS_ORPH_NODE].max_len = c->leb_size;
	c->ranges[UBIFS_DENT_NODE].min_len = UBIFS_DENT_NODE_SZ;
	c->ranges[UBIFS_DENT_NODE].max_len = UBIFS_MAX_DENT_NODE_SZ;
	c->ranges[UBIFS_XENT_NODE].min_len = UBIFS_XENT_NODE_SZ;
	c->ranges[UBIFS_XENT_NODE].max_len = UBIFS_MAX_XENT_NODE_SZ;
	c->ranges[UBIFS_DATA_NODE].min_len = UBIFS_DATA_NODE_SZ;
	c->ranges[UBIFS_DATA_NODE].max_len = UBIFS_MAX_DATA_NODE_SZ;
	/*
	 * Minimum indexing node size is amended later when superblock is
	 * read and the key length is known.
	 */
	c->ranges[UBIFS_IDX_NODE].min_len = UBIFS_IDX_NODE_SZ + UBIFS_BRANCH_SZ;
	/*
	 * Maximum indexing node size is amended later when superblock is
	 * read and the fanout is known.
	 */
	c->ranges[UBIFS_IDX_NODE].max_len = INT_MAX;

	/*
	 * Initialize dead and dark LEB space watermarks. See gc.c for comments
	 * about these values.
	 */
	c->dead_wm = ALIGN(MIN_WRITE_SZ, c->min_io_size);
	c->dark_wm = ALIGN(UBIFS_MAX_NODE_SZ, c->min_io_size);

	/*
	 * Calculate how many bytes would be wasted at the end of LEB if it was
	 * fully filled with data nodes of maximum size. This is used in
	 * calculations when reporting free space.
	 */
	c->leb_overhead = c->leb_size % UBIFS_MAX_DATA_NODE_SZ;

	/* Buffer size for bulk-reads */
	c->max_bu_buf_len = UBIFS_MAX_BULK_READ * UBIFS_MAX_DATA_NODE_SZ;
	if (c->max_bu_buf_len > c->leb_size)
		c->max_bu_buf_len = c->leb_size;
	return 0;
}

/*
 * removed in barebox
static int bud_wbuf_callback(struct ubifs_info *c, int lnum, int free, int pad)
 */

/*
 * init_constants_sb - initialize UBIFS constants.
 * @c: UBIFS file-system description object
 *
 * This is a helper function which initializes various UBIFS constants after
 * the superblock has been read. It also checks various UBIFS parameters and
 * makes sure they are all right. Returns zero in case of success and a
 * negative error code in case of failure.
 */
static int init_constants_sb(struct ubifs_info *c)
{
	int tmp;
	long long tmp64;

	c->main_bytes = (long long)c->main_lebs * c->leb_size;
	c->max_znode_sz = sizeof(struct ubifs_znode) +
				c->fanout * sizeof(struct ubifs_zbranch);

	tmp = ubifs_idx_node_sz(c, 1);
	c->ranges[UBIFS_IDX_NODE].min_len = tmp;
	c->min_idx_node_sz = ALIGN(tmp, 8);

	tmp = ubifs_idx_node_sz(c, c->fanout);
	c->ranges[UBIFS_IDX_NODE].max_len = tmp;
	c->max_idx_node_sz = ALIGN(tmp, 8);

	/* Make sure LEB size is large enough to fit full commit */
	tmp = UBIFS_CS_NODE_SZ + UBIFS_REF_NODE_SZ * c->jhead_cnt;
	tmp = ALIGN(tmp, c->min_io_size);
	if (tmp > c->leb_size) {
		ubifs_err(c, "too small LEB size %d, at least %d needed",
			  c->leb_size, tmp);
		return -EINVAL;
	}

	/*
	 * Make sure that the log is large enough to fit reference nodes for
	 * all buds plus one reserved LEB.
	 */
	tmp64 = c->max_bud_bytes + c->leb_size - 1;
	c->max_bud_cnt = div_u64(tmp64, c->leb_size);
	tmp = (c->ref_node_alsz * c->max_bud_cnt + c->leb_size - 1);
	tmp /= c->leb_size;
	tmp += 1;
	if (c->log_lebs < tmp) {
		ubifs_err(c, "too small log %d LEBs, required min. %d LEBs",
			  c->log_lebs, tmp);
		return -EINVAL;
	}

	/*
	 * When budgeting we assume worst-case scenarios when the pages are not
	 * be compressed and direntries are of the maximum size.
	 *
	 * Note, data, which may be stored in inodes is budgeted separately, so
	 * it is not included into 'c->bi.inode_budget'.
	 */
	c->bi.page_budget = UBIFS_MAX_DATA_NODE_SZ * UBIFS_BLOCKS_PER_PAGE;
	c->bi.inode_budget = UBIFS_INO_NODE_SZ;
	c->bi.dent_budget = UBIFS_MAX_DENT_NODE_SZ;

	/*
	 * When the amount of flash space used by buds becomes
	 * 'c->max_bud_bytes', UBIFS just blocks all writers and starts commit.
	 * The writers are unblocked when the commit is finished. To avoid
	 * writers to be blocked UBIFS initiates background commit in advance,
	 * when number of bud bytes becomes above the limit defined below.
	 */
	c->bg_bud_bytes = (c->max_bud_bytes * 13) >> 4;

	/*
	 * Ensure minimum journal size. All the bytes in the journal heads are
	 * considered to be used, when calculating the current journal usage.
	 * Consequently, if the journal is too small, UBIFS will treat it as
	 * always full.
	 */
	tmp64 = (long long)(c->jhead_cnt + 1) * c->leb_size + 1;
	if (c->bg_bud_bytes < tmp64)
		c->bg_bud_bytes = tmp64;
	if (c->max_bud_bytes < tmp64 + c->leb_size)
		c->max_bud_bytes = tmp64 + c->leb_size;

	/* No lpt in barebox */

	/* Initialize effective LEB size used in budgeting calculations */
	c->idx_leb_size = c->leb_size - c->max_idx_node_sz;
	return 0;
}

/*
 * removed in barebox
static void init_constants_master(struct ubifs_info *c)
 */

/*
 * removed in barebox
static int take_gc_lnum(struct ubifs_info *c)
 */

/**
 * alloc_wbufs - allocate write-buffers.
 * @c: UBIFS file-system description object
 *
 * This helper function allocates and initializes UBIFS write-buffers. Returns
 * zero in case of success and %-ENOMEM in case of failure.
 */
static int alloc_wbufs(struct ubifs_info *c)
{
	int i, err;

	c->jheads = kcalloc(c->jhead_cnt, sizeof(struct ubifs_jhead),
			    GFP_KERNEL);
	if (!c->jheads)
		return -ENOMEM;

	/* Initialize journal heads */
	for (i = 0; i < c->jhead_cnt; i++) {
		INIT_LIST_HEAD(&c->jheads[i].buds_list);
		err = ubifs_wbuf_init(c, &c->jheads[i].wbuf);
		if (err)
			return err;

		c->jheads[i].wbuf.jhead = i;
		c->jheads[i].grouped = 1;
	}

	/*
	 * Garbage Collector head does not need to be synchronized by timer.
	 * Also GC head nodes are not grouped.
	 */
	c->jheads[GCHD].wbuf.no_timer = 1;
	c->jheads[GCHD].grouped = 0;

	return 0;
}

/**
 * free_wbufs - free write-buffers.
 * @c: UBIFS file-system description object
 */
static void free_wbufs(struct ubifs_info *c)
{
	int i;

	if (c->jheads) {
		for (i = 0; i < c->jhead_cnt; i++) {
			kfree(c->jheads[i].wbuf.buf);
			kfree(c->jheads[i].wbuf.inodes);
		}
		kfree(c->jheads);
		c->jheads = NULL;
	}
}

/*
 * removed in barebox
static void free_orphans(struct ubifs_info *c)
 */

/**
 * free_buds - free per-bud objects.
 * @c: UBIFS file-system description object
 */
static void free_buds(struct ubifs_info *c)
{
	struct ubifs_bud *bud, *n;

	rbtree_postorder_for_each_entry_safe(bud, n, &c->buds, rb)
		kfree(bud);
}

/**
 * check_volume_empty - check if the UBI volume is empty.
 * @c: UBIFS file-system description object
 *
 * This function checks if the UBIFS volume is empty by looking if its LEBs are
 * mapped or not. The result of checking is stored in the @c->empty variable.
 * Returns zero in case of success and a negative error code in case of
 * failure.
 */
static int check_volume_empty(struct ubifs_info *c)
{
	int lnum, err;

	c->empty = 1;
	for (lnum = 0; lnum < c->leb_cnt; lnum++) {
		err = ubifs_is_mapped(c, lnum);
		if (unlikely(err < 0))
			return err;
		if (err == 1) {
			c->empty = 0;
			break;
		}

		cond_resched();
	}

	return 0;
}

/*
 * removed in barebox
static int parse_standard_option(const char *option)
*/

/*
 * removed in barebox
static int ubifs_parse_options(struct ubifs_info *c, char *options,
			       int is_remount)
 */

/**
 * destroy_journal - destroy journal data structures.
 * @c: UBIFS file-system description object
 *
 * This function destroys journal data structures including those that may have
 * been created by recovery functions.
 */
static void destroy_journal(struct ubifs_info *c)
{
	while (!list_empty(&c->unclean_leb_list)) {
		struct ubifs_unclean_leb *ucleb;

		ucleb = list_entry(c->unclean_leb_list.next,
				   struct ubifs_unclean_leb, list);
		list_del(&ucleb->list);
		kfree(ucleb);
	}
	while (!list_empty(&c->old_buds)) {
		struct ubifs_bud *bud;

		bud = list_entry(c->old_buds.next, struct ubifs_bud, list);
		list_del(&bud->list);
		kfree(bud);
	}
	ubifs_destroy_size_tree(c);
	ubifs_tnc_close(c);
	free_buds(c);
}

/*
 * removed in barebox
static void bu_init(struct ubifs_info *c)
 */

/*
 * removed in barebox
static int check_free_space(struct ubifs_info *c)
 */

/**
 * mount_ubifs - mount UBIFS file-system.
 * @c: UBIFS file-system description object
 *
 * This function mounts UBIFS file system. Returns zero in case of success and
 * a negative error code in case of failure.
 */
static int mount_ubifs(struct ubifs_info *c)
{
	int err;
	long long x, y;
	size_t sz;

	/* Always readonly in barebox */
	c->ro_mount = true;
	/* Suppress error messages while probing if SB_SILENT is set */
	c->probing = !!(c->vfs_sb->s_flags & SB_SILENT);

	err = init_constants_early(c);
	if (err)
		return err;

	/* No debugging in barebox, use Kernel to debug */

	err = check_volume_empty(c);
	if (err)
		goto out_free;

	if (c->empty && (c->ro_mount || c->ro_media)) {
		/*
		 * This UBI volume is empty, and read-only, or the file system
		 * is mounted read-only - we cannot format it.
		 */
		ubifs_err(c, "can't format empty UBI volume: read-only %s",
			  c->ro_media ? "UBI volume" : "mount");
		err = -EROFS;
		goto out_free;
	}

	if (c->ro_media && !c->ro_mount) {
		ubifs_err(c, "cannot mount read-write - read-only media");
		err = -EROFS;
		goto out_free;
	}

	/*
	 * The requirement for the buffer is that it should fit indexing B-tree
	 * height amount of integers. We assume the height if the TNC tree will
	 * never exceed 64.
	 */
	err = -ENOMEM;
	c->bottom_up_buf = kmalloc_array(BOTTOM_UP_HEIGHT, sizeof(int),
					 GFP_KERNEL);
	if (!c->bottom_up_buf)
		goto out_free;

	c->sbuf = vmalloc(c->leb_size);
	if (!c->sbuf)
		goto out_free;


	c->mounting = 1;

	err = ubifs_read_superblock(c);
	if (err)
		goto out_free;

	c->probing = 0;

	/*
	 * Make sure the compressor which is set as default in the superblock
	 * or overridden by mount options is actually compiled in.
	 */
	if (!ubifs_compr_present(c, c->default_compr)) {
		ubifs_err(c, "'compressor \"%s\" is not compiled in",
			  ubifs_compr_name(c, c->default_compr));
		err = -ENOTSUPP;
		goto out_free;
	}

	err = init_constants_sb(c);
	if (err)
		goto out_free;

	sz = ALIGN(c->max_idx_node_sz, c->min_io_size);
	sz = ALIGN(sz + c->max_idx_node_sz, c->min_io_size);
	c->cbuf = kmalloc(sz, GFP_NOFS);
	if (!c->cbuf) {
		err = -ENOMEM;
		goto out_free;
	}

	err = alloc_wbufs(c);
	if (err)
		goto out_cbuf;

	sprintf(c->bgt_name, BGT_NAME_PATTERN, c->vi.ubi_num, c->vi.vol_id);

	err = ubifs_read_master(c);
	if (err)
		goto out_master;

	if ((c->mst_node->flags & cpu_to_le32(UBIFS_MST_DIRTY)) != 0) {
		ubifs_msg(c, "recovery needed");
		c->need_recovery = 1;
	}

	err = ubifs_replay_journal(c);
	if (err)
		goto out_journal;

	if (!c->ro_mount) {
	} else if (c->need_recovery) {
		err = ubifs_recover_size(c);
		if (err)
			goto out_orphans;
	}

	if (c->need_recovery) {
		if (c->ro_mount)
			ubifs_msg(c, "recovery deferred");
		else {
			c->need_recovery = 0;
			ubifs_msg(c, "recovery completed");
			/*
			 * GC LEB has to be empty and taken at this point. But
			 * the journal head LEBs may also be accounted as
			 * "empty taken" if they are empty.
			 */
			ubifs_assert(c, c->lst.taken_empty_lebs > 0);
		}
	} else {
		/* c->lst.taken_empty_lebs is always 0 in ro implementation */
	}

	c->mounting = 0;

	ubifs_msg(c, "UBIFS: mounted UBI device %d, volume %d, name \"%s\"%s",
		  c->vi.ubi_num, c->vi.vol_id, c->vi.name,
		  c->ro_mount ? ", R/O mode" : "");
	x = (long long)c->main_lebs * c->leb_size;
	y = (long long)c->log_lebs * c->leb_size + c->max_bud_bytes;
	ubifs_msg(c, "LEB size: %d bytes (%d KiB), min./max. I/O unit sizes: %d bytes/%d bytes",
		  c->leb_size, c->leb_size >> 10, c->min_io_size,
		  c->max_write_size);
	ubifs_msg(c, "FS size: %lld bytes (%lld MiB, %d LEBs), journal size %lld bytes (%lld MiB, %d LEBs)",
		  x, x >> 20, c->main_lebs,
		  y, y >> 20, c->log_lebs + c->max_bud_cnt);
	ubifs_msg(c, "reserved for root: %llu bytes (%llu KiB)",
		  c->report_rp_size, c->report_rp_size >> 10);
	ubifs_msg(c, "media format: w%d/r%d (latest is w%d/r%d), UUID %pUB%s",
		  c->fmt_version, c->ro_compat_version,
		  UBIFS_FORMAT_VERSION, UBIFS_RO_COMPAT_VERSION, c->uuid,
		  c->big_lpt ? ", big LPT model" : ", small LPT model");

	dbg_gen("default compressor:  %s", ubifs_compr_name(c, c->default_compr));
	dbg_gen("data journal heads:  %d",
		c->jhead_cnt - NONDATA_JHEADS_CNT);
	dbg_gen("log LEBs:            %d (%d - %d)",
		c->log_lebs, UBIFS_LOG_LNUM, c->log_last);
	dbg_gen("LPT area LEBs:       %d (%d - %d)",
		c->lpt_lebs, c->lpt_first, c->lpt_last);
	dbg_gen("orphan area LEBs:    %d (%d - %d)",
		c->orph_lebs, c->orph_first, c->orph_last);
	dbg_gen("main area LEBs:      %d (%d - %d)",
		c->main_lebs, c->main_first, c->leb_cnt - 1);
	dbg_gen("index LEBs:          %d", c->lst.idx_lebs);
	dbg_gen("total index bytes:   %lld (%lld KiB, %lld MiB)",
		c->bi.old_idx_sz, c->bi.old_idx_sz >> 10,
		c->bi.old_idx_sz >> 20);
	dbg_gen("key hash type:       %d", c->key_hash_type);
	dbg_gen("tree fanout:         %d", c->fanout);
	dbg_gen("reserved GC LEB:     %d", c->gc_lnum);
	dbg_gen("max. znode size      %d", c->max_znode_sz);
	dbg_gen("max. index node size %d", c->max_idx_node_sz);
	dbg_gen("node sizes:          data %zu, inode %zu, dentry %zu",
		UBIFS_DATA_NODE_SZ, UBIFS_INO_NODE_SZ, UBIFS_DENT_NODE_SZ);
	dbg_gen("node sizes:          trun %zu, sb %zu, master %zu",
		UBIFS_TRUN_NODE_SZ, UBIFS_SB_NODE_SZ, UBIFS_MST_NODE_SZ);
	dbg_gen("node sizes:          ref %zu, cmt. start %zu, orph %zu",
		UBIFS_REF_NODE_SZ, UBIFS_CS_NODE_SZ, UBIFS_ORPH_NODE_SZ);
	dbg_gen("max. node sizes:     data %zu, inode %zu dentry %zu, idx %d",
		UBIFS_MAX_DATA_NODE_SZ, UBIFS_MAX_INO_NODE_SZ,
		UBIFS_MAX_DENT_NODE_SZ, ubifs_idx_node_sz(c, c->fanout));
	dbg_gen("dead watermark:      %d", c->dead_wm);
	dbg_gen("dark watermark:      %d", c->dark_wm);
	dbg_gen("LEB overhead:        %d", c->leb_overhead);
	x = (long long)c->main_lebs * c->dark_wm;
	dbg_gen("max. dark space:     %lld (%lld KiB, %lld MiB)",
		x, x >> 10, x >> 20);
	dbg_gen("maximum bud bytes:   %lld (%lld KiB, %lld MiB)",
		c->max_bud_bytes, c->max_bud_bytes >> 10,
		c->max_bud_bytes >> 20);
	dbg_gen("BG commit bud bytes: %lld (%lld KiB, %lld MiB)",
		c->bg_bud_bytes, c->bg_bud_bytes >> 10,
		c->bg_bud_bytes >> 20);
	dbg_gen("current bud bytes    %lld (%lld KiB, %lld MiB)",
		c->bud_bytes, c->bud_bytes >> 10, c->bud_bytes >> 20);
	dbg_gen("max. seq. number:    %llu", c->max_sqnum);
	dbg_gen("commit number:       %llu", c->cmt_no);

	return 0;

out_orphans:
out_journal:
	destroy_journal(c);
out_master:
	kfree(c->mst_node);
	kfree(c->rcvrd_mst_node);
	if (c->bgt)
		kthread_stop(c->bgt);
	free_wbufs(c);
out_cbuf:
	kfree(c->cbuf);
out_free:
	kfree(c->write_reserve_buf);
	kfree(c->bu.buf);
	vfree(c->ileb_buf);
	vfree(c->sbuf);
	kfree(c->bottom_up_buf);
	return err;
}

/**
 * ubifs_umount - un-mount UBIFS file-system.
 * @c: UBIFS file-system description object
 *
 * Note, this function is called to free allocated resourced when un-mounting,
 * as well as free resources when an error occurred while we were half way
 * through mounting (error path cleanup function). So it has to make sure the
 * resource was actually allocated before freeing it.
 */
void ubifs_umount(struct ubifs_info *c)
{
	dbg_gen("un-mounting UBI device %d, volume %d", c->vi.ubi_num,
		c->vi.vol_id);

	spin_lock(&ubifs_infos_lock);
	list_del(&c->infos_list);
	spin_unlock(&ubifs_infos_lock);

	free_wbufs(c);

	kfree(c->cbuf);
	kfree(c->rcvrd_mst_node);
	kfree(c->mst_node);
	kfree(c->write_reserve_buf);
	kfree(c->bu.buf);
	vfree(c->ileb_buf);
	vfree(c->sbuf);
	kfree(c->bottom_up_buf);
}

/*
 * removed in barebox
static int ubifs_remount_rw(struct ubifs_info *c)
 */

/*
 * removed in barebox
static void ubifs_remount_ro(struct ubifs_info *c)
 */

/*
 * removed in barebox
static void ubifs_put_super(struct super_block *sb)
 */

/*
 * removed in barebox
static int ubifs_remount_fs(struct super_block *sb, int *flags, char *data)
 */

const struct super_operations ubifs_super_operations = {
	.alloc_inode   = ubifs_alloc_inode,
	.destroy_inode = ubifs_destroy_inode,
};

/*
 * removed in barebox
static struct ubi_volume_desc *open_ubi(const char *name, int mode)
 */

static struct ubifs_info *alloc_ubifs_info(struct ubi_volume_desc *ubi)
{
	struct ubifs_info *c;

	c = kzalloc(sizeof(struct ubifs_info), GFP_KERNEL);
	if (c) {
		spin_lock_init(&c->cnt_lock);
		spin_lock_init(&c->cs_lock);
		spin_lock_init(&c->buds_lock);
		spin_lock_init(&c->space_lock);
		spin_lock_init(&c->orphan_lock);
		init_rwsem(&c->commit_sem);
		mutex_init(&c->lp_mutex);
		mutex_init(&c->tnc_mutex);
		mutex_init(&c->log_mutex);
		mutex_init(&c->umount_mutex);
		mutex_init(&c->bu_mutex);
		mutex_init(&c->write_reserve_mutex);
		init_waitqueue_head(&c->cmt_wq);
		c->buds = RB_ROOT;
		c->old_idx = RB_ROOT;
		c->size_tree = RB_ROOT;
		c->orph_tree = RB_ROOT;
		INIT_LIST_HEAD(&c->infos_list);
		INIT_LIST_HEAD(&c->idx_gc);
		INIT_LIST_HEAD(&c->replay_list);
		INIT_LIST_HEAD(&c->replay_buds);
		INIT_LIST_HEAD(&c->uncat_list);
		INIT_LIST_HEAD(&c->empty_list);
		INIT_LIST_HEAD(&c->freeable_list);
		INIT_LIST_HEAD(&c->frdi_idx_list);
		INIT_LIST_HEAD(&c->unclean_leb_list);
		INIT_LIST_HEAD(&c->old_buds);
		INIT_LIST_HEAD(&c->orph_list);
		INIT_LIST_HEAD(&c->orph_new);
		c->no_chk_data_crc = 1;
		c->assert_action = ASSACT_RO;

		c->highest_inum = UBIFS_FIRST_INO;
		c->lhead_lnum = c->ltail_lnum = UBIFS_LOG_LNUM;

		ubi_get_volume_info(ubi, &c->vi);
		ubi_get_device_info(c->vi.ubi_num, &c->di);
	}
	return c;
}

static int ubifs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct ubifs_info *c = sb->s_fs_info;
	struct inode *root;
	int err;

	c->vfs_sb = sb;

	sb->s_fs_info = c;
	sb->s_magic = UBIFS_SUPER_MAGIC;
	sb->s_blocksize = UBIFS_BLOCK_SIZE;
	sb->s_blocksize_bits = UBIFS_BLOCK_SHIFT;
	sb->s_maxbytes = c->max_inode_sz = key_max_inode_size(c);
	if (c->max_inode_sz > MAX_LFS_FILESIZE)
		sb->s_maxbytes = c->max_inode_sz = MAX_LFS_FILESIZE;
	sb->s_op = &ubifs_super_operations;
#ifdef CONFIG_UBIFS_FS_XATTR
	sb->s_xattr = ubifs_xattr_handlers;
#endif
#ifdef CONFIG_UBIFS_FS_ENCRYPTION
	sb->s_cop = &ubifs_crypt_operations;
#endif

	mutex_lock(&c->umount_mutex);
	err = mount_ubifs(c);
	if (err) {
		ubifs_assert(c, err < 0);
		goto out_unlock;
	}

	/* Read the root inode */
	root = ubifs_iget(sb, UBIFS_ROOT_INO);
	if (IS_ERR(root)) {
		err = PTR_ERR(root);
		goto out_umount;
	}

	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_umount;
	}

	mutex_unlock(&c->umount_mutex);
	return 0;

out_umount:
	ubifs_umount(c);
out_unlock:
	mutex_unlock(&c->umount_mutex);

	return err;
}

/*
 * removed in barebox
static int sb_test(struct super_block *sb, void *data)
 */

/*
 * removed in barebox
static int sb_set(struct super_block *sb, void *data)
 */

/*
 * removed in barebox
static struct dentry *ubifs_mount(struct file_system_type *fs_type, int flags,
			const char *name, void *data)
 */

/*
 * removed in barebox
static void kill_ubifs_super(struct super_block *s)
 */

/*
 * Inode slab cache constructor.
 */
static void inode_slab_ctor(void *obj)
{
}

static int __init ubifs_init(void)
{
	int err;

	BUILD_BUG_ON(sizeof(struct ubifs_ch) != 24);

	/* Make sure node sizes are 8-byte aligned */
	BUILD_BUG_ON(UBIFS_CH_SZ        & 7);
	BUILD_BUG_ON(UBIFS_INO_NODE_SZ  & 7);
	BUILD_BUG_ON(UBIFS_DENT_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_XENT_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_DATA_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_TRUN_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_SB_NODE_SZ   & 7);
	BUILD_BUG_ON(UBIFS_MST_NODE_SZ  & 7);
	BUILD_BUG_ON(UBIFS_REF_NODE_SZ  & 7);
	BUILD_BUG_ON(UBIFS_CS_NODE_SZ   & 7);
	BUILD_BUG_ON(UBIFS_ORPH_NODE_SZ & 7);

	BUILD_BUG_ON(UBIFS_MAX_DENT_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_MAX_XENT_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_MAX_DATA_NODE_SZ & 7);
	BUILD_BUG_ON(UBIFS_MAX_INO_NODE_SZ  & 7);
	BUILD_BUG_ON(UBIFS_MAX_NODE_SZ      & 7);
	BUILD_BUG_ON(MIN_WRITE_SZ           & 7);

	/* Check min. node size */
	BUILD_BUG_ON(UBIFS_INO_NODE_SZ  < MIN_WRITE_SZ);
	BUILD_BUG_ON(UBIFS_DENT_NODE_SZ < MIN_WRITE_SZ);
	BUILD_BUG_ON(UBIFS_XENT_NODE_SZ < MIN_WRITE_SZ);
	BUILD_BUG_ON(UBIFS_TRUN_NODE_SZ < MIN_WRITE_SZ);

	BUILD_BUG_ON(UBIFS_MAX_DENT_NODE_SZ > UBIFS_MAX_NODE_SZ);
	BUILD_BUG_ON(UBIFS_MAX_XENT_NODE_SZ > UBIFS_MAX_NODE_SZ);
	BUILD_BUG_ON(UBIFS_MAX_DATA_NODE_SZ > UBIFS_MAX_NODE_SZ);
	BUILD_BUG_ON(UBIFS_MAX_INO_NODE_SZ  > UBIFS_MAX_NODE_SZ);

	/* Defined node sizes */
	BUILD_BUG_ON(UBIFS_SB_NODE_SZ  != 4096);
	BUILD_BUG_ON(UBIFS_MST_NODE_SZ != 512);
	BUILD_BUG_ON(UBIFS_INO_NODE_SZ != 160);
	BUILD_BUG_ON(UBIFS_REF_NODE_SZ != 64);

	/*
	 * We use 2 bit wide bit-fields to store compression type, which should
	 * be amended if more compressors are added. The bit-fields are:
	 * @compr_type in 'struct ubifs_inode', @default_compr in
	 * 'struct ubifs_info' and @compr_type in 'struct ubifs_mount_opts'.
	 */
	BUILD_BUG_ON(UBIFS_COMPR_TYPES_CNT > 4);

	/*
	 * We require that PAGE_SIZE is greater-than-or-equal-to
	 * UBIFS_BLOCK_SIZE. It is assumed that both are powers of 2.
	 */
	if (PAGE_SIZE < UBIFS_BLOCK_SIZE) {
		pr_err("UBIFS error: VFS page cache size is %u bytes, but UBIFS requires at least 4096 bytes",
		       (unsigned int)PAGE_SIZE);
		return -EINVAL;
	}

	ubifs_inode_slab = kmem_cache_create("ubifs_inode_slab",
				sizeof(struct ubifs_inode), 0,
				SLAB_MEM_SPREAD | SLAB_RECLAIM_ACCOUNT,
				&inode_slab_ctor);
	if (!ubifs_inode_slab)
		return -ENOMEM;

	err = ubifs_compressors_init();
	if (err)
		goto out_shrinker;

	return 0;

out_shrinker:
	kmem_cache_destroy(ubifs_inode_slab);
	return err;
}
/* late_initcall to let compressors initialize first */
late_initcall(ubifs_init);

int ubifs_get_super(struct device_d *dev, struct ubi_volume_desc *ubi, int silent)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct super_block *sb;
	struct ubifs_info *c;
	int err;

	sb = &fsdev->sb;
	c = alloc_ubifs_info(ubi);

	c->dev = dev;
	sb->s_fs_info = c;
	strncpy(sb->s_id, dev->name, sizeof(sb->s_id));

	c->ubi = ubi;

	err = ubifs_fill_super(sb, NULL, silent);
	if (err) {
		ubifs_assert(c, err < 0);
		goto out;
	}
	sb->s_dev = c->vi.cdev;

	return 0;
out:
	kfree(c);
	return err;
}

/*
 * removed in barebox
static void __exit ubifs_exit(void)
 */
