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
#include <linux/mtd/mtd-abi.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/list.h>

struct nand_bb {
	char cdevname[MAX_DRIVER_NAME];
	struct cdev *cdev_parent;
	char *name;
	int open;
	int needs_write;

	struct mtd_info_user info;

	loff_t raw_size;
	loff_t size;
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
	struct cdev *parent = bb->cdev_parent;
	int ret, bytes = 0, now;

	debug("%s 0x%08llx %d\n", __func__, offset, count);

	while(count) {
		ret = cdev_ioctl(parent, MEMGETBADBLOCK, &bb->offset);
		if (ret < 0)
			return ret;

		if (ret) {
			printf("skipping bad block at 0x%08llx\n", bb->offset);
			bb->offset += bb->info.erasesize;
			continue;
		}

		now = min(count, (size_t)(bb->info.erasesize -
				((size_t)bb->offset % bb->info.erasesize)));
		ret = cdev_read(parent, buf, now, bb->offset, 0);
		if (ret < 0)
			return ret;
		buf += now;
		count -= now;
		bb->offset += now;
		bytes += now;
	};

	return bytes;
}

/* Must be a multiple of the largest NAND page size */
#define BB_WRITEBUF_SIZE	4096

#ifdef CONFIG_MTD_WRITE
static int nand_bb_write_buf(struct nand_bb *bb, size_t count)
{
	int ret, now;
	struct cdev *parent = bb->cdev_parent;
	void *buf = bb->writebuf;
	loff_t cur_ofs = bb->offset & ~(BB_WRITEBUF_SIZE - 1);

	while (count) {
		ret = cdev_ioctl(parent, MEMGETBADBLOCK, &cur_ofs);
		if (ret < 0)
			return ret;

		if (ret) {
			debug("skipping bad block at 0x%08llx\n", cur_ofs);
			bb->offset += bb->info.erasesize;
			cur_ofs += bb->info.erasesize;
			continue;
		}

		now = min(count, (size_t)(bb->info.erasesize));
		ret = cdev_write(parent, buf, now, cur_ofs, 0);
		if (ret < 0)
			return ret;
		buf += now;
		count -= now;
		cur_ofs += now;
	};

	return 0;
}

static ssize_t nand_bb_write(struct cdev *cdev, const void *buf, size_t count,
	loff_t offset, ulong flags)
{
	struct nand_bb *bb = cdev->priv;
	int bytes = count, now, wroffs, ret;

	debug("%s offset: 0x%08llx count: 0x%08x\n", __func__, offset, count);

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

static int nand_bb_erase(struct cdev *cdev, size_t count, loff_t offset)
{
	struct nand_bb *bb = cdev->priv;

	if (offset != 0) {
		printf("can only erase from beginning of device\n");
		return -EINVAL;
	}

	return cdev_erase(bb->cdev_parent, bb->raw_size, 0);
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
	int ret;

	while (pos < bb->raw_size) {
		ret = cdev_ioctl(bb->cdev_parent, MEMGETBADBLOCK, &pos);
		if (ret < 0)
			return ret;
		if (!ret)
			bb->cdev.size += bb->info.erasesize;

		pos += bb->info.erasesize;
	}

	return 0;
}

static loff_t nand_bb_lseek(struct cdev *cdev, loff_t __offset)
{
	struct nand_bb *bb = cdev->priv;
	loff_t raw_pos = 0;
	uint32_t offset = __offset;
	int ret;

	/* lseek only in readonly mode */
	if (bb->flags & O_ACCMODE)
		return -ENOSYS;
	while (raw_pos < bb->raw_size) {
		off_t now = min(offset, bb->info.erasesize);

		ret = cdev_ioctl(bb->cdev_parent, MEMGETBADBLOCK, &raw_pos);
		if (ret < 0)
			return ret;
		if (!ret) {
			offset -= now;
			raw_pos += now;
		} else {
			raw_pos += bb->info.erasesize;
		}

		if (!offset) {
			bb->offset = raw_pos;
			return __offset;
		}
	}

	return -EINVAL;
}

static struct file_operations nand_bb_ops = {
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

/**
 * Add a bad block aware device ontop of another (NAND) device
 * @param[in] dev The device to add a partition on
 * @param[in] name Partition name (can be obtained with devinfo command)
 * @return The device representing the new partition.
 */
int dev_add_bb_dev(char *path, const char *name)
{
	struct nand_bb *bb;
	int ret = -ENOMEM;

	bb = xzalloc(sizeof(*bb));

	bb->cdev_parent = cdev_open(path, O_RDWR);
	if (!bb->cdev_parent)
		goto out1;

	if (name) {
		strcpy(bb->cdevname, name);
	} else {
		strcpy(bb->cdevname, path);
		strcat(bb->cdevname, ".bb");
	}

	bb->cdev.name = bb->cdevname;

	bb->raw_size = bb->cdev_parent->size;

	ret = cdev_ioctl(bb->cdev_parent, MEMGETINFO, &bb->info);
	if (ret)
		goto out4;

	nand_bb_calc_size(bb);
	bb->cdev.ops = &nand_bb_ops;
	bb->cdev.priv = bb;

	ret = devfs_create(&bb->cdev);
	if (ret)
		goto out4;

	list_add_tail(&bb->list, &bb_list);

	return 0;

out4:
	cdev_close(bb->cdev_parent);
out1:
	free(bb);
	return ret;
}

int dev_remove_bb_dev(const char *name)
{
	struct nand_bb *bb;

	list_for_each_entry(bb, &bb_list, list) {
		if (!strcmp(bb->cdev.name, name)) {
			devfs_remove(&bb->cdev);
			cdev_close(bb->cdev_parent);
			list_del_init(&bb->list);
			free(bb);
			return 0;
		}
	}

	return -ENODEV;
}
