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

int mtd_all_ff(const void *buf, unsigned int len)
{
	while ((unsigned long)buf & 0x3) {
		if (*(const uint8_t *)buf != 0xff)
			return 0;
		len--;
		if (!len)
			return 1;
		buf++;
	}

	while (len > 0x3) {
		if (*(const uint32_t *)buf != 0xffffffff)
			return 0;

		len -= sizeof(uint32_t);
		if (!len)
			return 1;

		buf += sizeof(uint32_t);
	}

	while (len) {
		if (*(const uint8_t *)buf != 0xff)
			return 0;
		len--;
		buf++;
	}

	return 1;
}

static ssize_t mtd_op_read(struct cdev *cdev, void* buf, size_t count,
			  loff_t _offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen;
	int ret;
	unsigned long offset = _offset;

	dev_dbg(cdev->dev, "read ofs: 0x%08lx count: 0x%08zx\n",
			offset, count);

	ret = mtd_read(mtd, offset, count, &retlen, buf);

	if(ret) {
		printf("err %d\n", ret);
		return ret;
	}
	return retlen;
}

#define NOTALIGNED(x) (x & (mtd->writesize - 1)) != 0
#define MTDPGALG(x) ((x) & ~(mtd->writesize - 1))

#ifdef CONFIG_MTD_WRITE
static ssize_t mtd_op_write(struct cdev* cdev, const void *buf, size_t _count,
			  loff_t _offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen;
	int ret;

	ret = mtd_write(mtd, _offset, _count, &retlen, buf);

	return ret ? ret : _count;
}

static struct mtd_erase_region_info *mtd_find_erase_region(struct mtd_info *mtd, loff_t offset)
{
	int i;

	for (i = 0; i < mtd->numeraseregions; i++) {
		struct mtd_erase_region_info *e = &mtd->eraseregions[i];
		if (offset > e->offset + e->erasesize * e->numblocks)
			continue;
		return e;
	}

	return NULL;
}

static int mtd_erase_align(struct mtd_info *mtd, size_t *count, loff_t *offset)
{
	struct mtd_erase_region_info *e;
	loff_t ofs;

	if (mtd->numeraseregions == 0) {
		ofs = *offset & ~(mtd->erasesize - 1);
		*count += (*offset - ofs);
		*count = ALIGN(*count, mtd->erasesize);
		*offset = ofs;
		return 0;
	}

	e = mtd_find_erase_region(mtd, *offset);
	if (!e)
		return -EINVAL;

	ofs = *offset & ~(e->erasesize - 1);
	*count += (*offset - ofs);

	e = mtd_find_erase_region(mtd, *offset + *count);
	if (!e)
		return -EINVAL;

	*count = ALIGN(*count, e->erasesize);
	*offset = ofs;

	return 0;
}

static int mtd_op_erase(struct cdev *cdev, size_t count, loff_t offset)
{
	struct mtd_info *mtd = cdev->priv;
	struct erase_info erase;
	int ret;

	ret = mtd_erase_align(mtd, &count, &offset);
	if (ret)
		return ret;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	erase.addr = offset;

	if (!mtd->block_isbad) {
		erase.len = count;
		return mtd_erase(mtd, &erase);
	}

	erase.len = mtd->erasesize;

	while (count > 0) {
		dev_dbg(cdev->dev, "erase %d %d\n", erase.addr, erase.len);

		if (!mtd->allow_erasebad)
			ret = mtd_block_isbad(mtd, erase.addr);
		else
			ret = 0;

		if (ret > 0) {
			printf("Skipping bad block at 0x%08x\n", erase.addr);
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

static ssize_t mtd_op_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
{
	struct mtd_info *mtd = cdev->priv;

	if (!mtd->unlock || !mtd->lock)
		return -ENOSYS;

	if (prot)
		return mtd_lock(mtd, offset, count);
	else
		return mtd_unlock(mtd, offset, count);
}

#endif /* CONFIG_MTD_WRITE */

int mtd_ioctl(struct cdev *cdev, int request, void *buf)
{
	int ret = 0;
	struct mtd_info *mtd = cdev->priv;
	struct mtd_info_user *user = buf;
#if (defined(CONFIG_NAND_ECC_HW) || defined(CONFIG_NAND_ECC_SOFT))
	struct mtd_ecc_stats *ecc = buf;
#endif
	struct region_info_user *reg = buf;
#ifdef CONFIG_MTD_WRITE
	struct erase_info_user *ei = buf;
#endif
	loff_t *offset = buf;

	switch (request) {
	case MEMGETBADBLOCK:
		dev_dbg(cdev->dev, "MEMGETBADBLOCK: 0x%08llx\n", *offset);
		ret = mtd_block_isbad(mtd, *offset);
		break;
#ifdef CONFIG_MTD_WRITE
	case MEMSETBADBLOCK:
		dev_dbg(cdev->dev, "MEMSETBADBLOCK: 0x%08llx\n", *offset);
		ret = mtd_block_markbad(mtd, *offset);
		break;
	case MEMERASE:
		ret = mtd_op_erase(cdev, ei->length, ei->start + cdev->offset);
		break;
#endif
	case MEMGETINFO:
		user->type	= mtd->type;
		user->flags	= mtd->flags;
		user->size	= mtd->size;
		user->erasesize	= mtd->erasesize;
		user->writesize	= mtd->writesize;
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

int mtd_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	if (!mtd->lock)
		return -EOPNOTSUPP;
	if (ofs < 0 || ofs > mtd->size || len > mtd->size - ofs)
		return -EINVAL;
	if (!len)
		return 0;
	return mtd->lock(mtd, ofs, len);
}

int mtd_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	if (!mtd->unlock)
		return -EOPNOTSUPP;
	if (ofs < 0 || ofs > mtd->size || len > mtd->size - ofs)
		return -EINVAL;
	if (!len)
		return 0;
	return mtd->unlock(mtd, ofs, len);
}

int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	if (!mtd->block_isbad)
		return 0;

	if (ofs < 0 || ofs > mtd->size)
		return -EINVAL;

	return mtd->block_isbad(mtd, ofs);
}

int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	if (mtd->block_markbad)
		ret = mtd->block_markbad(mtd, ofs);
	else
		ret = -ENOSYS;

	return ret;
}

int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
		u_char *buf)
{
	return mtd->read(mtd, from, len, retlen, buf);
}

