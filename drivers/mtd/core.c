/*
 * (C) Copyright 2005
 * 2N Telekomunikace, a.s. <www.2n.cz>
 * Ladislav Michl <michl@2n.cz>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <init.h>
#include <xfuncs.h>
#include <driver.h>
#include <malloc.h>
#include <ioctl.h>
#include <nand.h>
#include <errno.h>

#include "mtd.h"

static LIST_HEAD(mtd_register_hooks);

static 	ssize_t mtd_read(struct cdev *cdev, void* buf, size_t count,
			  loff_t _offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen;
	int ret;
	unsigned long offset = _offset;

	debug("mtd_read: 0x%08lx 0x%08x\n", offset, count);

	ret = mtd->read(mtd, offset, count, &retlen, buf);

	if(ret) {
		printf("err %d\n", ret);
		return ret;
	}
	return retlen;
}

#define NOTALIGNED(x) (x & (mtd->writesize - 1)) != 0
#define MTDPGALG(x) ((x) & ~(mtd->writesize - 1))

#ifdef CONFIG_MTD_WRITE
static int all_ff(const void *buf, int len)
{
	int i;
	const uint8_t *p = buf;

	for (i = 0; i < len; i++)
		if (p[i] != 0xFF)
			return 0;
	return 1;
}

static ssize_t mtd_write(struct cdev* cdev, const void *buf, size_t _count,
			  loff_t _offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen, now;
	int ret = 0;
	void *wrbuf = NULL;
	size_t count = _count;
	unsigned long offset = _offset;

	if (NOTALIGNED(offset)) {
		printf("offset 0x%0lx not page aligned\n", offset);
		return -EINVAL;
	}

	dev_dbg(cdev->dev, "write: 0x%08lx 0x%08x\n", offset, count);
	while (count) {
		now = count > mtd->writesize ? mtd->writesize : count;

		if (NOTALIGNED(now)) {
			dev_dbg(cdev->dev, "not aligned: %d %ld\n",
				mtd->writesize,
				(offset % mtd->writesize));
			wrbuf = xmalloc(mtd->writesize);
			memset(wrbuf, 0xff, mtd->writesize);
			memcpy(wrbuf + (offset % mtd->writesize), buf, now);
			if (!all_ff(wrbuf, mtd->writesize))
				ret = mtd->write(mtd, MTDPGALG(offset),
						  mtd->writesize, &retlen,
						  wrbuf);
			free(wrbuf);
		} else {
			if (!all_ff(buf, mtd->writesize))
				ret = mtd->write(mtd, offset, now, &retlen,
						  buf);
			dev_dbg(cdev->dev,
				"offset: 0x%08lx now: 0x%08x retlen: 0x%08x\n",
				offset, now, retlen);
		}
		if (ret)
			goto out;

		offset += now;
		count -= now;
		buf += now;
	}

out:
	return ret ? ret : _count;
}
#endif

int mtd_ioctl(struct cdev *cdev, int request, void *buf)
{
	int ret = 0;
	struct mtd_info *mtd = cdev->priv;
	struct mtd_info_user *user = buf;
#if (defined(CONFIG_NAND_ECC_HW) || defined(CONFIG_NAND_ECC_SOFT))
	struct mtd_ecc_stats *ecc = buf;
#endif
	struct region_info_user *reg = buf;
	loff_t *offset = buf;

	switch (request) {
	case MEMGETBADBLOCK:
		dev_dbg(cdev->dev, "MEMGETBADBLOCK: 0x%08llx\n", *offset);
		ret = mtd->block_isbad(mtd, *offset);
		break;
#ifdef CONFIG_MTD_WRITE
	case MEMSETBADBLOCK:
		dev_dbg(cdev->dev, "MEMSETBADBLOCK: 0x%08llx\n", *offset);
		ret = mtd->block_markbad(mtd, *offset);
		break;
#endif
	case MEMGETINFO:
		user->type	= mtd->type;
		user->flags	= mtd->flags;
		user->size	= mtd->size;
		user->erasesize	= mtd->erasesize;
		user->oobsize	= mtd->oobsize;
		user->mtd	= mtd;
		/* The below fields are obsolete */
		user->ecctype	= -1;
		user->eccsize	= 0;
		break;
