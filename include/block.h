#ifndef __BLOCK_H
#define __BLOCK_H

#include <driver.h>

struct block_device;

struct block_device_ops {
	int (*read)(struct block_device *, void *buf, int block, int num_blocks);
	int (*write)(struct block_device *, const void *buf, int block, int num_blocks);
};

struct block_device {
	struct device_d *dev;
	struct block_device_ops *ops;
	int blockbits;
	int num_blocks;
	void *rdbuf; /* read buffer */
	int rdbufsize;
	int rdblock; /* start block in read buffer */
	int rdblockend; /* end block in read buffer */
	void *wrbuf; /* write buffer */
	int wrblock; /* start block in write buffer */
	int wrbufblocks; /* number of blocks currently in write buffer */
	int wrbufsize; /* size of write buffer in blocks */
	struct cdev cdev;
};

int blockdevice_register(struct block_device *blk);
int blockdevice_unregister(struct block_device *blk);

#endif /* __BLOCK_H */
