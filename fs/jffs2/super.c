// SPDX-License-Identifier: GPL-2.0-only

/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 */
#define pr_fmt(fmt) "jffs2: " fmt
#include <common.h>
#include <init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <fs.h>
#include <linux/err.h>
#include <linux/mount.h>
#include <linux/jffs2.h>
#include <linux/pagemap.h>
#include <linux/mtd/mtd.h>
#include <linux/ctype.h>
#include <linux/namei.h>
#include "compr.h"
#include "nodelist.h"

static struct kmem_cache *jffs2_inode_cachep;

/* from include/linux/fs.h */
static inline void i_uid_write(struct inode *inode, uid_t uid)
{
	inode->i_uid = uid;
}

static inline void i_gid_write(struct inode *inode, gid_t gid)
{
	inode->i_gid = gid;
}


static struct inode *jffs2_alloc_inode(struct super_block *sb)
{
	struct jffs2_inode_info *f;

	f = kmem_cache_alloc(jffs2_inode_cachep, GFP_KERNEL);
	if (!f)
		return NULL;
	return &f->vfs_inode;
}

static void jffs2_destroy_inode(struct inode *inode)
{
	struct jffs2_inode_info *f = JFFS2_INODE_INFO(inode);

	kfree(f->target);
	kmem_cache_free(jffs2_inode_cachep, f);
}

static void jffs2_i_init_once(void *foo)
{
}

static const struct super_operations jffs2_super_operations =
{
	.alloc_inode	= jffs2_alloc_inode,
	.destroy_inode	= jffs2_destroy_inode,
};

/*
 * fill in the superblock
 */
int jffs2_fill_super(struct fs_device *fsdev, int silent)
{
	struct super_block *sb = &fsdev->sb;
	struct jffs2_sb_info *c = sb->s_fs_info;

	c->os_priv = sb;

	/* Initialize JFFS2 superblock locks, the further initialization will
	 * be done later */
	mutex_init(&c->alloc_sem);
	mutex_init(&c->erase_free_sem);
	init_waitqueue_head(&c->erase_wait);
	init_waitqueue_head(&c->inocache_wq);
	spin_lock_init(&c->erase_completion_lock);
	spin_lock_init(&c->inocache_lock);

	sb->s_op = &jffs2_super_operations;
	sb->s_flags = sb->s_flags | SB_NOATIME;
	sb->s_xattr = jffs2_xattr_handlers;

	return jffs2_do_fill_super(sb, silent);
}

static int __init init_jffs2_fs(void)
{
	/* Paranoia checks for on-medium structures. If we ask GCC
	   to pack them with __attribute__((packed)) then it _also_
	   assumes that they're not aligned -- so it emits crappy
	   code on some architectures. Ideally we want an attribute
	   which means just 'no padding', without the alignment
	   thing. But GCC doesn't have that -- we have to just
	   hope the structs are the right sizes, instead. */
	BUILD_BUG_ON(sizeof(struct jffs2_unknown_node) != 12);
	BUILD_BUG_ON(sizeof(struct jffs2_raw_dirent) != 40);
	BUILD_BUG_ON(sizeof(struct jffs2_raw_inode) != 68);
	BUILD_BUG_ON(sizeof(struct jffs2_raw_summary) != 32);

	pr_info("JFFS version 2.2."
#ifdef CONFIG_JFFS2_FS_WRITEBUFFER
	       " (NAND)"
#endif
#ifdef CONFIG_JFFS2_SUMMARY
	       " (SUMMARY) "
#endif
	       " © 2001-2006 Red Hat, Inc.\n");

	jffs2_inode_cachep = kmem_cache_create("jffs2_i",
					     sizeof(struct jffs2_inode_info),
					     0, (SLAB_RECLAIM_ACCOUNT|
						SLAB_MEM_SPREAD|SLAB_ACCOUNT),
					     jffs2_i_init_once);
	if (!jffs2_inode_cachep) {
		pr_err("error: Failed to initialise inode cache\n");
		return -ENOMEM;
	}

	return 0;
}
late_initcall(init_jffs2_fs);