int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
		const u_char *buf)
{
	return mtd->write(mtd, to, len, retlen, buf);
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return mtd->erase(mtd, instr);
}

int mtd_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	int ret_code;

	ops->retlen = ops->oobretlen = 0;
	if (!mtd->read_oob)
		return -EOPNOTSUPP;
	/*
	 * In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
	 * similar to mtd->_read(), returning a non-negative integer
	 * representing max bitflips. In other cases, mtd->_read_oob() may
	 * return -EUCLEAN. In all cases, perform similar logic to mtd_read().
	 */
	ret_code = mtd->read_oob(mtd, from, ops);
	if (unlikely(ret_code < 0))
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

static struct file_operations mtd_ops = {
	.read   = mtd_op_read,
#ifdef CONFIG_MTD_WRITE
	.write  = mtd_op_write,
	.erase  = mtd_op_erase,
	.protect = mtd_op_protect,
#endif
	.ioctl  = mtd_ioctl,
	.lseek  = dev_lseek_default,
};

int add_mtd_device(struct mtd_info *mtd, char *devname)
{
	struct mtddev_hook *hook;

	if (!devname)
		devname = "mtd";
	strcpy(mtd->class_dev.name, devname);
	mtd->class_dev.id = DEVICE_ID_DYNAMIC;
	if (mtd->parent)
		mtd->class_dev.parent = mtd->parent;
	register_device(&mtd->class_dev);

	mtd->cdev.ops = &mtd_ops;
	mtd->cdev.size = mtd->size;
	mtd->cdev.name = asprintf("%s%d", devname, mtd->class_dev.id);
	mtd->cdev.priv = mtd;
	mtd->cdev.dev = &mtd->class_dev;
	mtd->cdev.mtd = mtd;

	if (IS_ENABLED(CONFIG_PARAMETER)) {
		dev_add_param_int_ro(&mtd->class_dev, "size", mtd->size, "%u");
		dev_add_param_int_ro(&mtd->class_dev, "erasesize", mtd->erasesize, "%u");
		dev_add_param_int_ro(&mtd->class_dev, "writesize", mtd->oobsize, "%u");
		dev_add_param_int_ro(&mtd->class_dev, "oobsize", mtd->oobsize, "%u");
	}

	devfs_create(&mtd->cdev);
	of_parse_partitions(&mtd->cdev, mtd->parent->device_node);

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->add_mtd_device)
			hook->add_mtd_device(mtd, devname, &hook->priv);

	return 0;
}

int del_mtd_device (struct mtd_info *mtd)
{
	struct mtddev_hook *hook;

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->del_mtd_device)
			hook->del_mtd_device(mtd, &hook->priv);

	devfs_remove(&mtd->cdev);
	unregister_device(&mtd->class_dev);
	free(mtd->param_size.value);
	free(mtd->cdev.name);
	return 0;
}

void mtdcore_add_hook(struct mtddev_hook *hook)
{
	list_add(&hook->hook, &mtd_register_hooks);
}
