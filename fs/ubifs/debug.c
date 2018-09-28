/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation
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
 * This file implements most of the debugging stuff which is compiled in only
 * when it is enabled. But some debugging check functions are implemented in
 * corresponding subsystem, just because they are closely related and utilize
 * various local functions of those subsystems.
 */

#include <linux/err.h>
#include "ubifs.h"

static const char *get_key_fmt(int fmt)
{
	switch (fmt) {
	case UBIFS_SIMPLE_KEY_FMT:
		return "simple";
	default:
		return "unknown/invalid format";
	}
}

static const char *get_key_hash(int hash)
{
	switch (hash) {
	case UBIFS_KEY_HASH_R5:
		return "R5";
	case UBIFS_KEY_HASH_TEST:
		return "test";
	default:
		return "unknown/invalid name hash";
	}
}

static const char *get_key_type(int type)
{
	switch (type) {
	case UBIFS_INO_KEY:
		return "inode";
	case UBIFS_DENT_KEY:
		return "direntry";
	case UBIFS_XENT_KEY:
		return "xentry";
	case UBIFS_DATA_KEY:
		return "data";
	case UBIFS_TRUN_KEY:
		return "truncate";
	default:
		return "unknown/invalid key";
	}
}

/*
 * removed in barebox
static const char *get_dent_type(int type)
 */

const char *dbg_snprintf_key(const struct ubifs_info *c,
			     const union ubifs_key *key, char *buffer, int len)
{
	char *p = buffer;
	int type = key_type(c, key);

	if (c->key_fmt == UBIFS_SIMPLE_KEY_FMT) {
		switch (type) {
		case UBIFS_INO_KEY:
			len -= snprintf(p, len, "(%lu, %s)",
					(unsigned long)key_inum(c, key),
					get_key_type(type));
			break;
		case UBIFS_DENT_KEY:
		case UBIFS_XENT_KEY:
			len -= snprintf(p, len, "(%lu, %s, %#08x)",
					(unsigned long)key_inum(c, key),
					get_key_type(type), key_hash(c, key));
			break;
		case UBIFS_DATA_KEY:
			len -= snprintf(p, len, "(%lu, %s, %u)",
					(unsigned long)key_inum(c, key),
					get_key_type(type), key_block(c, key));
			break;
		case UBIFS_TRUN_KEY:
			len -= snprintf(p, len, "(%lu, %s)",
					(unsigned long)key_inum(c, key),
					get_key_type(type));
			break;
		default:
			len -= snprintf(p, len, "(bad key type: %#08x, %#08x)",
					key->u32[0], key->u32[1]);
		}
	} else
		len -= snprintf(p, len, "bad key format %d", c->key_fmt);
	ubifs_assert(c, len > 0);
	return p;
}

const char *dbg_ntype(int type)
{
	switch (type) {
	case UBIFS_PAD_NODE:
		return "padding node";
	case UBIFS_SB_NODE:
		return "superblock node";
	case UBIFS_MST_NODE:
		return "master node";
	case UBIFS_REF_NODE:
		return "reference node";
	case UBIFS_INO_NODE:
		return "inode node";
	case UBIFS_DENT_NODE:
		return "direntry node";
	case UBIFS_XENT_NODE:
		return "xentry node";
	case UBIFS_DATA_NODE:
		return "data node";
	case UBIFS_TRUN_NODE:
		return "truncate node";
	case UBIFS_IDX_NODE:
		return "indexing node";
	case UBIFS_CS_NODE:
		return "commit start node";
	case UBIFS_ORPH_NODE:
		return "orphan node";
	default:
		return "unknown node";
	}
}

static const char *dbg_gtype(int type)
{
	switch (type) {
	case UBIFS_NO_NODE_GROUP:
		return "no node group";
	case UBIFS_IN_NODE_GROUP:
		return "in node group";
	case UBIFS_LAST_OF_NODE_GROUP:
		return "last of node group";
	default:
		return "unknown";
	}
}

const char *dbg_cstate(int cmt_state)
{
	switch (cmt_state) {
	case COMMIT_RESTING:
		return "commit resting";
	case COMMIT_BACKGROUND:
		return "background commit requested";
	case COMMIT_REQUIRED:
		return "commit required";
	case COMMIT_RUNNING_BACKGROUND:
		return "BACKGROUND commit running";
	case COMMIT_RUNNING_REQUIRED:
		return "commit running and required";
	case COMMIT_BROKEN:
		return "broken commit";
	default:
		return "unknown commit state";
	}
}

