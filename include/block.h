#ifndef __BLOCK_H
#define __BLOCK_H

#include <driver.h>

struct block_device;

struct block_device_ops {
	int (*read)(struct block_device *, void *buf, int block, int num_blocks);
	int (*write)(struct block_device *, const void *buf, int block, int num_blocks);
};

struct chunk;

struct block_device {
	struct device_d *dev;
	struct block_device_ops *ops;
	int blockbits;
	int num_blocks;
	int rdbufsize;
	int blkmask;

	struct list_head buffered_blocks;
	struct list_head idle_blocks;

	struct cdev cdev;
};

int blockdevice_register(struct block_device *blk);
int blockdevice_unregister(struct block_device *blk);

#endif /* __BLOCK_H */
