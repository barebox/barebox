/*
 * Copyright (c) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 */
#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <xfuncs.h>
#include <init.h>
#include <ioctl.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/list.h>
#include <linux/err.h>

struct nand_bb {
	char *name;
	int open;
	int needs_write;

	struct mtd_info *mtd;

	loff_t offset;
	unsigned long flags;
	void *writebuf;

	struct cdev cdev;

	struct list_head list;
};

static ssize_t nand_bb_read(struct cdev *cdev, void *buf, size_t count,
	loff_t offset, ulong flags)
{
	struct nand_bb *bb = cdev->priv;
	size_t retlen;
	int ret, bytes = 0, now;

	debug("%s offset: 0x%08llx (raw: 0x%08llx) count: 0x%08zx\n",
			__func__, offset, bb->offset, count);

	while (count) {
		loff_t max = bb->mtd->size - bb->offset;

		if (max <= 0)
			break;

		if (mtd_block_isbad(bb->mtd, bb->offset)) {
			printf("skipping bad block at 0x%08llx\n", bb->offset);
			bb->offset += bb->mtd->erasesize;
			continue;
		}

		now = min(count, (size_t)(bb->mtd->erasesize -
				((size_t)bb->offset % bb->mtd->erasesize)));

		if (now > max)
			now = max;

		ret = mtd_read(bb->mtd, bb->offset, now, &retlen, buf);
		if (ret < 0)
			return ret;
		buf += retlen;
		count -= retlen;
		bb->offset += retlen;
		bytes += retlen;
	};

	return bytes;
}

/* Must be a multiple of the largest NAND page size */
#define BB_WRITEBUF_SIZE	8192

#ifdef CONFIG_MTD_WRITE
static int nand_bb_write_buf(struct nand_bb *bb, size_t count)
{
	int ret, now;
	size_t retlen;
	void *buf = bb->writebuf;
	loff_t cur_ofs = bb->offset & ~(BB_WRITEBUF_SIZE - 1);

	while (count) {
		loff_t max = bb->mtd->size - cur_ofs;

		if (max <= 0)
			return -ENOSPC;

		if (mtd_block_isbad(bb->mtd, cur_ofs)) {
			debug("skipping bad block at 0x%08llx\n", cur_ofs);
			bb->offset += bb->mtd->erasesize;
			cur_ofs += bb->mtd->erasesize;
			continue;
		}

		now = min(count, (size_t)(bb->mtd->erasesize));
		if (now > max)
			return -ENOSPC;

		ret = mtd_write(bb->mtd, cur_ofs, now, &retlen, buf);
		if (ret < 0)
			return ret;
		buf += retlen;
		count -= retlen;
		cur_ofs += retlen;
	};

	return 0;
}

static ssize_t nand_bb_write(struct cdev *cdev, const void *buf, size_t count,
	loff_t offset, ulong flags)
{
	struct nand_bb *bb = cdev->priv;
	int bytes = count, now, wroffs, ret;

	debug("%s offset: 0x%08llx (raw: 0x%08llx) count: 0x%08zx\n",
			__func__, offset, bb->offset, count);

	while (count) {
		wroffs = bb->offset % BB_WRITEBUF_SIZE;
		now = min((int)count, BB_WRITEBUF_SIZE - wroffs);
		memcpy(bb->writebuf + wroffs, buf, now);

		if (wroffs + now == BB_WRITEBUF_SIZE) {
			bb->needs_write = 0;
			ret = nand_bb_write_buf(bb, BB_WRITEBUF_SIZE);
			if (ret)
				return ret;
		} else {
			bb->needs_write = 1;
		}

		bb->offset += now;
		count -= now;
		buf += now;
	}

	return bytes;
}

static int nand_bb_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct nand_bb *bb = cdev->priv;
	struct erase_info erase = {};
	int ret;

	if (offset != 0) {
		printf("can only erase from beginning of device\n");
		return -EINVAL;
	}

	while (count) {
		if (offset + bb->mtd->erasesize > bb->mtd->size)
			return 0;

		if (mtd_block_isbad(bb->mtd, offset)) {
			offset += bb->mtd->erasesize;
			continue;
		}

		erase.addr = offset;
		erase.len = bb->mtd->erasesize;

		ret = mtd_erase(bb->mtd, &erase);
		if (ret)
			return ret;

		offset += bb->mtd->erasesize;
		count -= bb->mtd->erasesize;
	}

	return 0;
}
#endif