const char *dbg_jhead(int jhead)
{
	switch (jhead) {
	case GCHD:
		return "0 (GC)";
	case BASEHD:
		return "1 (base)";
	case DATAHD:
		return "2 (data)";
	default:
		return "unknown journal head";
	}
}

static void dump_ch(const struct ubifs_ch *ch)
{
	pr_err("\tmagic          %#x\n", le32_to_cpu(ch->magic));
	pr_err("\tcrc            %#x\n", le32_to_cpu(ch->crc));
	pr_err("\tnode_type      %d (%s)\n", ch->node_type,
	       dbg_ntype(ch->node_type));
	pr_err("\tgroup_type     %d (%s)\n", ch->group_type,
	       dbg_gtype(ch->group_type));
	pr_err("\tsqnum          %llu\n",
	       (unsigned long long)le64_to_cpu(ch->sqnum));
	pr_err("\tlen            %u\n", le32_to_cpu(ch->len));
}

void ubifs_dump_inode(struct ubifs_info *c, const struct inode *inode)
{
	/* removed in barebox */
}

void ubifs_dump_node(const struct ubifs_info *c, const void *node)
{
	int i, n;
	union ubifs_key key;
	const struct ubifs_ch *ch = node;
	char key_buf[DBG_KEY_BUF_LEN];

	/* If the magic is incorrect, just hexdump the first bytes */
	if (le32_to_cpu(ch->magic) != UBIFS_NODE_MAGIC) {
		pr_err("Not a node, first %zu bytes:", UBIFS_CH_SZ);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 32, 1,
			       (void *)node, UBIFS_CH_SZ, 1);
		return;
	}

	spin_lock(&dbg_lock);
	dump_ch(node);

	switch (ch->node_type) {
	case UBIFS_PAD_NODE:
	{
		const struct ubifs_pad_node *pad = node;

		pr_err("\tpad_len        %u\n", le32_to_cpu(pad->pad_len));
		break;
	}
	case UBIFS_SB_NODE:
	{
		const struct ubifs_sb_node *sup = node;
		unsigned int sup_flags = le32_to_cpu(sup->flags);

		pr_err("\tkey_hash       %d (%s)\n",
		       (int)sup->key_hash, get_key_hash(sup->key_hash));
		pr_err("\tkey_fmt        %d (%s)\n",
		       (int)sup->key_fmt, get_key_fmt(sup->key_fmt));
		pr_err("\tflags          %#x\n", sup_flags);
		pr_err("\tbig_lpt        %u\n",
		       !!(sup_flags & UBIFS_FLG_BIGLPT));
		pr_err("\tspace_fixup    %u\n",
		       !!(sup_flags & UBIFS_FLG_SPACE_FIXUP));
		pr_err("\tmin_io_size    %u\n", le32_to_cpu(sup->min_io_size));
		pr_err("\tleb_size       %u\n", le32_to_cpu(sup->leb_size));
		pr_err("\tleb_cnt        %u\n", le32_to_cpu(sup->leb_cnt));
		pr_err("\tmax_leb_cnt    %u\n", le32_to_cpu(sup->max_leb_cnt));
		pr_err("\tmax_bud_bytes  %llu\n",
		       (unsigned long long)le64_to_cpu(sup->max_bud_bytes));
		pr_err("\tlog_lebs       %u\n", le32_to_cpu(sup->log_lebs));
		pr_err("\tlpt_lebs       %u\n", le32_to_cpu(sup->lpt_lebs));
		pr_err("\torph_lebs      %u\n", le32_to_cpu(sup->orph_lebs));
		pr_err("\tjhead_cnt      %u\n", le32_to_cpu(sup->jhead_cnt));
		pr_err("\tfanout         %u\n", le32_to_cpu(sup->fanout));
		pr_err("\tlsave_cnt      %u\n", le32_to_cpu(sup->lsave_cnt));
		pr_err("\tdefault_compr  %u\n",
		       (int)le16_to_cpu(sup->default_compr));
		pr_err("\trp_size        %llu\n",
		       (unsigned long long)le64_to_cpu(sup->rp_size));
		pr_err("\trp_uid         %u\n", le32_to_cpu(sup->rp_uid));
		pr_err("\trp_gid         %u\n", le32_to_cpu(sup->rp_gid));
		pr_err("\tfmt_version    %u\n", le32_to_cpu(sup->fmt_version));
		pr_err("\ttime_gran      %u\n", le32_to_cpu(sup->time_gran));
		pr_err("\tUUID           %pUB\n", sup->uuid);
		break;
	}
	case UBIFS_MST_NODE:
	{
		const struct ubifs_mst_node *mst = node;

		pr_err("\thighest_inum   %llu\n",
		       (unsigned long long)le64_to_cpu(mst->highest_inum));
		pr_err("\tcommit number  %llu\n",
		       (unsigned long long)le64_to_cpu(mst->cmt_no));
		pr_err("\tflags          %#x\n", le32_to_cpu(mst->flags));
		pr_err("\tlog_lnum       %u\n", le32_to_cpu(mst->log_lnum));
		pr_err("\troot_lnum      %u\n", le32_to_cpu(mst->root_lnum));
		pr_err("\troot_offs      %u\n", le32_to_cpu(mst->root_offs));
		pr_err("\troot_len       %u\n", le32_to_cpu(mst->root_len));
		pr_err("\tgc_lnum        %u\n", le32_to_cpu(mst->gc_lnum));
		pr_err("\tihead_lnum     %u\n", le32_to_cpu(mst->ihead_lnum));
		pr_err("\tihead_offs     %u\n", le32_to_cpu(mst->ihead_offs));
		pr_err("\tindex_size     %llu\n",
		       (unsigned long long)le64_to_cpu(mst->index_size));
		pr_err("\tlpt_lnum       %u\n", le32_to_cpu(mst->lpt_lnum));
		pr_err("\tlpt_offs       %u\n", le32_to_cpu(mst->lpt_offs));
		pr_err("\tnhead_lnum     %u\n", le32_to_cpu(mst->nhead_lnum));
		pr_err("\tnhead_offs     %u\n", le32_to_cpu(mst->nhead_offs));
		pr_err("\tltab_lnum      %u\n", le32_to_cpu(mst->ltab_lnum));
		pr_err("\tltab_offs      %u\n", le32_to_cpu(mst->ltab_offs));
		pr_err("\tlsave_lnum     %u\n", le32_to_cpu(mst->lsave_lnum));
		pr_err("\tlsave_offs     %u\n", le32_to_cpu(mst->lsave_offs));
		pr_err("\tlscan_lnum     %u\n", le32_to_cpu(mst->lscan_lnum));
		pr_err("\tleb_cnt        %u\n", le32_to_cpu(mst->leb_cnt));
		pr_err("\tempty_lebs     %u\n", le32_to_cpu(mst->empty_lebs));
		pr_err("\tidx_lebs       %u\n", le32_to_cpu(mst->idx_lebs));
		pr_err("\ttotal_free     %llu\n",
		       (unsigned long long)le64_to_cpu(mst->total_free));
		pr_err("\ttotal_dirty    %llu\n",
		       (unsigned long long)le64_to_cpu(mst->total_dirty));
		pr_err("\ttotal_used     %llu\n",
		       (unsigned long long)le64_to_cpu(mst->total_used));
		pr_err("\ttotal_dead     %llu\n",
		       (unsigned long long)le64_to_cpu(mst->total_dead));
		pr_err("\ttotal_dark     %llu\n",
		       (unsigned long long)le64_to_cpu(mst->total_dark));
		break;
	}
	case UBIFS_REF_NODE:
	{
		const struct ubifs_ref_node *ref = node;

		pr_err("\tlnum           %u\n", le32_to_cpu(ref->lnum));
		pr_err("\toffs           %u\n", le32_to_cpu(ref->offs));
		pr_err("\tjhead          %u\n", le32_to_cpu(ref->jhead));
		break;
	}
	case UBIFS_INO_NODE:
	{
		const struct ubifs_ino_node *ino = node;

		key_read(c, &ino->key, &key);
		pr_err("\tkey            %s\n",
		       dbg_snprintf_key(c, &key, key_buf, DBG_KEY_BUF_LEN));
		pr_err("\tcreat_sqnum    %llu\n",
		       (unsigned long long)le64_to_cpu(ino->creat_sqnum));
		pr_err("\tsize           %llu\n",
		       (unsigned long long)le64_to_cpu(ino->size));
		pr_err("\tnlink          %u\n", le32_to_cpu(ino->nlink));
		pr_err("\tatime          %lld.%u\n",
		       (long long)le64_to_cpu(ino->atime_sec),
		       le32_to_cpu(ino->atime_nsec));
		pr_err("\tmtime          %lld.%u\n",
		       (long long)le64_to_cpu(ino->mtime_sec),
		       le32_to_cpu(ino->mtime_nsec));
		pr_err("\tctime          %lld.%u\n",
		       (long long)le64_to_cpu(ino->ctime_sec),
		       le32_to_cpu(ino->ctime_nsec));
		pr_err("\tuid            %u\n", le32_to_cpu(ino->uid));
		pr_err("\tgid            %u\n", le32_to_cpu(ino->gid));
		pr_err("\tmode           %u\n", le32_to_cpu(ino->mode));
		pr_err("\tflags          %#x\n", le32_to_cpu(ino->flags));
		pr_err("\txattr_cnt      %u\n", le32_to_cpu(ino->xattr_cnt));
		pr_err("\txattr_size     %u\n", le32_to_cpu(ino->xattr_size));
		pr_err("\txattr_names    %u\n", le32_to_cpu(ino->xattr_names));
		pr_err("\tcompr_type     %#x\n",
		       (int)le16_to_cpu(ino->compr_type));
		pr_err("\tdata len       %u\n", le32_to_cpu(ino->data_len));
		break;
	}
	case UBIFS_DENT_NODE:
	case UBIFS_XENT_NODE:
	{
		const struct ubifs_dent_node *dent = node;
		int nlen = le16_to_cpu(dent->nlen);

		key_read(c, &dent->key, &key);
		pr_err("\tkey            %s\n",
		       dbg_snprintf_key(c, &key, key_buf, DBG_KEY_BUF_LEN));
		pr_err("\tinum           %llu\n",
		       (unsigned long long)le64_to_cpu(dent->inum));
		pr_err("\ttype           %d\n", (int)dent->type);
		pr_err("\tnlen           %d\n", nlen);
		pr_err("\tname           ");

		if (nlen > UBIFS_MAX_NLEN)
			pr_err("(bad name length, not printing, bad or corrupted node)");
		else {
			for (i = 0; i < nlen && dent->name[i]; i++)
				pr_cont("%c", isprint(dent->name[i]) ?
					dent->name[i] : '?');
		}
		pr_cont("\n");

		break;
	}
	case UBIFS_DATA_NODE:
	{
		const struct ubifs_data_node *dn = node;
		int dlen = le32_to_cpu(ch->len) - UBIFS_DATA_NODE_SZ;

		key_read(c, &dn->key, &key);
		pr_err("\tkey            %s\n",
		       dbg_snprintf_key(c, &key, key_buf, DBG_KEY_BUF_LEN));
		pr_err("\tsize           %u\n", le32_to_cpu(dn->size));
		pr_err("\tcompr_typ      %d\n",
		       (int)le16_to_cpu(dn->compr_type));
		pr_err("\tdata size      %d\n", dlen);
		pr_err("\tdata:\n");
		print_hex_dump(KERN_ERR, "\t", DUMP_PREFIX_OFFSET, 32, 1,
			       (void *)&dn->data, dlen, 0);
		break;
	}
	case UBIFS_TRUN_NODE:
	{
		const struct ubifs_trun_node *trun = node;

		pr_err("\tinum           %u\n", le32_to_cpu(trun->inum));
		pr_err("\told_size       %llu\n",
		       (unsigned long long)le64_to_cpu(trun->old_size));
		pr_err("\tnew_size       %llu\n",
		       (unsigned long long)le64_to_cpu(trun->new_size));
		break;
	}
	case UBIFS_IDX_NODE:
	{
		const struct ubifs_idx_node *idx = node;

		n = le16_to_cpu(idx->child_cnt);
		pr_err("\tchild_cnt      %d\n", n);
		pr_err("\tlevel          %d\n", (int)le16_to_cpu(idx->level));
		pr_err("\tBranches:\n");

		for (i = 0; i < n && i < c->fanout - 1; i++) {
			const struct ubifs_branch *br;

			br = ubifs_idx_branch(c, idx, i);
			key_read(c, &br->key, &key);
			pr_err("\t%d: LEB %d:%d len %d key %s\n",
			       i, le32_to_cpu(br->lnum), le32_to_cpu(br->offs),
			       le32_to_cpu(br->len),
			       dbg_snprintf_key(c, &key, key_buf,
						DBG_KEY_BUF_LEN));
		}
		break;
	}
	case UBIFS_CS_NODE:
		break;
	case UBIFS_ORPH_NODE:
	{
		const struct ubifs_orph_node *orph = node;

		pr_err("\tcommit number  %llu\n",
		       (unsigned long long)
				le64_to_cpu(orph->cmt_no) & LLONG_MAX);
		pr_err("\tlast node flag %llu\n",
		       (unsigned long long)(le64_to_cpu(orph->cmt_no)) >> 63);
		n = (le32_to_cpu(ch->len) - UBIFS_ORPH_NODE_SZ) >> 3;
		pr_err("\t%d orphan inode numbers:\n", n);
		for (i = 0; i < n; i++)
			pr_err("\t  ino %llu\n",
			       (unsigned long long)le64_to_cpu(orph->inos[i]));
		break;
	}
	default:
		pr_err("node type %d was not recognized\n",
		       (int)ch->node_type);
	}
	spin_unlock(&dbg_lock);
}

