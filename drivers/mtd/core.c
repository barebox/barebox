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
#include <mtd/mtd-peb.h>
#include <mtd/ubi-user.h>
#include <cmdlinepart.h>
#include <filetype.h>
#include <init.h>
#include <xfuncs.h>
#include <driver.h>
#include <malloc.h>
#include <ioctl.h>
#include <nand.h>
#include <errno.h>
#include <of.h>

#include "mtd.h"

static LIST_HEAD(mtd_register_hooks);

/**
 * mtd_buf_all_ff - check if buffer contains only 0xff
 * @buf: buffer to check
 * @size: buffer size in bytes
 *
 * This function returns %1 if there are only 0xff bytes in @buf, and %0 if
 * something else was also found.
 */
int mtd_buf_all_ff(const void *buf, unsigned int len)
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

/**
 * mtd_buf_check_pattern - check if buffer contains only a certain byte pattern.
 * @buf: buffer to check
 * @patt: the pattern to check
 * @size: buffer size in bytes
 *
 * This function returns %1 in there are only @patt bytes in @buf, and %0 if
 * something else was also found.
 */
int mtd_buf_check_pattern(const void *buf, uint8_t patt, int size)
{
	int i;

	for (i = 0; i < size; i++)
		if (((const uint8_t *)buf)[i] != patt)
			return 0;
	return 1;
}

