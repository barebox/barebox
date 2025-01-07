// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 */
#define pr_fmt(fmt) "jffs2: " fmt
#include <common.h>
#include <crc.h>
#include <driver.h>
#include <init.h>
#include <fs.h>
#include <malloc.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/pagemap.h>
#include <linux/stat.h>
#include "compr.h"
#include "nodelist.h"
#include "os-linux.h"

static int jffs2_flash_setup(struct jffs2_sb_info *c);

/* from include/linux/fs.h */
static inline void i_uid_write(struct inode *inode, uid_t uid)
{
	inode->i_uid = uid;
}

static inline void i_gid_write(struct inode *inode, gid_t gid)
{
	inode->i_gid = gid;
}

const struct file_operations jffs2_file_operations;
const struct inode_operations jffs2_file_inode_operations;

static int jffs2_open(struct device *dev, FILE *file, const char *filename)
{
	struct inode *inode = file->f_inode;
	struct jffs2_file *jf;

	jf = xzalloc(sizeof(*jf));

	jf->inode = inode;
	jf->buf = xmalloc(JFFS2_BLOCK_SIZE);
	jf->offset = -1;

	file->private_data = jf;

	return 0;
}

static int jffs2_close(struct device *dev, FILE *f)
{
	struct jffs2_file *jf = f->private_data;

	free(jf->buf);
	free(jf);

	return 0;
}

static int jffs2_get_block(struct jffs2_file *jf, unsigned int pos)
{
	struct jffs2_sb_info *c = JFFS2_SB_INFO(jf->inode->i_sb);
	struct jffs2_inode_info *f = JFFS2_INODE_INFO(jf->inode);
	int ret;

	/* pos always has to be 4096 bytes aligned here */
	WARN_ON(pos % JFFS2_BLOCK_SIZE != 0);

	if (pos != jf->offset) {
		ret = jffs2_read_inode_range(c, f, jf->buf, pos,
					     JFFS2_BLOCK_SIZE);
		if (ret && ret != -ENOENT)
			return ret;
		jf->offset = pos;
	}

	return 0;
}

static int jffs2_read(struct device *_dev, FILE *f, void *buf,
		      size_t insize)
{
	struct jffs2_file *jf = f->private_data;
	unsigned int pos = f->f_pos;
	unsigned int ofs;
	unsigned int now;
	unsigned int size = insize;
	int ret;

	/* Read till end of current block */
	ofs = f->f_pos % JFFS2_BLOCK_SIZE;
	if (ofs) {
		ret = jffs2_get_block(jf, f->f_pos - ofs); /* Align down block */
		if (ret)
			return ret;

		now = min(size, JFFS2_BLOCK_SIZE - ofs);

		/* Complete block has been read, re-apply ofset now */
		memcpy(buf, jf->buf + ofs, now);
		size -= now;
		pos += now;
		buf += now;
	}

	/* Do full blocks */
	while (size >= JFFS2_BLOCK_SIZE) {
		ret = jffs2_get_block(jf, pos);
		if (ret)
			return ret;

		memcpy(buf, jf->buf, JFFS2_BLOCK_SIZE);
		size -= JFFS2_BLOCK_SIZE;
		pos += JFFS2_BLOCK_SIZE;
		buf += JFFS2_BLOCK_SIZE;
	}

	/* And the rest */
	if (size) {
		ret = jffs2_get_block(jf, pos);
		if (ret)
			return ret;
		memcpy(buf, jf->buf, size);
	}

	return insize;

}

struct inode *jffs2_iget(struct super_block *sb, unsigned long ino)
{
	struct jffs2_inode_info *f;
	struct jffs2_sb_info *c;
	struct jffs2_raw_inode latest_node;
	struct inode *inode;
	int ret;

	jffs2_dbg(1, "%s(): ino == %lu\n", __func__, ino);

	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;

	f = JFFS2_INODE_INFO(inode);
	c = JFFS2_SB_INFO(inode->i_sb);

	jffs2_init_inode_info(f);
	mutex_lock(&f->sem);

	ret = jffs2_do_read_inode(c, f, inode->i_ino, &latest_node);
	if (ret)
		goto error;