/*
 * removed in barebox
void ubifs_dump_budget_req(const struct ubifs_budget_req *req)
 */

/*
 * removed in barebox
void ubifs_dump_lstats(const struct ubifs_lp_stats *lst)
 */

/*
 * removed in barebox
void ubifs_dump_budg(struct ubifs_info *c, const struct ubifs_budg_info *bi)
 */

/*
 * removed in barebox
void ubifs_dump_lprop(const struct ubifs_info *c, const struct ubifs_lprops *lp)
 */

/*
 * removed in barebox
void ubifs_dump_lprops(struct ubifs_info *c)
 */

/*
 * removed in barebox
void ubifs_dump_lpt_info(struct ubifs_info *c)
 */

/*
 * removed in barebox
void ubifs_dump_sleb(const struct ubifs_info *c,
		     const struct ubifs_scan_leb *sleb, int offs)
 */

/*
 * removed in barebox
void ubifs_dump_leb(const struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
void ubifs_dump_znode(const struct ubifs_info *c,
		      const struct ubifs_znode *znode)
 */


/*
 * removed in barebox
void ubifs_dump_heap(struct ubifs_info *c, struct ubifs_lpt_heap *heap, int cat)
 */

/*
 * removed in barebox
void ubifs_dump_pnode(struct ubifs_info *c, struct ubifs_pnode *pnode,
		      struct ubifs_nnode *parent, int iip)
 */