static int nand_bb_open(struct cdev *cdev, unsigned long flags)
{
	struct nand_bb *bb = cdev->priv;

	if (bb->open)
		return -EBUSY;

	bb->flags = flags;
	bb->open = 1;
	bb->offset = 0;
	bb->needs_write = 0;
	bb->writebuf = xmalloc(BB_WRITEBUF_SIZE);

	return 0;
}

static int nand_bb_close(struct cdev *cdev)
{
	struct nand_bb *bb = cdev->priv;

#ifdef CONFIG_MTD_WRITE
	if (bb->needs_write)
		nand_bb_write_buf(bb, bb->offset % BB_WRITEBUF_SIZE);
#endif
	bb->open = 0;
	free(bb->writebuf);

	return 0;
}

static int nand_bb_calc_size(struct nand_bb *bb)
{
	loff_t pos = 0;

	while (pos < bb->mtd->size) {
		if (!mtd_block_isbad(bb->mtd, pos))
			bb->cdev.size += bb->mtd->erasesize;

		pos += bb->mtd->erasesize;
	}

	return 0;
}

static loff_t nand_bb_lseek(struct cdev *cdev, loff_t __offset)
{
	struct nand_bb *bb = cdev->priv;
	loff_t raw_pos = 0;
	uint32_t offset = __offset;

	/* lseek only in readonly mode */
	if (bb->flags & O_ACCMODE)
		return -ENOSYS;
	while (raw_pos < bb->mtd->size) {
		off_t now = min(offset, bb->mtd->erasesize);

		if (mtd_block_isbad(bb->mtd, raw_pos)) {
			raw_pos += bb->mtd->erasesize;
		} else {
			offset -= now;
			raw_pos += now;
		}

		if (!offset) {
			bb->offset = raw_pos;
			return __offset;
		}
	}

	return -EINVAL;
}

static struct cdev_operations nand_bb_ops = {
	.open   = nand_bb_open,
	.close  = nand_bb_close,
	.read  	= nand_bb_read,
	.lseek	= nand_bb_lseek,
#ifdef CONFIG_MTD_WRITE
	.write 	= nand_bb_write,
	.erase	= nand_bb_erase,
#endif
};

static LIST_HEAD(bb_list);

struct cdev *mtd_add_bb(struct mtd_info *mtd, const char *name)
{
	struct nand_bb *bb;
	int ret;

	bb = xzalloc(sizeof(*bb));
	bb->mtd = mtd;

	if (name)
		bb->cdev.name = xstrdup(name);
	else
		bb->cdev.name = basprintf("%s.bb", mtd->cdev.name);

	nand_bb_calc_size(bb);
	bb->cdev.ops = &nand_bb_ops;
	bb->cdev.priv = bb;

	ret = devfs_create(&bb->cdev);
	if (ret)
		goto err;

	list_add_tail(&bb->list, &bb_list);

	return &bb->cdev;

err:
	free(bb);
	return ERR_PTR(ret);
}

void mtd_del_bb(struct mtd_info *mtd)
{
	struct cdev *cdev = mtd->cdev_bb;
	struct nand_bb *bb = container_of(cdev, struct nand_bb, cdev);

	devfs_remove(&bb->cdev);
	list_del_init(&bb->list);
	free(bb->name);
	free(bb);

	mtd->cdev_bb = NULL;
}

/**
 * Add a bad block aware device ontop of another (NAND) device
 * @param[in] dev The device to add a partition on
 * @param[in] name Partition name (can be obtained with devinfo command)
 * @return The device representing the new partition.
 */
int dev_add_bb_dev(const char *path, const char *name)
{
	struct cdev *parent, *cdev;

	parent = cdev_by_name(path);
	if (!parent)
		return -ENODEV;

	if (!parent->mtd)
		return -EINVAL;

	cdev = mtd_add_bb(parent->mtd, name);

	return PTR_ERR(cdev);
}

int dev_remove_bb_dev(const char *name)
{
	struct nand_bb *bb, *tmp;

	list_for_each_entry_safe(bb, tmp, &bb_list, list) {
		if (!strcmp(bb->cdev.name, name)) {
			mtd_del_bb(bb->mtd);
			return 0;
		}
	}

	return -ENODEV;
}
