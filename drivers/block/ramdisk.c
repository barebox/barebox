// SPDX-License-Identifier: GPL-2.0-only
// lightweight anonymous block device wrapper around memory buffers

#include <ramdisk.h>
#include <block.h>
#include <driver.h>
#include <fs.h>
#include <string.h>
#include <xfuncs.h>
#include <linux/align.h>
#include <linux/log2.h>

struct ramdisk {
	struct block_device blk;
	struct device dev;
	union {
		void *base_rw;
		const void *base_ro;
	};
	size_t size;
	bool ro;
};

static int ramdisk_check_read(const struct ramdisk *ramdisk)
{
	return ramdisk->base_ro ? 1 : 0;
}

static ssize_t ramdisk_read(struct cdev *cdev, void *buf, size_t count,
			    loff_t offset, unsigned long flags)
{
	struct ramdisk *ramdisk = cdev->priv;
	size_t cpy_count, pad_count;
	int ret;

	ret = ramdisk_check_read(ramdisk);
	if (ret < 1)
		return ret;

	if (size_add(offset, count) > ramdisk->size)
		return -ENXIO;

	cpy_count = min_t(size_t, count, ramdisk->size - offset);
	buf = mempcpy(buf, ramdisk->base_ro + offset, cpy_count);

	pad_count = min_t(size_t, count, cdev->size - offset) - cpy_count;

	memset(buf, 0x00, pad_count);

	pr_vdebug("read %zu bytes\n", cpy_count + pad_count);

	return cpy_count + pad_count;
}

static int ramdisk_check_write(const struct ramdisk *ramdisk)
{
	if (!ramdisk->base_rw)
		return -EPERM;
	if (ramdisk->ro)
		return -EACCES;
	return 1;
}

static ssize_t ramdisk_write(struct cdev *cdev, const void *buf, size_t count,
		  loff_t offset, unsigned long flags)
{
	struct ramdisk *ramdisk = cdev->priv;
	int ret;

	ret = ramdisk_check_write(ramdisk);
	if (ret < 1)
		return ret;
	if (size_add(offset, count) > ramdisk->size)
		return -ENXIO;

	memcpy(ramdisk->base_rw + offset, buf,
	       min_t(size_t, count, ramdisk->size - offset));

	return count;
}

static int ramdisk_memmap(struct cdev *cdev, void **map, int flags)
{
	struct ramdisk *ramdisk = cdev->priv;
	int ret;

	ret = flags & PROT_WRITE ? ramdisk_check_write(ramdisk)
			         : ramdisk_check_read(ramdisk);
	if (ret < 1)
		return ret;

	*map = ramdisk->base_rw;
	return 0;
}

static struct cdev_operations ramdisk_ops = {
	.read  = ramdisk_read,
	.write = ramdisk_write,
	.memmap = ramdisk_memmap,
};

struct ramdisk *ramdisk_init(int sector_size)
{
	struct ramdisk *ramdisk;
	struct block_device *blk;
	int ret;

	ramdisk = xzalloc(sizeof(*ramdisk));

	dev_set_name(&ramdisk->dev, "ramdisk");
	ramdisk->dev.id = DEVICE_ID_DYNAMIC;

	ret = register_device(&ramdisk->dev);
	if (ret)
		return NULL;

	blk = &ramdisk->blk;
	blk->dev = &ramdisk->dev;
	blk->type = BLK_TYPE_VIRTUAL;

	blk->cdev.size = 0;
	blk->cdev.name = xstrdup(dev_name(&ramdisk->dev));
	blk->cdev.dev = blk->dev;
	blk->cdev.ops = &ramdisk_ops;
	blk->cdev.priv = ramdisk;
	blk->cdev.flags |= DEVFS_IS_BLOCK_DEV;
	blk->blockbits = ilog2(sector_size);

	ret = devfs_create(&blk->cdev);
	if (ret)
		return NULL;

	INIT_LIST_HEAD(&blk->buffered_blocks);
	INIT_LIST_HEAD(&blk->idle_blocks);

	return ramdisk;
}

struct block_device *ramdisk_get_block_device(struct ramdisk *ramdisk)
{
	return &ramdisk->blk;
}

void ramdisk_free(struct ramdisk *ramdisk)
{
	devfs_remove(&ramdisk->blk.cdev);
	unregister_device(&ramdisk->dev);
	free(ramdisk);
}

static void ramdisk_setup_size(struct ramdisk *ramdisk, size_t size)
{
	ramdisk->size = size;
	ramdisk->blk.cdev.size = ALIGN(size, 1 << ramdisk->blk.blockbits);
}

void ramdisk_setup_ro(struct ramdisk *ramdisk, const void *data, size_t size)
{
	ramdisk->ro = true;
	ramdisk->base_ro = data;
	ramdisk_setup_size(ramdisk, size);
}

const void *ramdisk_mmap(struct ramdisk *ramdisk, loff_t offset,
			 size_t size)
{
	if (size_add(offset, size) > ramdisk->size)
		return NULL;

	return ramdisk->base_ro + offset;
}

void ramdisk_setup_rw(struct ramdisk *ramdisk, void *data, size_t size)
{
	ramdisk->ro = false;
	ramdisk->base_rw = data;
	ramdisk_setup_size(ramdisk, size);
}
