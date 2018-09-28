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
 * Authors: Adrian Hunter
 *          Artem Bityutskiy (Битюцкий Артём)
 */

/*
 * This file implements commit-related functionality of the LEB properties
 * subsystem.
 */

#include <linux/err.h>
#include "crc16.h"
#include "ubifs.h"

/*
 * removed in barebox
static struct ubifs_cnode *first_dirty_cnode(const struct ubifs_info *c, struct ubifs_nnode *nnode)
 */

/*
 * removed in barebox
static struct ubifs_cnode *next_dirty_cnode(const struct ubifs_info *c, struct ubifs_cnode *cnode)
 */

/*
 * removed in barebox
static int get_cnodes_to_commit(struct ubifs_info *c)
 */

/*
 * removed in barebox
static void upd_ltab(struct ubifs_info *c, int lnum, int free, int dirty)
 */

/*
 * removed in barebox
static int alloc_lpt_leb(struct ubifs_info *c, int *lnum)
 */

/*
 * removed in barebox
static int layout_cnodes(struct ubifs_info *c)
 */

/*
 * removed in barebox
static int realloc_lpt_leb(struct ubifs_info *c, int *lnum)
 */

/*
 * removed in barebox
static int write_cnodes(struct ubifs_info *c)
 */

/*
 * removed in barebox
static struct ubifs_pnode *next_pnode_to_dirty(struct ubifs_info *c,
					       struct ubifs_pnode *pnode)
 */

/*
 * removed in barebox
static struct ubifs_pnode *pnode_lookup(struct ubifs_info *c, int i)
 */

/*
 * removed in barebox
static void add_pnode_dirt(struct ubifs_info *c, struct ubifs_pnode *pnode)
 */

/*
 * removed in barebox
static void do_make_pnode_dirty(struct ubifs_info *c, struct ubifs_pnode *pnode)
 */

/*
 * removed in barebox
static int make_tree_dirty(struct ubifs_info *c)
 */

/*
 * removed in barebox
static int need_write_all(struct ubifs_info *c)
 */

/*
 * removed in barebox
static void lpt_tgc_start(struct ubifs_info *c)
 */

/*
 * removed in barebox
static int lpt_tgc_end(struct ubifs_info *c)
 */

/*
 * removed in barebox
static void populate_lsave(struct ubifs_info *c)
 */

/*
 * removed in barebox
static struct ubifs_nnode *nnode_lookup(struct ubifs_info *c, int i)
 */

/*
 * removed in barebox
static int make_nnode_dirty(struct ubifs_info *c, int node_num, int lnum,
			    int offs)
 */

/*
 * removed in barebox
static int make_pnode_dirty(struct ubifs_info *c, int node_num, int lnum,
			    int offs)
 */

/*
 * removed in barebox
static int make_ltab_dirty(struct ubifs_info *c, int lnum, int offs)
 */

/*
 * removed in barebox
static int make_lsave_dirty(struct ubifs_info *c, int lnum, int offs)
 */

/*
 * removed in barebox
static int make_node_dirty(struct ubifs_info *c, int node_type, int node_num,
			   int lnum, int offs)
 */

/*
 * removed in barebox
static int get_lpt_node_len(const struct ubifs_info *c, int node_type)
 */

/*
 * removed in barebox
static int get_pad_len(const struct ubifs_info *c, uint8_t *buf, int len)
 */

/*
 * removed in barebox
static int get_lpt_node_type(const struct ubifs_info *c, uint8_t *buf,
			     int *node_num)
 */

/*
 * removed in barebox
static int is_a_node(const struct ubifs_info *c, uint8_t *buf, int len)
 */

/*
 * removed in barebox
static int lpt_gc_lnum(struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
static int lpt_gc(struct ubifs_info *c)
 */

/*
 * removed in barebox
int ubifs_lpt_start_commit(struct ubifs_info *c)
 */

/*
 * removed in barebox
static void free_obsolete_cnodes(struct ubifs_info *c)
 */

/*
 * removed in barebox
int ubifs_lpt_end_commit(struct ubifs_info *c)
 */

/*
 * removed in barebox
int ubifs_lpt_post_commit(struct ubifs_info *c)
 */

/*
 * removed in barebox
static struct ubifs_nnode *first_nnode(struct ubifs_info *c, int *hght)
 */

/*
 * removed in barebox
static struct ubifs_nnode *next_nnode(struct ubifs_info *c,
				      struct ubifs_nnode *nnode, int *hght)
 */

/*
 * removed in barebox
void ubifs_lpt_free(struct ubifs_info *c, int wr_only)
 */

/*
 * Everything below is related to debugging.
 */

/*
 * removed in barebox
static int dbg_is_all_ff(uint8_t *buf, int len)
 */

/*
 * removed in barebox
static int dbg_is_nnode_dirty(struct ubifs_info *c, int lnum, int offs)
 */

/*
 * removed in barebox
static int dbg_is_pnode_dirty(struct ubifs_info *c, int lnum, int offs)
 */

/*
 * removed in barebox
static int dbg_is_ltab_dirty(struct ubifs_info *c, int lnum, int offs)
 */

/*
 * removed in barebox
static int dbg_is_lsave_dirty(struct ubifs_info *c, int lnum, int offs)
 */

/*
 * removed in barebox
static int dbg_is_node_dirty(struct ubifs_info *c, int node_type, int lnum,
			     int offs)
*/

/*
 * removed in barebox
static int dbg_check_ltab_lnum(struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
int dbg_check_ltab(struct ubifs_info *c)
 */

/*
 * removed in barebox
int dbg_chk_lpt_free_spc(struct ubifs_info *c)
 */

/*
 * removed in barebox
int dbg_chk_lpt_sz(struct ubifs_info *c, int action, int len)
 */

/*
 * removed in barebox
static void dump_lpt_leb(const struct ubifs_info *c, int lnum)
 */

/*
 * removed in barebox
void ubifs_dump_lpt_lebs(const struct ubifs_info *c)
 */

/*
 * removed in barebox
static int dbg_populate_lsave(struct ubifs_info *c)
 */