/*
 * removed in barebox
void ubifs_dump_tnc(struct ubifs_info *c)
 */


/*
 * removed in barebox
static int dump_znode(struct ubifs_info *c, struct ubifs_znode *znode,
		      void *priv)
 */

/*
 * removed in barebox
void ubifs_dump_index(struct ubifs_info *c)
 */

/*
 * removed in barebox
void dbg_save_space_info(struct ubifs_info *c)
 */

/*
 * removed in barebox
int dbg_check_space_info(struct ubifs_info *c)
 */

/*
 * removed in barebox
int dbg_check_synced_i_size(const struct ubifs_info *c, struct inode *inode)
 */

/*
 * removed in barebox
int dbg_check_dir(struct ubifs_info *c, const struct inode *dir)
 */

/*
 * removed in barebox
static int dbg_check_key_order(struct ubifs_info *c, struct ubifs_zbranch *zbr1,
			       struct ubifs_zbranch *zbr2)
 */

/*
 * removed in barebox
static int dbg_check_znode(struct ubifs_info *c, struct ubifs_zbranch *zbr)
 */

int dbg_check_tnc(struct ubifs_info *c, int extra)
{
	return 0;
}

/*
 * removed in barebox
int dbg_walk_index(struct ubifs_info *c, dbg_leaf_callback leaf_cb,
		   dbg_znode_callback znode_cb, void *priv)
 */

