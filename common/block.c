// SPDX-License-Identifier: GPL-2.0-only
/*
 * block.c - simple block layer
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <block.h>
#include <disks.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/list.h>
#include <dma.h>
#include <range.h>
#include <bootargs.h>
#include <file-list.h>

LIST_HEAD(block_device_list);

/* a chunk of contiguous data */
struct chunk {
	void *data; /* data buffer */
	sector_t block_start; /* first block in this chunk */
	int dirty; /* need to write back to device */
	int num; /* number of chunk, debugging only */
	struct list_head list;
};

#define BUFSIZE (PAGE_SIZE * 16)

static int writebuffer_io_len(struct block_device *blk, struct chunk *chunk)
{
	return min_t(blkcnt_t, blk->rdbufsize, blk->num_blocks - chunk->block_start);
}

#ifdef CONFIG_BLOCK_STATS
static void blk_stats_record_read(struct block_device *blk, blkcnt_t count)
{
	blk->stats.read_sectors += count;
}
static void blk_stats_record_write(struct block_device *blk, blkcnt_t count)
{
	blk->stats.write_sectors += count;
}
static void blk_stats_record_erase(struct block_device *blk, blkcnt_t count)
{
	blk->stats.erase_sectors += count;
}
#else
static void blk_stats_record_read(struct block_device *blk, blkcnt_t count) { }
static void blk_stats_record_write(struct block_device *blk, blkcnt_t count) { }
static void blk_stats_record_erase(struct block_device *blk, blkcnt_t count) { }
#endif

static int chunk_flush(struct block_device *blk, struct chunk *chunk)
{
	size_t len;
	int ret;

	if (!chunk->dirty)
		return 0;

	len = writebuffer_io_len(blk, chunk);
	ret = blk->ops->write(blk, chunk->data, chunk->block_start, len);
	if (ret < 0)
		return ret;

	blk_stats_record_write(blk, len);

	chunk->dirty = 0;

	return 0;
}

/*
 * Write all dirty chunks back to the device
 */
static int writebuffer_flush(struct block_device *blk)
{
	struct chunk *chunk;
	int ret;

	if (!IS_ENABLED(CONFIG_BLOCK_WRITE))
		return 0;

	list_for_each_entry(chunk, &blk->buffered_blocks, list) {
		ret = chunk_flush(blk, chunk);
		if (ret < 0)
			return ret;
	}

	if (blk->ops->flush)
		return blk->ops->flush(blk);

	return 0;
}

/*
 * get the chunk containing a given block. Will return NULL if the
 * block is not cached, the chunk otherwise.
 */
static struct chunk *chunk_get_cached(struct block_device *blk, sector_t block)
{
	struct chunk *chunk;

	list_for_each_entry(chunk, &blk->buffered_blocks, list) {
		if (block >= chunk->block_start &&
				block < chunk->block_start + blk->rdbufsize) {
			dev_vdbg(blk->dev, "%s: found %llu in %d\n", __func__,
				block, chunk->num);
			/*
			 * move most recently used entry to the head of the list
			 */
			list_move(&chunk->list, &blk->buffered_blocks);
			return chunk;
		}
	}

	return NULL;
}

/*
 * Get the data pointer for a given block. Will return NULL if
 * the block is not cached, the data pointer otherwise.
 */
static void *block_get_cached(struct block_device *blk, sector_t block)
{
	struct chunk *chunk;

	chunk = chunk_get_cached(blk, block);
	if (!chunk)
		return NULL;

	return chunk->data + (block - chunk->block_start) * BLOCKSIZE(blk);
}

/*
 * Get a data chunk, either from the idle list or if the idle list
 * is empty, the least recently used is written back to disk and
 * returned.
 */
static struct chunk *get_chunk(struct block_device *blk)
{
	struct chunk *chunk;
	int ret;

	if (list_empty(&blk->idle_blocks)) {
		/* use last entry which is the most unused */
		chunk = list_last_entry(&blk->buffered_blocks, struct chunk, list);
		ret = chunk_flush(blk, chunk);
		if (ret < 0)
			return ERR_PTR(ret);
	} else {
		chunk = list_first_entry(&blk->idle_blocks, struct chunk, list);
	}

	list_del(&chunk->list);

	return chunk;
}

/*
 * read a block into the cache. This assumes that the block is
 * not cached already. By definition block_get_cached() for
 * the same block will succeed after this call.
 */
static int block_cache(struct block_device *blk, sector_t block)
{
	struct chunk *chunk;
	size_t len;
	int ret;

	chunk = get_chunk(blk);
	if (IS_ERR(chunk))
		return PTR_ERR(chunk);

	chunk->block_start = block & ~blk->blkmask;

	dev_vdbg(blk->dev, "%s: %llu to %d\n", __func__, chunk->block_start,
		chunk->num);

	len = writebuffer_io_len(blk, chunk);
	if (chunk->block_start * BLOCKSIZE(blk) >= blk->discard_start &&
	    chunk->block_start * BLOCKSIZE(blk) + len
	    <= blk->discard_start + blk->discard_size) {
		memset(chunk->data, 0, len);
		list_add(&chunk->list, &blk->buffered_blocks);
		return 0;
	}

	ret = blk->ops->read(blk, chunk->data, chunk->block_start, len);
	if (ret) {
		list_add_tail(&chunk->list, &blk->idle_blocks);
		return ret;
	}

	blk_stats_record_read(blk, len);
	list_add(&chunk->list, &blk->buffered_blocks);

	return 0;
}

