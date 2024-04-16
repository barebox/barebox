/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 */

#ifndef __JFFS2_OS_LINUX_H__
#define __JFFS2_OS_LINUX_H__

#include <linux/mtd/mtd.h>

/* JFFS2 uses Linux mode bits natively -- no need for conversion */
#define os_to_jffs2_mode(x) (x)
#define jffs2_to_os_mode(x) (x)

struct kstatfs;
struct kvec;
struct fs_device;

#define JFFS2_BLOCK_SIZE	PAGE_SIZE

#define JFFS2_INODE_INFO(i) (container_of(i, struct jffs2_inode_info, vfs_inode))
#define OFNI_EDONI_2SFFJ(f)  (&(f)->vfs_inode)
#define JFFS2_SB_INFO(sb) (sb->s_fs_info)
#define OFNI_BS_2SFFJ(c)  ((struct super_block *)c->os_priv)


#define JFFS2_F_I_SIZE(f) (OFNI_EDONI_2SFFJ(f)->i_size)
#define JFFS2_F_I_MODE(f) (OFNI_EDONI_2SFFJ(f)->i_mode)
#define JFFS2_F_I_UID(f) (i_uid_read(OFNI_EDONI_2SFFJ(f)))
#define JFFS2_F_I_GID(f) (i_gid_read(OFNI_EDONI_2SFFJ(f)))
#define JFFS2_F_I_RDEV(f) (OFNI_EDONI_2SFFJ(f)->i_rdev)

#define JFFS2_CLAMP_TIME(t) ((uint32_t)clamp_t(time64_t, (t), 0, U32_MAX))
#define ITIME(sec) ((struct timespec){sec, 0})
#define JFFS2_NOW() JFFS2_CLAMP_TIME(ktime_get_real_seconds())
#define I_SEC(tv) JFFS2_CLAMP_TIME((tv).tv_sec)
#define JFFS2_F_I_CTIME(f) I_SEC(OFNI_EDONI_2SFFJ(f)->i_ctime)
#define JFFS2_F_I_MTIME(f) I_SEC(OFNI_EDONI_2SFFJ(f)->i_mtime)
#define JFFS2_F_I_ATIME(f) I_SEC(OFNI_EDONI_2SFFJ(f)->i_atime)
#define sleep_on_spinunlock(wq, s)				\
	do {							\
		DECLARE_WAITQUEUE(__wait, current);		\
		add_wait_queue((wq), &__wait);			\
		set_current_state(TASK_UNINTERRUPTIBLE);	\
		spin_unlock(s);					\
		schedule();					\
		remove_wait_queue((wq), &__wait);		\
	} while(0)

static inline void jffs2_init_inode_info(struct jffs2_inode_info *f)
{
	f->highest_version = 0;
	f->fragtree = RB_ROOT;
	f->metadata = NULL;
	f->dents = NULL;
	f->target = NULL;
	f->flags = 0;
	f->usercompr = 0;
}

struct jffs2_file {
	struct inode *inode;
	void *buf;
	unsigned int offset;
};

/* Read-only support */
#define jffs2_is_readonly(c) (1)

#define SECTOR_ADDR(x) ( (((unsigned long)(x) / c->sector_size) * c->sector_size) )

/**
 * Read data from memory and ignore any hints about bitflips in case of NAND
 * memory (because we cannot repair them).
 */
static inline int jffs2_flash_read(struct jffs2_sb_info *c, loff_t ofs, size_t len, size_t *retlen, u_char *buf)
{
	int rc = mtd_read((c)->mtd, ofs, len, retlen, buf);
	if (rc == -EUCLEAN)
		return 0; // we are read-only, we cannot repair anything.

	return rc;
}

/* support run-time speed-up while scanning NAND flashs */
#define jffs2_cleanmarker_oob(c) (c->mtd->type == MTD_NANDFLASH)
/* no write buffer, due to read-only suppport */
#define jffs2_wbuf_dirty(c) (0)
#define jffs2_is_writebuffered(c) (0)

#ifdef CONFIG_JFFS2_SUMMARY
#define jffs2_can_mark_obsolete(c) (0)
#else
#define jffs2_can_mark_obsolete(c) (1)
#endif

#define jffs2_nand_flash_setup(c) (0)
#define jffs2_nand_flash_cleanup(c) do {} while(0)

#define jffs2_dataflash(c) (0)
#define jffs2_dataflash_setup(c) (0)
#define jffs2_dataflash_cleanup(c) do {} while (0)

#define jffs2_ubivol(c) (0)
#define jffs2_ubivol_setup(c) (0)
#define jffs2_ubivol_cleanup(c) do {} while (0)

#define jffs2_nor_wbuf_flash(c) (0)
#define jffs2_nor_wbuf_flash_cleanup(c) do {} while (0)

/* background.c */
int jffs2_start_garbage_collect_thread(struct jffs2_sb_info *c);
void jffs2_stop_garbage_collect_thread(struct jffs2_sb_info *c);
void jffs2_garbage_collect_trigger(struct jffs2_sb_info *c);

/* dir.c */
extern const struct file_operations jffs2_dir_operations;
extern const struct inode_operations jffs2_dir_inode_operations;

/* file.c */
extern const struct file_operations jffs2_file_operations;
extern const struct inode_operations jffs2_file_inode_operations;
extern const struct address_space_operations jffs2_file_address_operations;
int jffs2_fsync(struct file *, loff_t, loff_t, int);

/* ioctl.c */
long jffs2_ioctl(struct file *, unsigned int, unsigned long);

/* symlink.c */
extern const struct inode_operations jffs2_symlink_inode_operations;

/* fs.c */
struct inode *jffs2_iget(struct super_block *, unsigned long);
int jffs2_do_fill_super(struct super_block *sb, int);
void jffs2_gc_release_inode(struct jffs2_sb_info *c,
			    struct jffs2_inode_info *f);
struct jffs2_inode_info *jffs2_gc_fetch_inode(struct jffs2_sb_info *c,
					      int inum, int unlinked);

void jffs2_flash_cleanup(struct jffs2_sb_info *c);

/* super.c */
int jffs2_fill_super(struct fs_device *fsdev, int silent);

/* writev.c */
int jffs2_flash_direct_writev(struct jffs2_sb_info *c, const struct kvec *vecs,
		       unsigned long count, loff_t to, size_t *retlen);
int jffs2_flash_direct_write(struct jffs2_sb_info *c, loff_t ofs, size_t len,
			size_t *retlen, const u_char *buf);

#endif /* __JFFS2_OS_LINUX_H__ */