/*
 * removed in barebox
static int add_size(struct ubifs_info *c, struct ubifs_znode *znode, void *priv)
 */

/*
 * removed in barebox
int dbg_check_idx_size(struct ubifs_info *c, long long idx_size)
 */

/*
 * removed in barebox
static struct fsck_inode *add_inode(struct ubifs_info *c,
				    struct fsck_data *fsckd,
				    struct ubifs_ino_node *ino)
 */

/*
 * removed in barebox
static struct fsck_inode *search_inode(struct fsck_data *fsckd, ino_t inum)
 */

/*
 * removed in barebox
static struct fsck_inode *read_add_inode(struct ubifs_info *c,
					 struct fsck_data *fsckd, ino_t inum)
 */

/*
 * removed in barebox
static int check_leaf(struct ubifs_info *c, struct ubifs_zbranch *zbr,
		      void *priv)
 */

/*
 * removed in barebox
static void free_inodes(struct fsck_data *fsckd)
 */


/*
 * removed in barebox
static int check_inodes(struct ubifs_info *c, struct fsck_data *fsckd)
 */

/*
 * removed in barebox
int dbg_check_filesystem(struct ubifs_info *c)
 */

/*
 * removed in barebox
int dbg_check_data_nodes_order(struct ubifs_info *c, struct list_head *head)
 */

