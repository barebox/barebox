/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BLOCK_H
#define __BLOCK_H

#include <driver.h>
#include <disks.h>
#include <linux/list.h>
#include <linux/types.h>

struct block_device;
struct file_list;

struct block_device_ops {
	int (*read)(struct block_device *, void *buf, sector_t block, blkcnt_t num_blocks);
	int (*write)(struct block_device *, const void *buf, sector_t block, blkcnt_t num_blocks);
	int (*erase)(struct block_device *blk, sector_t block, blkcnt_t num_blocks);
	int (*flush)(struct block_device *);
	char *(*get_root)(struct block_device *blk, const struct cdev *partcdev);
};

struct chunk;

enum blk_type {
	BLK_TYPE_UNSPEC = 0,
	BLK_TYPE_USB,
	BLK_TYPE_SD,
	BLK_TYPE_AHCI,
	BLK_TYPE_IDE,
	BLK_TYPE_NVME,
	BLK_TYPE_VIRTUAL,
	BLK_TYPE_MMC,
	BLK_TYPE_COUNT,
};

const char *blk_type_str(enum blk_type);

struct block_device_stats {
	blkcnt_t read_sectors;
	blkcnt_t write_sectors;
	blkcnt_t erase_sectors;
};

struct block_device {
	struct device *dev;
	struct list_head list;
	struct block_device_ops *ops;
	u8 blockbits;
	u8 type; /* holds enum blk_type */
	u8 rootwait:1;
	u8 removable:1;
	blkcnt_t num_blocks;
	int rdbufsize;
	int blkmask;

	loff_t discard_start;
	loff_t discard_size;

	struct list_head buffered_blocks;
	struct list_head idle_blocks;

	struct cdev cdev;

	bool need_reparse;

#ifdef CONFIG_BLOCK_STATS
	struct block_device_stats stats;
#endif
};

#define BLOCKSIZE(blk)	(1u << (blk)->blockbits)

int block_size_bits(struct device *dev, unsigned block_size);

static inline u64 blockdevice_size(const struct block_device *blk)
{
	return blk->num_blocks << blk->blockbits;
}

static inline blkcnt_t
blockdevice_round_nblocks(const struct block_device *blk, u64 nbytes)
{
	if (nbytes == 0)
		return 0;

	return (((u64)nbytes - 1) >> blk->blockbits) + 1;
}

static inline u64
blockdevice_round_block_nbytes(const struct block_device *blk, u64 nbytes)
{
	return blockdevice_round_nblocks(blk, nbytes) << blk->blockbits;
}

extern struct list_head block_device_list;

#define for_each_block_device(bdev) list_for_each_entry(bdev, &block_device_list, list)

int blockdevice_register(struct block_device *blk);
int blockdevice_unregister(struct block_device *blk);

int block_read(struct block_device *blk, void *buf, sector_t block, blkcnt_t num_blocks);
int block_write(struct block_device *blk, void *buf, sector_t block, blkcnt_t num_blocks);

static inline int block_flush(struct block_device *blk)
{
	return cdev_flush(&blk->cdev);
}

#ifdef CONFIG_BLOCK
unsigned file_list_add_blockdevs(struct file_list *files);
char *cdev_get_linux_rootarg(const struct cdev *partcdev);
int cdev_get_linux_root_and_opts(const struct cdev *partcdev, const char **root,
				 const char **rootopts);
#else
static inline unsigned file_list_add_blockdevs(struct file_list *files)
{
	return 0;
}
static inline char *cdev_get_linux_rootarg(const struct cdev *partcdev)
{
	return NULL;
}
static inline int cdev_get_linux_root_and_opts(const struct cdev *partcdev,
					       const char **root,
					       const char **rootopts)
{
	return -ENOSYS;
}
#endif

static inline bool cdev_is_block_device(const struct cdev *cdev)
{
	return IS_ENABLED(CONFIG_BLOCK) && cdev &&
		(cdev->flags & DEVFS_IS_BLOCK_DEV);
}

static inline bool cdev_is_block_partition(const struct cdev *cdev)
{
	return cdev_is_block_device(cdev) && cdev_is_partition(cdev);
}

static inline bool cdev_is_block_disk(const struct cdev *cdev)
{
	return cdev_is_block_device(cdev) && !cdev_is_partition(cdev);
}

static inline struct block_device *cdev_get_block_device(const struct cdev *cdev)
{
	return cdev_is_block_device(cdev) ? cdev->priv : NULL;
}

static inline unsigned cdev_blockbits(const struct cdev *cdev)
{
	struct block_device *bdev = cdev_get_block_device(cdev);
	return bdev ? bdev->blockbits : SECTOR_SHIFT;
}

static inline unsigned cdev_blocksize(const struct cdev *cdev)
{
	return 1u << cdev_blockbits(cdev);
}

#endif /* __BLOCK_H */
