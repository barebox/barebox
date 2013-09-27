#ifndef __BLOCK_H
#define __BLOCK_H

#include <driver.h>
#include <linux/list.h>

struct block_device;

struct block_device_ops {
	int (*read)(struct block_device *, void *buf, int block, int num_blocks);
	int (*write)(struct block_device *, const void *buf, int block, int num_blocks);
};

struct chunk;

struct block_device {
	struct device_d *dev;
	struct list_head list;
	struct block_device_ops *ops;
	int blockbits;
	int num_blocks;
	int rdbufsize;
	int blkmask;

	struct list_head buffered_blocks;
	struct list_head idle_blocks;

	struct cdev cdev;
};

extern struct list_head block_device_list;

#define for_each_block_device(bdev) list_for_each_entry(bdev, &block_device_list, list)

int blockdevice_register(struct block_device *blk);
int blockdevice_unregister(struct block_device *blk);

int block_read(struct block_device *blk, void *buf, int block, int num_blocks);
int block_write(struct block_device *blk, void *buf, int block, int num_blocks);

static inline int block_flush(struct block_device *blk)
{
	return cdev_flush(&blk->cdev);
}

#endif /* __BLOCK_H */