#if (defined(CONFIG_NAND_ECC_HW) || defined(CONFIG_NAND_ECC_SOFT))
	case ECCGETSTATS:
		ecc->corrected = mtd->ecc_stats.corrected;
		ecc->failed = mtd->ecc_stats.failed;
		ecc->badblocks = mtd->ecc_stats.badblocks;
		ecc->bbtblocks = mtd->ecc_stats.bbtblocks;
		break;
#endif
	case MEMGETREGIONINFO:
		if (cdev->mtd) {
			unsigned long size = cdev->size;
			reg->offset = cdev->offset;
			reg->erasesize = cdev->mtd->erasesize;
			reg->numblocks = size / reg->erasesize;
			reg->regionindex = cdev->mtd->index;
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_MTD_WRITE
static ssize_t mtd_erase(struct cdev *cdev, size_t count, loff_t offset)
{
	struct mtd_info *mtd = cdev->priv;
	struct erase_info erase;
	int ret;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.addr = offset;
	erase.len = mtd->erasesize;

	while (count > 0) {
		dev_dbg(cdev->dev, "erase %d %d\n", erase.addr, erase.len);

		ret = mtd->block_isbad(mtd, erase.addr);
		if (ret > 0) {
			printf("Skipping bad block at 0x%08x\n", erase.addr);
		} else {
			ret = mtd->erase(mtd, &erase);
			if (ret)
				return ret;
		}

		erase.addr += mtd->erasesize;
		count -= count > mtd->erasesize ? mtd->erasesize : count;
	}

	return 0;
}
#endif

static struct file_operations mtd_ops = {
	.read   = mtd_read,
#ifdef CONFIG_MTD_WRITE
	.write  = mtd_write,
	.erase  = mtd_erase,
#endif
	.ioctl  = mtd_ioctl,
	.lseek  = dev_lseek_default,
};

int add_mtd_device(struct mtd_info *mtd, char *devname)
{
	char str[16];
	struct mtddev_hook *hook;

	if (!devname)
		devname = "mtd";
	strcpy(mtd->class_dev.name, devname);
	mtd->class_dev.id = DEVICE_ID_DYNAMIC;
	register_device(&mtd->class_dev);

	mtd->cdev.ops = &mtd_ops;
	mtd->cdev.size = mtd->size;
	mtd->cdev.name = asprintf("%s%d", devname, mtd->class_dev.id);
	mtd->cdev.priv = mtd;
	mtd->cdev.dev = &mtd->class_dev;
	mtd->cdev.mtd = mtd;

	if (IS_ENABLED(CONFIG_PARAMETER)) {
		sprintf(str, "%u", mtd->size);
		dev_add_param_fixed(&mtd->class_dev, "size", str);
		sprintf(str, "%u", mtd->erasesize);
		dev_add_param_fixed(&mtd->class_dev, "erasesize", str);
		sprintf(str, "%u", mtd->writesize);
		dev_add_param_fixed(&mtd->class_dev, "writesize", str);
		sprintf(str, "%u", mtd->oobsize);
		dev_add_param_fixed(&mtd->class_dev, "oobsize", str);
	}

	devfs_create(&mtd->cdev);

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->add_mtd_device)
			hook->add_mtd_device(mtd, devname);

	return 0;
}

int del_mtd_device (struct mtd_info *mtd)
{
	struct mtddev_hook *hook;

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->del_mtd_device)
			hook->del_mtd_device(mtd);
	unregister_device(&mtd->class_dev);
	free(mtd->param_size.value);
	free(mtd->cdev.name);
	return 0;
}

void mtdcore_add_hook(struct mtddev_hook *hook)
{
	list_add(&hook->hook, &mtd_register_hooks);
}