	inode->i_mode = jemode_to_cpu(latest_node.mode);
	i_uid_write(inode, je16_to_cpu(latest_node.uid));
	i_gid_write(inode, je16_to_cpu(latest_node.gid));
	inode->i_size = je32_to_cpu(latest_node.isize);
	inode->i_atime = ITIME(je32_to_cpu(latest_node.atime));
	inode->i_mtime = ITIME(je32_to_cpu(latest_node.mtime));
	inode->i_ctime = ITIME(je32_to_cpu(latest_node.ctime));

	set_nlink(inode, f->inocache->pino_nlink);

	inode->i_blocks = (inode->i_size + 511) >> 9;

	switch (inode->i_mode & S_IFMT) {

	case S_IFLNK:
		inode->i_op = &simple_symlink_inode_operations;
		inode->i_link = f->target;
		break;

	case S_IFDIR:
	{
		struct jffs2_full_dirent *fd;
		set_nlink(inode, 2); /* parent and '.' */

		for (fd=f->dents; fd; fd = fd->next) {
			if (fd->type == DT_DIR && fd->ino)
				inc_nlink(inode);
		}
		/* Root dir gets i_nlink 3 for some reason */
		if (inode->i_ino == 1)
			inc_nlink(inode);

		inode->i_op = &jffs2_dir_inode_operations;
		inode->i_fop = &jffs2_dir_operations;
		break;
	}
	case S_IFREG:
		inode->i_op = &jffs2_file_inode_operations;
		inode->i_fop = &jffs2_file_operations;
		break;

	case S_IFBLK:
	case S_IFCHR:
	case S_IFSOCK:
	case S_IFIFO:
		break;

	default:
		pr_warn("%s(): Bogus i_mode %o for ino %lu\n",
			__func__, inode->i_mode, (unsigned long)inode->i_ino);
	}

	mutex_unlock(&f->sem);

	jffs2_dbg(1, "jffs2_read_inode() returning\n");
	return inode;

error:
	mutex_unlock(&f->sem);
	iget_failed(inode);
	return ERR_PTR(ret);
}

static int calculate_inocache_hashsize(uint32_t flash_size)
{
	/*
	 * Pick a inocache hash size based on the size of the medium.
	 * Count how many megabytes we're dealing with, apply a hashsize twice
	 * that size, but rounding down to the usual big powers of 2. And keep
	 * to sensible bounds.
	 */

	int size_mb = flash_size / 1024 / 1024;
	int hashsize = (size_mb * 2) & ~0x3f;

	if (hashsize < INOCACHE_HASHSIZE_MIN)
		return INOCACHE_HASHSIZE_MIN;
	if (hashsize > INOCACHE_HASHSIZE_MAX)
		return INOCACHE_HASHSIZE_MAX;

	return hashsize;
}

static void jffs2_put_super(struct super_block *sb)
{
	if (sb->s_fs_info) {
		struct jffs2_sb_info *ctx = sb->s_fs_info;

		jffs2_free_ino_caches(ctx);
		jffs2_free_raw_node_refs(ctx);
		kfree(ctx->blocks);
		kfree(ctx->inocache_list);
		jffs2_flash_cleanup(ctx);

		kfree(sb->s_fs_info);
	}
}

int jffs2_do_fill_super(struct super_block *sb, int silent)
{
	struct jffs2_sb_info *c;
	struct inode *root_i;
	int ret;
	size_t blocks;

	c = JFFS2_SB_INFO(sb);

	c->flash_size = c->mtd->size;
	c->sector_size = c->mtd->erasesize;
	blocks = c->flash_size / c->sector_size;

	/*
	 * Size alignment check
	 */
	if ((c->sector_size * blocks) != c->flash_size) {
		c->flash_size = c->sector_size * blocks;
		pr_info("Flash size not aligned to erasesize, reducing to %dKiB",
			c->flash_size / 1024);
	}

	if (c->flash_size < 5*c->sector_size) {
		pr_err("Too few erase blocks (%d)",
		       c->flash_size / c->sector_size);
		return -EINVAL;
	}

	c->cleanmarker_size = sizeof(struct jffs2_unknown_node);

	/* NAND (or other bizarre) flash... do setup accordingly */
	ret = jffs2_flash_setup(c);
	if (ret)
		return ret;

	c->inocache_hashsize = calculate_inocache_hashsize(c->flash_size);
	c->inocache_list = kcalloc(c->inocache_hashsize, sizeof(struct jffs2_inode_cache *), GFP_KERNEL);
	if (!c->inocache_list) {
		ret = -ENOMEM;
		goto out_wbuf;
	}

	jffs2_init_xattr_subsystem(c);

	if ((ret = jffs2_do_mount_fs(c)))
		goto out_inohash;

	jffs2_dbg(1, "%s(): Getting root inode\n", __func__);
	root_i = jffs2_iget(sb, 1);
	if (IS_ERR(root_i)) {
		jffs2_dbg(1, "get root inode failed\n");
		ret = PTR_ERR(root_i);
		goto out_root;
	}

	ret = -ENOMEM;

	jffs2_dbg(1, "%s(): d_make_root()\n", __func__);
	sb->s_root = d_make_root(root_i);
	if (!sb->s_root)
		goto out_root;

	sb->s_maxbytes = 0xFFFFFFFF;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = JFFS2_SUPER_MAGIC;

	return 0;

out_root:
	jffs2_free_ino_caches(c);
	jffs2_free_raw_node_refs(c);
out_inohash:
	kfree(c->inocache_list);
out_wbuf:
	jffs2_flash_cleanup(c);
	return ret;
}