/*
 * removed in barebox
int dbg_check_nondata_nodes_order(struct ubifs_info *c, struct list_head *head)
 */

/*
 * removed in barebox
static inline int chance(unsigned int n, unsigned int out_of)
 */

/*
 * removed in barebox
static int power_cut_emulated(struct ubifs_info *c, int lnum, int write)
 */

/*
 * removed in barebox
static int corrupt_data(const struct ubifs_info *c, const void *buf,
			unsigned int len)
 */

/*
 * removed in barebox
 int dbg_leb_write(struct ubifs_info *c, int lnum, const void *buf,
		  int offs, int len)
 */

/*
 * removed in barebox
int dbg_leb_change(struct ubifs_info *c, int lnum, const void *buf,
		   int len)
 */

/*
 * removed in barebox
int dbg_leb_unmap(struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
int dbg_leb_map(struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
static int dfs_file_open(struct inode *inode, struct file *file)
 */

/*
 * removed in barebox
static int provide_user_output(int val, char __user *u, size_t count,
			       loff_t *ppos)
 */

/*
 * removed in barebox
static ssize_t dfs_file_read(struct file *file, char __user *u, size_t count,
			     loff_t *ppos)
 */

/*
 * removed in barebox
static int interpret_user_input(const char __user *u, size_t count)
 */

/*
 * removed in barebox
static ssize_t dfs_file_write(struct file *file, const char __user *u,
			      size_t count, loff_t *ppos)
 */

/*
 * removed in barebox
int dbg_debugfs_init_fs(struct ubifs_info *c)
 */

/*
 * removed in barebox
void dbg_debugfs_exit_fs(struct ubifs_info *c)
 */

/*
 * removed in barebox
static ssize_t dfs_global_file_read(struct file *file, char __user *u,
				    size_t count, loff_t *ppos)
 */

/*
 * removed in barebox
static ssize_t dfs_global_file_write(struct file *file, const char __user *u,
				     size_t count, loff_t *ppos)
 */

/*
 * removed in barebox
int dbg_debugfs_init(void)
 */

/*
 * removed in barebox
void dbg_debugfs_exit(void)
 */

void ubifs_assert_failed(struct ubifs_info *c, const char *expr,
			 const char *file, int line)
{
	ubifs_err(c, "UBIFS assert failed: %s, in %s:%u", expr, file, line);

	switch (c->assert_action) {
		case ASSACT_PANIC:
		BUG();
		break;

		case ASSACT_RO:
		ubifs_ro_mode(c, -EINVAL);
		break;

		case ASSACT_REPORT:
		default:
		dump_stack();
		break;

	}
}

/*
 * removed in barebox
int ubifs_debugging_init(struct ubifs_info *c)
 */

/*
 * removed in barebox
void ubifs_debugging_exit(struct ubifs_info *c)
 */
