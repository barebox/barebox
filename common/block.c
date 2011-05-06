/*
 * block.c - simple block layer
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <common.h>
#include <block.h>
#include <linux/err.h>

#define BLOCKSIZE(blk)	(1 << blk->blockbits)

#define WRBUFFER_LAST(blk)	(blk->wrblock + blk->wrbufblocks - 1)

#ifdef CONFIG_BLOCK_WRITE
static int writebuffer_flush(struct block_device *blk)
{
	if (!blk->wrbufblocks)
		return 0;

	blk->ops->write(blk, blk->wrbuf, blk->wrblock,
			blk->wrbufblocks);

	blk->wrbufblocks = 0;

	return 0;
}

static int block_put(struct block_device *blk, const void *buf, int block)
{
	if (block >= blk->num_blocks)
		return -EIO;

	if (block < blk->wrblock || block > blk->wrblock + blk->wrbufblocks) {
		writebuffer_flush(blk);
	}

	if (blk->wrbufblocks == 0) {
		blk->wrblock = block;
		blk->wrbufblocks = 1;
	}

	memcpy(blk->wrbuf + (block - blk->wrblock) * BLOCKSIZE(blk),
			buf, BLOCKSIZE(blk));

	if (block > WRBUFFER_LAST(blk))
		blk->wrbufblocks++;

	if (blk->wrbufblocks == blk->wrbufsize)
		writebuffer_flush(blk);

	return 0;
}

#else
static int writebuffer_flush(struct block_device *blk)
{
	return 0;
}
#endif

static void *block_get(struct block_device *blk, int block)
{
	int ret;
	int num_blocks;

	if (block >= blk->num_blocks)
		return ERR_PTR(-EIO);

	/* first look into write buffer */
	if (block >= blk->wrblock && block <= WRBUFFER_LAST(blk))
		return blk->wrbuf + (block - blk->wrblock) * BLOCKSIZE(blk);

	/* then look into read buffer */
	if (block >= blk->rdblock && block <= blk->rdblockend)
		return blk->rdbuf + (block - blk->rdblock) * BLOCKSIZE(blk);

	/*
	 * If none of the buffers above match read the block from
	 * the device
	 */
	num_blocks = min(blk->rdbufsize, blk->num_blocks - block);

	ret = blk->ops->read(blk, blk->rdbuf, block, num_blocks);
	if (ret)
		return ERR_PTR(ret);

	blk->rdblock = block;
	blk->rdblockend = block + num_blocks - 1;

	return blk->rdbuf;
}

static ssize_t block_read(struct cdev *cdev, void *buf, size_t count,
		unsigned long offset, unsigned long flags)
{
	struct block_device *blk = cdev->priv;
	unsigned long mask = BLOCKSIZE(blk) - 1;
	unsigned long block = offset >> blk->blockbits;
	size_t icount = count;
	int blocks;

	if (offset & mask) {
		size_t now = BLOCKSIZE(blk) - (offset & mask);
		void *iobuf = block_get(blk, block);

		now = min(count, now);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		memcpy(buf, iobuf + (offset & mask), now);
		buf += now;
		count -= now;
		block++;
	}

	blocks = count >> blk->blockbits;

	while (blocks) {
		void *iobuf = block_get(blk, block);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		memcpy(buf, iobuf, BLOCKSIZE(blk));
		buf += BLOCKSIZE(blk);
		blocks--;
		block++;
		count -= BLOCKSIZE(blk);
	}

	if (count) {
		void *iobuf = block_get(blk, block);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		memcpy(buf, iobuf, count);
	}

	return icount;
}

#ifdef CONFIG_BLOCK_WRITE
static ssize_t block_write(struct cdev *cdev, const void *buf, size_t count,
		unsigned long offset, ulong flags)
{
	struct block_device *blk = cdev->priv;
	unsigned long mask = BLOCKSIZE(blk) - 1;
	unsigned long block = offset >> blk->blockbits;
	size_t icount = count;
	int blocks;

	if (offset & mask) {
		size_t now = BLOCKSIZE(blk) - (offset & mask);
		void *iobuf = block_get(blk, block);

		now = min(count, now);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		memcpy(iobuf + (offset & mask), buf, now);
		block_put(blk, iobuf, block);
		buf += now;
		count -= now;
		block++;
	}

	blocks = count >> blk->blockbits;

	while (blocks) {
		block_put(blk, buf, block);
		buf += BLOCKSIZE(blk);
		blocks--;
		block++;
		count -= BLOCKSIZE(blk);
	}

	if (count) {
		void *iobuf = block_get(blk, block);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		memcpy(iobuf, buf, count);
		block_put(blk, iobuf, block);
	}

	return icount;
}
#endif

static int block_close(struct cdev *cdev)
{
	struct block_device *blk = cdev->priv;

	return writebuffer_flush(blk);
}

static int block_flush(struct cdev *cdev)
{
	struct block_device *blk = cdev->priv;

	return writebuffer_flush(blk);
}

struct file_operations block_ops = {
	.read	= block_read,
#ifdef CONFIG_BLOCK_WRITE
	.write	= block_write,
#endif
	.close	= block_close,
	.flush	= block_flush,
	.lseek	= dev_lseek_default,
};

int blockdevice_register(struct block_device *blk)
{
	size_t size = blk->num_blocks * BLOCKSIZE(blk);
	int ret;

	blk->cdev.size = size;
	blk->cdev.dev = blk->dev;
	blk->cdev.ops = &block_ops;
	blk->cdev.priv = blk;
	blk->rdbufsize = PAGE_SIZE >> blk->blockbits;
	blk->rdbuf = xmalloc(PAGE_SIZE);
	blk->rdblock = 1;
	blk->rdblockend = 0;
	blk->wrbufsize = PAGE_SIZE >> blk->blockbits;
	blk->wrbuf = xmalloc(PAGE_SIZE);
	blk->wrblock = 0;
	blk->wrbufblocks = 0;

	ret = devfs_create(&blk->cdev);
	if (ret)
		return ret;

	return 0;
}

int blockdevice_unregister(struct block_device *blk)
{
	return 0;
}

