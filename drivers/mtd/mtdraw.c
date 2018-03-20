/*
 * MTD raw device
 *
 * Copyright (C) 2011 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Adds a character devices :
 *  - mtdraw<N>
 *
 * Device mtd_raw<N> provides acces to the MTD "pages+OOB". For example if a MTD
 * has pages of 512 bytes and OOB of 16 bytes, mtd_oob<N> will be made of blocks
 * of 528 bytes, with page data being followed by OOB.
 * The layout will be: <page0> <oob0> <page1> <oob1> ... <pageN> <oobN>.
 * This means that a read at offset 516 of 20 bytes will give the 12 last bytes
 * of the OOB of page0, and the 8 first bytes of page1.
 * Same thing applies for writes, which have to be page+oob aligned (ie. offset
 * and size should be multiples of (mtd->writesize + mtd->oobsize)).
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <ioctl.h>
#include <errno.h>
#include <linux/mtd/mtd.h>

#include "mtd.h"

/* Must be a multiple of the largest NAND page size */
#define RAW_WRITEBUF_SIZE	4096

/**
 * mtdraw - mtdraw device private data
 * @cdev: character device "mtdraw<N>"
 * @mtd: MTD device to handle read/writes/erases
 *
 * @writebuf: buffer to handle unaligned writes (ie. writes of sizes which are
 * not multiples of MTD (writesize+oobsize)
 * @write_fill: number of bytes in writebuf
 * @write_ofs: offset in character device (mtdraw) where last write(s) stored
 * bytes because of unaligned writes (ie. remain of writesize+oobsize write)
 *
 * The mtdraw device must allow unaligned writes. This is enabled by a write buffer which gathers data to issue mtd->write_oob() with full page+oob data.
 * Suppose writesize=512, oobsize=16.
 * A first write of 512 bytes triggers:
 *  - write_ofs = offset of write()
 *  - write_fill = 512
 *  - copy of the 512 provided bytes into writebuf
 *  - no actual mtd->write if done
 * A second write of 512 bytes triggers:
 *  - copy of the 16 first bytes into writebuf
 *  - a mtd_write_oob() from writebuf
 *  - empty writebuf
 *  - copy the remaining 496 bytes into writebuf
 *    => write_fill = 496, write_ofs = offset + 528
 * Etc ...
 */
struct mtdraw {
	struct cdev cdev;
	struct mtd_info *mtd;
	void *writebuf;
	int write_fill;
	int write_ofs;
};

static struct mtdraw *to_mtdraw(struct cdev *cdev)
{
	return cdev->priv;
}

static struct mtd_info *to_mtd(struct cdev *cdev)
{
	struct mtdraw *mtdraw = to_mtdraw(cdev);
	return mtdraw->mtd;
}

static unsigned int mtdraw_offset_to_block(struct mtd_info *mtd, loff_t offset)
{
	u64 ofs64 = offset;

	do_div(ofs64, mtd->writesize + mtd->oobsize);

	return ofs64;
}

static ssize_t mtdraw_read_unaligned(struct mtd_info *mtd, void *dst,
				     size_t count, int skip, ulong offset)
{
	struct mtd_oob_ops ops;
	ssize_t ret;
	int partial = 0;
	void *tmp = dst;

	if (skip || count < mtd->writesize + mtd->oobsize)
		partial = 1;
	if (partial)
		tmp = malloc(mtd->writesize + mtd->oobsize);
	if (!tmp)
		return -ENOMEM;
	ops.mode = MTD_OPS_RAW;
	ops.ooboffs = 0;
	ops.datbuf = tmp;
	ops.len = mtd->writesize;
	ops.oobbuf = tmp + mtd->writesize;
	ops.ooblen = mtd->oobsize;
	ret = mtd_read_oob(mtd, offset, &ops);
	if (ret)
		goto err;
	if (partial)
		memcpy(dst, tmp + skip, count);
	ret = count;
err:
	if (partial)
		free(tmp);

	return ret;
}

static ssize_t mtdraw_read(struct cdev *cdev, void *buf, size_t count,
			    loff_t offset, ulong flags)
{
	struct mtd_info *mtd = to_mtd(cdev);
	ssize_t retlen = 0, ret = 1, toread;
	ulong numblock;
	int skip;

	numblock = mtdraw_offset_to_block(mtd, offset);
	skip = offset - numblock * (mtd->writesize + mtd->oobsize);

	while (ret > 0 && count > 0) {
		toread = min_t(int, count,
				mtd->writesize + mtd->oobsize - skip);
		ret = mtdraw_read_unaligned(mtd, buf, toread,
					    skip, numblock++ * mtd->writesize);
		buf += ret;
		skip = 0;
		count -= ret;
		retlen += ret;
	}
	if (ret < 0)
		printf("err %zd\n", ret);
	else
		ret = retlen;
	return ret;
}

#ifdef CONFIG_MTD_WRITE
static loff_t mtdraw_raw_to_mtd_offset(struct mtd_info *mtd, loff_t offset)
{
	return (loff_t)mtdraw_offset_to_block(mtd, offset) * mtd->writesize;
}

static ssize_t mtdraw_blkwrite(struct mtd_info *mtd, const void *buf,
			       ulong offset)
{
	struct mtd_oob_ops ops;
	int ret;

	ops.mode = MTD_OPS_RAW;
	ops.ooboffs = 0;
	ops.datbuf = (void *)buf;
	ops.len = mtd->writesize;
	ops.oobbuf = (void *)buf + mtd->writesize;
	ops.ooblen = mtd->oobsize;
	ret = mtd_write_oob(mtd, offset, &ops);
	if (!ret)
		ret = ops.retlen + ops.oobretlen;
	return ret;
}