static int jffs2_flash_setup(struct jffs2_sb_info *c) {
	int ret = 0;

	if (jffs2_cleanmarker_oob(c)) {
		/* NAND flash... do setup accordingly */
		ret = jffs2_nand_flash_setup(c);
		if (ret)
			return ret;
	}

	/* and Dataflash */
	if (jffs2_dataflash(c)) {
		ret = jffs2_dataflash_setup(c);
		if (ret)
			return ret;
	}

	/* and an UBI volume */
	if (jffs2_ubivol(c)) {
		ret = jffs2_ubivol_setup(c);
		if (ret)
			return ret;
	}

	return ret;
}

void jffs2_flash_cleanup(struct jffs2_sb_info *c) {

	if (jffs2_cleanmarker_oob(c)) {
		jffs2_nand_flash_cleanup(c);
	}

	/* and DataFlash */
	if (jffs2_dataflash(c)) {
		jffs2_dataflash_cleanup(c);
	}

	/* and Intel "Sibley" flash */
	if (jffs2_nor_wbuf_flash(c)) {
		jffs2_nor_wbuf_flash_cleanup(c);
	}

	/* and an UBI volume */
	if (jffs2_ubivol(c)) {
		jffs2_ubivol_cleanup(c);
	}
}

static int jffs2_probe_cnt;

static int jffs2_probe(struct device *dev)
{
	struct fs_device *fsdev;
	struct super_block *sb;
	struct jffs2_sb_info *ctx;
	int ret;

	fsdev = dev_to_fs_device(dev);
	sb = &fsdev->sb;

	ret = fsdev_open_cdev(fsdev);
	if (ret)
		goto err_out;

	ctx = kzalloc(sizeof(struct jffs2_sb_info), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->mtd = fsdev->cdev->mtd;

	sb->s_fs_info = ctx;

	if (!jffs2_probe_cnt) {
		ret = jffs2_compressors_init();
		if (ret) {
			pr_err("error: Failed to initialise compressors\n");
			goto err_out;
		}
	}

        if (jffs2_fill_super(fsdev, 0)) {
		dev_err(dev, "no valid jffs2 found\n");
		ret = -EINVAL;
		goto err_compressors;
	}

	jffs2_probe_cnt++;

	return 0;

err_compressors:
	jffs2_compressors_exit();
err_out:

	return ret;
}

static void jffs2_remove(struct device *dev)
{
	struct fs_device *fsdev;
	struct super_block *sb;

	fsdev = dev_to_fs_device(dev);
	sb = &fsdev->sb;

	jffs2_probe_cnt--;

	if (!jffs2_probe_cnt) {
		jffs2_compressors_exit();
	}

	jffs2_put_super(sb);
}


static struct fs_driver jffs2_driver = {
	.open = jffs2_open,
	.close = jffs2_close,
	.read = jffs2_read,
	.type = filetype_jffs2,
	.flags     = 0,
	.drv = {
		.probe  = jffs2_probe,
		.remove = jffs2_remove,
		.name = "jffs2",
	}
};

static int jffs2_init(void)
{
	return register_fs_driver(&jffs2_driver);
}
coredevice_initcall(jffs2_init);