static ssize_t mtd_op_read(struct cdev *cdev, void* buf, size_t count,
			  loff_t offset, ulong flags)
{
	struct mtd_info *mtd = cdev->priv;
	size_t retlen;
	int ret;

	dev_dbg(cdev->dev, "read ofs: 0x%08llx count: 0x%08zx\n",
			offset, count);

	ret = mtd_read(mtd, offset, count, &retlen, buf);
	if (ret < 0 && ret != -EUCLEAN)
		return ret;

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

static int mtd_erase_align(struct mtd_info *mtd, loff_t *count, loff_t *offset)
{
	struct mtd_erase_region_info *e;
	loff_t ofs;

	if (mtd->numeraseregions == 0) {
		ofs = *offset & ~(loff_t)(mtd->erasesize - 1);
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

static int mtd_op_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct mtd_info *mtd = cdev->priv;
	struct erase_info erase;
	loff_t addr;
	int ret;

	ret = mtd_erase_align(mtd, &count, &offset);
	if (ret)
		return ret;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	addr = offset;

	if (!mtd->block_isbad) {
		erase.addr = addr;
		erase.len = count;
		return mtd_erase(mtd, &erase);
	}

	erase.len = mtd->erasesize;

	while (count > 0) {
		dev_dbg(cdev->dev, "erase 0x%08llx len: 0x%08llx\n", addr, erase.len);

		if (mtd->allow_erasebad || (mtd->master && mtd->master->allow_erasebad))
			ret = 0;
		else
			ret = mtd_block_isbad(mtd, addr);

		erase.addr = addr;

		if (ret > 0) {
			printf("Skipping bad block at 0x%08llx\n", addr);
		} else {
			ret = mtd_erase(mtd, &erase);
			if (ret)
				return ret;
		}

		addr += mtd->erasesize;
		count -= count > mtd->erasesize ? mtd->erasesize : count;
	}

	return 0;
}

static int mtd_op_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
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
	case MEMSETGOODBLOCK:
		dev_dbg(cdev->dev, "MEMSETGOODBLOCK: 0x%08llx\n", *offset);
		ret = mtd_block_markgood(mtd, *offset);
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
		user->subpagesize = mtd->writesize >> mtd->subpage_sft;
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

	if (ofs < 0 || ofs >= mtd->size)
		return -EINVAL;

	if (mtd->block_markbad)
		ret = mtd->block_markbad(mtd, ofs);
	else
		ret = -ENOSYS;

	return ret;
}

int mtd_block_markgood(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	if (mtd->block_markgood)
		ret = mtd->block_markgood(mtd, ofs);
	else
		ret = -ENOSYS;

	return ret;
}

int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
		u_char *buf)
{
	int ret_code;
	*retlen = 0;

	if (from < 0 || from >= mtd->size || len > mtd->size - from)
		return -EINVAL;
	if (!len)
		return 0;

	/*
	 * In the absence of an error, drivers return a non-negative integer
	 * representing the maximum number of bitflips that were corrected on
	 * any one ecc region (if applicable; zero otherwise).
	 */
	ret_code = mtd->read(mtd, from, len, retlen, buf);
	if (unlikely(ret_code < 0))
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
		const u_char *buf)
{
	*retlen = 0;

	if (to < 0 || to >= mtd->size || len > mtd->size - to)
		return -EINVAL;
	if (!mtd->write || !(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (!len)
		return 0;

	return mtd->write(mtd, to, len, retlen, buf);
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (instr->addr >= mtd->size || instr->len > mtd->size - instr->addr)
		return -EINVAL;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
	if (!instr->len) {
		instr->state = MTD_ERASE_DONE;
		mtd_erase_callback(instr);
		return 0;
	}
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

static int mtd_partition_set(struct device_d *dev, struct param_d *p, const char *val)
{
	struct mtd_info *mtd = container_of(dev, struct mtd_info, class_dev);
	struct mtd_info *mtdpart, *tmp;
	int ret;

	if (!val)
		return -EINVAL;

	list_for_each_entry_safe(mtdpart, tmp, &mtd->partitions, partitions_entry) {
		ret = mtd_del_partition(mtdpart);
		if (ret)
			return ret;
	}

	return cmdlinepart_do_parse(mtd->cdev.name, val, mtd->size, CMDLINEPART_ADD_DEVNAME);
}

static char *print_size(uint64_t s)
{
	if (!(s & ((1 << 20) - 1)))
		return basprintf("%lldM", s >> 20);
	if (!(s & ((1 << 10) - 1)))
		return basprintf("%lldk", s >> 10);
	return basprintf("0x%lld", s);
}

static int print_part(char *buf, int bufsize, struct mtd_info *mtd, uint64_t last_ofs,
		int is_last)
{
	char *size = print_size(mtd->size);
	char *ofs = print_size(mtd->master_offset);
	int ret;

	if (!size || !ofs) {
		ret = -ENOMEM;
		goto out;
	}

	if (mtd->master_offset == last_ofs)
		ret = snprintf(buf, bufsize, "%s(%s)%s", size,
				mtd->cdev.partname,
				is_last ? "" : ",");
	else
		ret = snprintf(buf, bufsize, "%s@%s(%s)%s", size,
				ofs,
				mtd->cdev.partname,
				is_last ? "" : ",");
out:
	free(size);
	free(ofs);

	return ret;
}

static int print_parts(char *buf, int bufsize, struct mtd_info *mtd)
{
	struct mtd_info *mtdpart;
	uint64_t last_ofs = 0;
	int ret = 0;

	list_for_each_entry(mtdpart, &mtd->partitions, partitions_entry) {
		int now;
		int is_last = list_is_last(&mtdpart->partitions_entry,
					&mtd->partitions);

		now = print_part(buf, bufsize, mtdpart, last_ofs, is_last);
		if (now < 0)
			return now;

		if (buf && bufsize) {
			buf += now;
			bufsize -= now;
		}
		ret += now;
		last_ofs = mtdpart->master_offset + mtdpart->size;
	}

	return ret;
}

static const char *mtd_partition_get(struct device_d *dev, struct param_d *p)
{
	struct mtd_info *mtd = container_of(dev, struct mtd_info, class_dev);
	int len = 0;

	free(p->value);

	len = print_parts(NULL, 0, mtd);
	p->value = xzalloc(len + 1);
	print_parts(p->value, len + 1, mtd);

	return p->value;
}

static int mtd_part_compare(struct list_head *a, struct list_head *b)
{
	struct mtd_info *mtda = container_of(a, struct mtd_info, partitions_entry);
	struct mtd_info *mtdb = container_of(b, struct mtd_info, partitions_entry);

	if (mtda->master_offset > mtdb->master_offset)
		return 1;
	if (mtda->master_offset < mtdb->master_offset)
		return -1;
	return 0;
}

static int mtd_detect(struct device_d *dev)
{
	struct mtd_info *mtd = container_of(dev, struct mtd_info, class_dev);
	int bufsize = 512;
	void *buf;
	int ret = 0, i;
	enum filetype filetype;
	int npebs = mtd_div_by_eb(mtd->size, mtd);

	npebs = max(npebs, 64);

	/*
	 * Do not try to attach an UBI device if this device has partitions
	 * as it's not a good idea to attach UBI on a raw device when the
	 * real UBI only spans the first partition.
	 */
	if (!list_empty(&mtd->partitions))
		return -EBUSY;

	buf = xmalloc(bufsize);

	for (i = 0; i < npebs; i++) {
		if (mtd_peb_is_bad(mtd, i))
			continue;

		ret = mtd_peb_read(mtd, buf, i, 0, 512);
		if (ret)
			continue;

		if (mtd_buf_all_ff(buf, 512))
			continue;

		filetype = file_detect_type(buf, bufsize);
		if (filetype == filetype_ubi) {
			ret = ubi_attach_mtd_dev(mtd, UBI_DEV_NUM_AUTO, 0, 20);
			if (ret == -EEXIST)
				ret = 0;
		}

		break;
	}

	free(buf);

	return ret;
}

int add_mtd_device(struct mtd_info *mtd, const char *devname, int device_id)
{
	struct mtddev_hook *hook;
	int ret;

	if (!devname)
		devname = "mtd";
	strcpy(mtd->class_dev.name, devname);
	mtd->class_dev.id = device_id;
	if (mtd->parent)
		mtd->class_dev.parent = mtd->parent;

	if (IS_ENABLED(CONFIG_MTD_UBI))
		mtd->class_dev.detect = mtd_detect;

	ret = register_device(&mtd->class_dev);
	if (ret)
		return ret;

	mtd->cdev.ops = &mtd_ops;
	mtd->cdev.size = mtd->size;
	if (device_id == DEVICE_ID_SINGLE)
		mtd->cdev.name = xstrdup(devname);
	else
		mtd->cdev.name = basprintf("%s%d", devname,
					     mtd->class_dev.id);

	INIT_LIST_HEAD(&mtd->partitions);

	mtd->cdev.priv = mtd;
	mtd->cdev.dev = &mtd->class_dev;
	mtd->cdev.mtd = mtd;

	if (IS_ENABLED(CONFIG_PARAMETER)) {
		dev_add_param_llint_ro(&mtd->class_dev, "size", mtd->size, "%llu");
		dev_add_param_int_ro(&mtd->class_dev, "erasesize", mtd->erasesize, "%u");
		dev_add_param_int_ro(&mtd->class_dev, "writesize", mtd->writesize, "%u");
		dev_add_param_int_ro(&mtd->class_dev, "oobsize", mtd->oobsize, "%u");
	}

	ret = devfs_create(&mtd->cdev);
	if (ret)
		goto err;

	if (mtd->master && !(mtd->cdev.flags & DEVFS_PARTITION_FIXED)) {
		struct mtd_info *mtdpart;

		list_for_each_entry(mtdpart, &mtd->master->partitions, partitions_entry) {
			if (mtdpart->master_offset + mtdpart->size <= mtd->master_offset)
				continue;
			if (mtd->master_offset + mtd->size <= mtdpart->master_offset)
				continue;
			dev_err(&mtd->class_dev, "New partition %s conflicts with %s\n",
					mtd->name, mtdpart->name);
			goto err1;
		}

		list_add_sort(&mtd->partitions_entry, &mtd->master->partitions, mtd_part_compare);
	}

	if (mtd_can_have_bb(mtd))
		mtd->cdev_bb = mtd_add_bb(mtd, NULL);

	if (mtd->parent && !mtd->master) {
		dev_add_param(&mtd->class_dev, "partitions", mtd_partition_set, mtd_partition_get, 0);
		of_parse_partitions(&mtd->cdev, mtd->parent->device_node);
		if (IS_ENABLED(CONFIG_OFDEVICE) && mtd->parent->device_node) {
			mtd->of_path = xstrdup(mtd->parent->device_node->full_name);
			ret = of_partitions_register_fixup(&mtd->cdev);
			if (ret)
				goto err1;
		}
	}

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->add_mtd_device)
			hook->add_mtd_device(mtd, devname, &hook->priv);

	return 0;
err1:
	devfs_remove(&mtd->cdev);
err:
	free(mtd->cdev.name);
	unregister_device(&mtd->class_dev);

	return ret;
}

int del_mtd_device (struct mtd_info *mtd)
{
	struct mtddev_hook *hook;

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->del_mtd_device)
			hook->del_mtd_device(mtd, &hook->priv);

	devfs_remove(&mtd->cdev);
	if (mtd->cdev_bb)
		mtd_del_bb(mtd);
	unregister_device(&mtd->class_dev);
	free(mtd->param_size.value);
	free(mtd->cdev.name);
	if (mtd->master)
		list_del(&mtd->partitions_entry);

	return 0;
}

void mtdcore_add_hook(struct mtddev_hook *hook)
{
	list_add(&hook->hook, &mtd_register_hooks);
}

const char *mtd_type_str(struct mtd_info *mtd)
{
	switch (mtd->type) {
	case MTD_ABSENT:
		return "absent";
	case MTD_RAM:
		return "ram";
	case MTD_ROM:
		return "rom";
	case MTD_NORFLASH:
		return "nor";
	case MTD_NANDFLASH:
		return "nand";
	case MTD_DATAFLASH:
		return"dataflash";
	case MTD_UBIVOLUME:
		return "ubi";
	default:
		return "unknown";
	}
}