/*
 * Get the data for a block, either from the cache or from
 * the device.
 */
static void *block_get(struct block_device *blk, sector_t block)
{
	void *outdata;
	int ret;

	if (block >= blk->num_blocks)
		return ERR_PTR(-ENXIO);

	outdata = block_get_cached(blk, block);
	if (outdata)
		return outdata;

	ret = block_cache(blk, block);
	if (ret)
		return ERR_PTR(ret);

	outdata = block_get_cached(blk, block);
	if (!outdata)
		BUG();

	return outdata;
}

static ssize_t block_op_read(struct cdev *cdev, void *buf, size_t count,
		loff_t offset, unsigned long flags)
{
	struct block_device *blk = cdev->priv;
	unsigned long mask = BLOCKSIZE(blk) - 1;
	sector_t block = offset >> blk->blockbits;
	size_t icount = count;
	blkcnt_t blocks;

	if (offset & mask) {
		size_t now = BLOCKSIZE(blk) - (offset & mask);
		void *iobuf = block_get(blk, block);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		now = min(count, now);

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

/*
 * Put data into a block. This only overwrites the data in the
 * cache and marks the corresponding chunk as dirty.
 */
static int block_put(struct block_device *blk, const void *buf, sector_t block)
{
	struct chunk *chunk;
	void *data;

	if (block >= blk->num_blocks)
		return -EINVAL;

	data = block_get(blk, block);
	if (IS_ERR(data))
		return PTR_ERR(data);

	memcpy(data, buf, 1 << blk->blockbits);

	chunk = chunk_get_cached(blk, block);
	chunk->dirty = 1;

	return 0;
}

static ssize_t block_op_write(struct cdev *cdev, const void *buf, size_t count,
		loff_t offset, ulong flags)
{
	struct block_device *blk = cdev->priv;
	unsigned long mask = BLOCKSIZE(blk) - 1;
	sector_t block = offset >> blk->blockbits;
	size_t icount = count;
	blkcnt_t blocks;
	int ret;

	/*
	 * When the offset that is written to is within the first two
	 * LBAs then the partition table has changed, reparse the partition
	 * table at close time in this case. A GPT covers more space than
	 * only the first two LBAs, but a CRC of the remaining pieces is
	 * written to LBA1, so LBA1 must change as well when the partioning
	 * is changed.
	 */
	if (offset < 2 * SECTOR_SIZE)
		blk->need_reparse = true;

	if (offset & mask) {
		size_t now = BLOCKSIZE(blk) - (offset & mask);
		void *iobuf = block_get(blk, block);

		now = min(count, now);

		if (IS_ERR(iobuf))
			return PTR_ERR(iobuf);

		memcpy(iobuf + (offset & mask), buf, now);
		ret = block_put(blk, iobuf, block);
		if (ret)
			return ret;

		buf += now;
		count -= now;
		block++;
	}

	blocks = count >> blk->blockbits;

	while (blocks) {
		ret = block_put(blk, buf, block);
		if (ret)
			return ret;

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
		ret = block_put(blk, iobuf, block);
		if (ret)
			return ret;
	}

	return icount;
}
#endif

static int block_op_flush(struct cdev *cdev)
{
	struct block_device *blk = cdev->priv;

	blk->discard_start = blk->discard_size = 0;

	return writebuffer_flush(blk);
}

static int block_op_close(struct cdev *cdev)
{
	struct block_device *blk = cdev->priv;

	block_op_flush(cdev);

	if (blk->need_reparse) {
		reparse_partition_table(blk);
		blk->need_reparse = false;
	}

	return 0;
}

static int block_op_discard_range(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct block_device *blk = cdev->priv;

	blk->discard_start = offset;
	blk->discard_size = count;

	return 0;
}

static __maybe_unused int block_op_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct block_device *blk = cdev->priv;
	struct chunk *chunk, *tmp;
	int ret;

	if (!blk->ops->erase)
		return -EOPNOTSUPP;

	count >>= blk->blockbits;
	offset >>= blk->blockbits;

	list_for_each_entry_safe(chunk, tmp, &blk->buffered_blocks, list) {
		if (region_overlap_size(offset, count, chunk->block_start, blk->rdbufsize)) {
			ret = chunk_flush(blk, chunk);
			if (ret < 0)
				return ret;
			list_move(&chunk->list, &blk->idle_blocks);
		}
	}

	ret = blk->ops->erase(blk, offset, count);
	if (ret)
		return ret;

	blk_stats_record_erase(blk, count);

	return 0;
}

static struct cdev_operations block_ops = {
	.read	= block_op_read,
#ifdef CONFIG_BLOCK_WRITE
	.write	= block_op_write,
	.erase = block_op_erase,
#endif
	.close	= block_op_close,
	.flush	= block_op_flush,
	.discard_range = block_op_discard_range,
};

int blockdevice_register(struct block_device *blk)
{
	loff_t size = (loff_t)blk->num_blocks * BLOCKSIZE(blk);
	int ret;
	int i;

	blk->cdev.size = size;
	blk->cdev.dev = blk->dev;
	blk->cdev.ops = &block_ops;
	blk->cdev.priv = blk;
	blk->cdev.flags |= DEVFS_IS_BLOCK_DEV;
	blk->rdbufsize = BUFSIZE >> blk->blockbits;

	INIT_LIST_HEAD(&blk->buffered_blocks);
	INIT_LIST_HEAD(&blk->idle_blocks);
	blk->blkmask = blk->rdbufsize - 1;

	dev_dbg(blk->dev, "rdbufsize: %d blockbits: %d blkmask: 0x%08x\n",
		blk->rdbufsize, blk->blockbits, blk->blkmask);

	if (!blk->rdbufsize) {
		pr_warn("block size of %u not supported\n", BLOCKSIZE(blk));
		return -ENOSYS;
	}

	for (i = 0; i < 8; i++) {
		struct chunk *chunk = xzalloc(sizeof(*chunk));
		chunk->data = dma_alloc(BUFSIZE);
		chunk->num = i;
		list_add_tail(&chunk->list, &blk->idle_blocks);
	}

	/* TODO: We currently set this to ignore ERASE_TO_FLASH, but it could
	 * be useful to propagate the enum erase_type down into the erase
	 * callbacks and then map ERASE_TO_FLASH here to a discard operation.
	 * Before doing that though, we need to optimize the hardware drivers
	 * (currently eMMC) to erase bigger regions at once
	 */
	blk->cdev.flags |= DEVFS_WRITE_AUTOERASE;

	ret = devfs_create(&blk->cdev);
	if (ret)
		return ret;

	list_add_tail(&blk->list, &block_device_list);

	cdev_create_default_automount(&blk->cdev);

	/* Lack of partition table is unusual, but not a failure */
	(void)parse_partition_table(blk);

	return 0;
}

int blockdevice_unregister(struct block_device *blk)
{
	struct chunk *chunk, *tmp;

	writebuffer_flush(blk);

	list_for_each_entry_safe(chunk, tmp, &blk->buffered_blocks, list) {
		dma_free(chunk->data);
		free(chunk);
	}

	list_for_each_entry_safe(chunk, tmp, &blk->idle_blocks, list) {
		dma_free(chunk->data);
		free(chunk);
	}

	devfs_remove(&blk->cdev);
	list_del(&blk->list);

	return 0;
}

int block_read(struct block_device *blk, void *buf, sector_t block, blkcnt_t num_blocks)
{
	int ret;

	ret = cdev_read(&blk->cdev, buf,
			num_blocks << blk->blockbits,
			block << blk->blockbits, 0);

	return ret < 0 ? ret : 0;
}

int block_write(struct block_device *blk, void *buf, sector_t block, blkcnt_t num_blocks)
{
	int ret;

	ret = cdev_write(&blk->cdev, buf,
			num_blocks << blk->blockbits,
			block << blk->blockbits, 0);

	return ret < 0 ? ret : 0;
}

unsigned file_list_add_blockdevs(struct file_list *files)
{
	struct block_device *blk;
	unsigned count = 0;
	int err;

	list_for_each_entry(blk, &block_device_list, list) {
		err = file_list_add_cdev_entry(files, &blk->cdev, 0);
		if (!err)
			count++;
	}

	return count;
}

const char *blk_type_str(enum blk_type type)
{
	switch (type) {
	case BLK_TYPE_UNSPEC:
		return "unspecified";
	case BLK_TYPE_SD:
		return "SD";
	case BLK_TYPE_MMC:
		return "MMC";
	case BLK_TYPE_VIRTUAL:
		return "virtual";
	case BLK_TYPE_IDE:
		return "IDE";
	case BLK_TYPE_AHCI:
		return "AHCI";
	case BLK_TYPE_USB:
		return "USB";
	case BLK_TYPE_NVME:
		return "NVMe";
	default:
		return "unknown";
	}
}

char *cdev_get_linux_rootarg(const struct cdev *partcdev)
{
	const struct cdev *cdevm;
	struct block_device *blk;
	char *rootarg = NULL;

	if (!partcdev)
		return NULL;

	cdevm = partcdev->master ?: partcdev;
	blk = cdev_get_block_device(cdevm);
	if (!blk)
		return NULL;

	if (blk->ops->get_rootarg)
		rootarg = blk->ops->get_rootarg(blk, partcdev);
	if (!rootarg && partcdev->partuuid[0] != 0)
		rootarg = basprintf("root=PARTUUID=%s", partcdev->partuuid);

	if (IS_ENABLED(CONFIG_ROOTWAIT_BOOTARG) && blk->rootwait)
		rootarg = linux_bootargs_append_rootwait(rootarg);

	return rootarg;
}