static void mtdraw_fillbuf(struct mtdraw *mtdraw, const void *src, int nbbytes)
{
	memcpy(mtdraw->writebuf + mtdraw->write_fill, src, nbbytes);
	mtdraw->write_fill += nbbytes;
}

static ssize_t mtdraw_write(struct cdev *cdev, const void *buf, size_t count,
			    loff_t offset, ulong flags)
{
	struct mtdraw *mtdraw = to_mtdraw(cdev);
	struct mtd_info *mtd = to_mtd(cdev);
	int bsz = mtd->writesize + mtd->oobsize;
	ulong numblock;
	size_t retlen = 0, tofill;
	int ret = 0;

	numblock = mtdraw_offset_to_block(mtd, offset);

	if (mtdraw->write_fill &&
	    mtdraw->write_ofs + mtdraw->write_fill != offset)
		return -EINVAL;
	if (mtdraw->write_fill == 0 && offset - numblock * mtd->writesize != 0)
		return -EINVAL;

	if (mtdraw->write_fill) {
		tofill = min_t(size_t, count, bsz - mtdraw->write_fill);
		mtdraw_fillbuf(mtdraw, buf, tofill);
		offset += tofill;
		count -= tofill;
		retlen += tofill;
	}

	if (mtdraw->write_fill == bsz) {
		numblock = mtdraw_offset_to_block(mtd, mtdraw->write_ofs);
		ret = mtdraw_blkwrite(mtd, mtdraw->writebuf,
				      mtd->writesize * numblock);
		mtdraw->write_fill = 0;
	}

	numblock = mtdraw_offset_to_block(mtd, offset);
	while (ret >= 0 && count >= bsz) {
		ret = mtdraw_blkwrite(mtd, buf + retlen,
				   mtd->writesize * numblock++);
		count -= ret;
		retlen += ret;
		offset += ret;
	}

	if (ret >= 0 && count) {
		mtdraw->write_ofs = offset - mtdraw->write_fill;
		mtdraw_fillbuf(mtdraw, buf + retlen, count);
		retlen += count;
	}

	if (ret < 0) {
		printf("err %d\n", ret);
		return ret;
	} else {
		return retlen;
	}
}

static int mtdraw_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct mtd_info *mtd = to_mtd(cdev);
	struct erase_info erase;
	int ret;

	offset = mtdraw_raw_to_mtd_offset(mtd, offset);
	count = mtdraw_raw_to_mtd_offset(mtd, count);

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.addr = offset;
	erase.len = mtd->erasesize;

	while (count > 0) {
		debug("erase 0x%08llx len: 0x%08llx\n", erase.addr, erase.len);

		if (!mtd->allow_erasebad)
			ret = mtd_block_isbad(mtd, erase.addr);
		else
			ret = 0;

		if (ret > 0) {
			printf("Skipping bad block at 0x%08llx\n", erase.addr);
		} else {
			ret = mtd_erase(mtd, &erase);
			if (ret)
				return ret;
		}

		erase.addr += mtd->erasesize;
		count -= count > mtd->erasesize ? mtd->erasesize : count;
	}

	return 0;
}
#else
static ssize_t mtdraw_write(struct cdev *cdev, const void *buf, size_t count,
			    loff_t offset, ulong flags)
{
	return 0;
}
static ssize_t mtdraw_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	return 0;
}
#endif

static const struct cdev_operations mtd_raw_fops = {
	.read		= mtdraw_read,
	.write		= mtdraw_write,
	.erase		= mtdraw_erase,
	.lseek		= dev_lseek_default,
};

static int add_mtdraw_device(struct mtd_info *mtd, const char *devname, void **priv)
{
	struct mtdraw *mtdraw;

	if (mtd->master || mtd->oobsize == 0)
		return 0;

	mtdraw = xzalloc(sizeof(*mtdraw));
	mtdraw->writebuf = xmalloc(RAW_WRITEBUF_SIZE);
	mtdraw->mtd = mtd;

	mtdraw->cdev.ops = (struct cdev_operations *)&mtd_raw_fops;
	mtdraw->cdev.size = mtd_div_by_wb(mtd->size, mtd) *
		(mtd->writesize + mtd->oobsize);
	mtdraw->cdev.name = basprintf("%s.raw", mtd->cdev.name);
	mtdraw->cdev.priv = mtdraw;
	mtdraw->cdev.dev = &mtd->class_dev;
	mtdraw->cdev.mtd = mtd;
	*priv = mtdraw;
	devfs_create(&mtdraw->cdev);

	return 0;
}

static int del_mtdraw_device(struct mtd_info *mtd, void **priv)
{
	struct mtdraw *mtdraw;

	if (mtd->master || mtd->oobsize == 0)
		return 0;

	mtdraw = *priv;
	devfs_remove(&mtdraw->cdev);
	free(mtdraw);

	return 0;
}

static struct mtddev_hook mtdraw_hook = {
	.add_mtd_device = add_mtdraw_device,
	.del_mtd_device = del_mtdraw_device,
};

static int __init register_mtdraw(void)
{
	mtdcore_add_hook(&mtdraw_hook);
	return 0;
}

coredevice_initcall(register_mtdraw);
