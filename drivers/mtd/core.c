// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2005
 * 2N Telekomunikace, a.s. <www.2n.cz>
 * Ladislav Michl <michl@2n.cz>
 */
#include <common.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
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
		if (offset > e->offset + (loff_t)e->erasesize * e->numblocks)
			continue;
		return e;
	}

	return NULL;
}

static loff_t __mtd_erase_round(loff_t x, uint32_t esize, int up)
{
	uint64_t dividend = x;
	uint32_t mod = do_div(dividend, esize);
	if (mod == 0)
		return x;
	if (up)
		x += esize;
	return x - mod;
}
#define mtd_erase_round_up(x, esize)	__mtd_erase_round(x, esize, 1)
#define mtd_erase_round_down(x, esize)	__mtd_erase_round(x, esize, 0)

static int mtd_erase_align(struct mtd_info *mtd, loff_t *count, loff_t *offset)
{
	struct mtd_erase_region_info *e;
	loff_t ofs;

	if (mtd->numeraseregions == 0) {
		ofs = mtd_erase_round_down(*offset, mtd->erasesize);
		*count += *offset - ofs;
		*count = mtd_erase_round_up(*count, mtd->erasesize);
		*offset = ofs;
		return 0;
	}

	e = mtd_find_erase_region(mtd, *offset);
	if (!e)
		return -EINVAL;

	ofs = mtd_erase_round_down(*offset, e->erasesize);
	*count += *offset - ofs;

	e = mtd_find_erase_region(mtd, *offset + *count);
	if (!e)
		return -EINVAL;

	*count = mtd_erase_round_up(*count, e->erasesize);
	*offset = ofs;

	return 0;
}

static int mtd_op_erase(struct cdev *cdev, loff_t count, loff_t offset)
{
	struct mtd_info *mtd = cdev->priv;
	struct erase_info erase;
	loff_t addr;
	int ret;

	if (mtd->flags & MTD_NO_ERASE)
		return -EOPNOTSUPP;

	ret = mtd_erase_align(mtd, &count, &offset);
	if (ret)
		return ret;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = mtd;
	addr = offset;

	if (!mtd->_block_isbad) {
		erase.addr = addr;
		erase.len = count;
		return mtd_erase(mtd, &erase);
	}

	erase.len = mtd->erasesize;

	while (count > 0) {
		dev_dbg(cdev->dev, "erase 0x%08llx len: 0x%08llx\n", addr, erase.len);

		if (mtd->allow_erasebad || (mtd->parent && mtd->parent->allow_erasebad))
			ret = 0;
		else
			ret = mtd_block_isbad(mtd, addr);

		erase.addr = addr;

		if (ret > 0) {
			printf("Skipping bad block at 0x%08llx\n", addr);
		} else {
			ret = mtd_erase(mtd, &erase);
			if (ret) {
				printf("%s: failed to erase block at 0x%08llx\n",
					__func__, addr);
				return ret;
			}
		}

		addr += mtd->erasesize;
		count -= count > mtd->erasesize ? mtd->erasesize : count;
	}

	return 0;
}

static int mtd_op_protect(struct cdev *cdev, size_t count, loff_t offset, int prot)
{
	struct mtd_info *mtd = cdev->priv;

	if (!mtd->_unlock || !mtd->_lock)
		return -ENOSYS;

	if (prot)
		return mtd_lock(mtd, offset, count);
	else
		return mtd_unlock(mtd, offset, count);
}

#endif /* CONFIG_MTD_WRITE */

int mtd_ioctl(struct cdev *cdev, unsigned int request, void *buf)
{
	int ret = 0;
	struct mtd_info *mtd = cdev->priv;
	struct mtd_info_user *user = buf;
	struct mtd_ecc_stats *ecc = buf;
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
	case ECCGETSTATS:
		ecc->corrected = mtd->ecc_stats.corrected;
		ecc->failed = mtd->ecc_stats.failed;
		ecc->badblocks = mtd->ecc_stats.badblocks;
		ecc->bbtblocks = mtd->ecc_stats.bbtblocks;
		break;
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
	if (!mtd->_lock)
		return -EOPNOTSUPP;
	if (ofs < 0 || ofs > mtd->size || len > mtd->size - ofs)
		return -EINVAL;
	if (!len)
		return 0;
	return mtd->_lock(mtd, ofs, len);
}

int mtd_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	if (!mtd->_unlock)
		return -EOPNOTSUPP;
	if (ofs < 0 || ofs > mtd->size || len > mtd->size - ofs)
		return -EINVAL;
	if (!len)
		return 0;
	return mtd->_unlock(mtd, ofs, len);
}

int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	if (!mtd->_block_isbad)
		return 0;

	if (ofs < 0 || ofs > mtd->size)
		return -EINVAL;

	return mtd->_block_isbad(mtd, ofs);
}

int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	if (ofs < 0 || ofs >= mtd->size)
		return -EINVAL;

	if (mtd->_block_markbad)
		ret = mtd->_block_markbad(mtd, ofs);
	else
		ret = -ENOSYS;

	return ret;
}

int mtd_block_markgood(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	if (mtd->_block_markgood)
		ret = mtd->_block_markgood(mtd, ofs);
	else
		ret = -ENOSYS;

	return ret;
}

int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
	     u_char *buf)
{
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = buf,
	};
	int ret;

	if (from < 0 || from >= mtd->size || len > mtd->size - from)
		return -EINVAL;
	if (!len)
		return 0;

	ret = mtd_read_oob(mtd, from, &ops);
	*retlen = ops.retlen;

	return ret;
}

int mtd_write_oob(struct mtd_info *mtd, loff_t to,
				struct mtd_oob_ops *ops)
{
	int ret;

	ops->retlen = ops->oobretlen = 0;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	/* Check the validity of a potential fallback on mtd->_write */
	if (!mtd->_write_oob && (!mtd->_write || ops->oobbuf))
		return -EOPNOTSUPP;

	if (mtd->_write_oob)
		ret = mtd->_write_oob(mtd, to, ops);
	else
		ret = mtd->_write(mtd, to, ops->len, &ops->retlen,
				     ops->datbuf);

	return ret;
}

int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
	      const u_char *buf)
{
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = (u8 *)buf,
	};
	int ret;

	if (to < 0 || to >= mtd->size || len > mtd->size - to)
		return -EINVAL;
	if (!len)
		return 0;

	ret = mtd_write_oob(mtd, to, &ops);
	*retlen = ops.retlen;

	return ret;
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (instr->addr >= mtd->size || instr->len > mtd->size - instr->addr)
		return -EINVAL;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
	if (!instr->len)
		return 0;

	return mtd->_erase(mtd, instr);
}

int mtd_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	int ret_code;

	ops->retlen = ops->oobretlen = 0;

	/* Check the validity of a potential fallback on mtd->_read */
	if (!mtd->_read_oob && (!mtd->_read || ops->oobbuf))
		return -EOPNOTSUPP;

	/*
	 * In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
	 * similar to mtd->_read(), returning a non-negative integer
	 * representing max bitflips. In other cases, mtd->_read_oob() may
	 * return -EUCLEAN. In all cases, perform similar logic to mtd_read().
	 */
	if (mtd->_read_oob)
		ret_code = mtd->_read_oob(mtd, from, ops);
	else
		ret_code = mtd->_read(mtd, from, ops->len, &ops->retlen,
				    ops->datbuf);

	if (unlikely(ret_code < 0))
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

static struct cdev_operations mtd_ops = {
	.read   = mtd_op_read,
#ifdef CONFIG_MTD_WRITE
	.write  = mtd_op_write,
	.erase  = mtd_op_erase,
	.protect = mtd_op_protect,
#endif
	.ioctl  = mtd_ioctl,
};

static int mtd_partition_set(struct param_d *p, void *priv)
{
	struct mtd_info *mtd = priv;
	struct mtd_info *mtdpart, *tmp;
	int ret;

	if (!mtd->partition_string)
		return -EINVAL;

	list_for_each_entry_safe(mtdpart, tmp, &mtd->partitions, partitions_entry) {
		ret = mtd_del_partition(mtdpart);
		if (ret)
			return ret;
	}

	return cmdlinepart_do_parse(mtd->cdev.name, mtd->partition_string, mtd->size, CMDLINEPART_ADD_DEVNAME);
}

static char *print_size(uint64_t s)
{
	if (!(s & ((1 << 20) - 1)))
		return basprintf("%lldM", s >> 20);
	if (!(s & ((1 << 10) - 1)))
		return basprintf("%lldk", s >> 10);
	return basprintf("0x%llx", s);
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

static int mtd_partition_get(struct param_d *p, void *priv)
{
	struct mtd_info *mtd = priv;
	int len = 0;

	free(mtd->partition_string);

	len = print_parts(NULL, 0, mtd);
	mtd->partition_string = xzalloc(len + 1);
	print_parts(mtd->partition_string, len + 1, mtd);

	return 0;
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

static int mtd_detect(struct device *dev)
{
	struct mtd_info *mtd = container_of(dev, struct mtd_info, dev);
	int bufsize = 512;
	void *buf;
	int ret = 0, i;
	enum filetype filetype;
	int npebs = mtd_div_by_eb(mtd->size, mtd);

	/* No point in looking for UBI on a partition that's too small */
	npebs = min(npebs, 64);
	if (npebs < 5)
		return 0;

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
		if (filetype == filetype_ubi && ubi_num_get_by_mtd(mtd) < 0)
			ret = ubi_attach_mtd_dev(mtd, UBI_DEV_NUM_AUTO, 0, 20);

		break;
	}

	free(buf);

	return ret;
}

static int mtd_partition_fixup_generic(struct mtd_info *mtd, struct device_node *root)
{
	struct cdev *cdev = &mtd->cdev;
	struct device_node *np, *mtdnp = mtd_get_of_node(mtd);
	char *name;

	name = of_get_reproducible_name(mtdnp);
	np = of_find_node_by_reproducible_name(root, name);
	free(name);
	if (!np) {
		dev_err(&mtd->dev, "Cannot find nodepath %pOF, cannot fixup\n",
				mtdnp);
		return -EINVAL;
	}

	return of_fixup_partitions(np, cdev);
}

static int mtd_partition_fixup(struct device_node *root, void *ctx)
{
	struct mtd_info *mtd = ctx;

	if (mtd->of_fixup)
		return mtd->of_fixup(mtd, root);

	return mtd_partition_fixup_generic(mtd, root);
}

int add_mtd_device(struct mtd_info *mtd, const char *devname, int device_id)
{
	struct mtddev_hook *hook;
	int ret;

	if (!devname)
		devname = "mtd";
	dev_set_name(&mtd->dev, devname);
	mtd->dev.id = device_id;

	if (IS_ENABLED(CONFIG_MTD_UBI))
		mtd->dev.detect = mtd_detect;

	ret = register_device(&mtd->dev);
	if (ret)
		return ret;

	mtd->cdev.ops = &mtd_ops;
	mtd->cdev.size = mtd->size;
	if (device_id == DEVICE_ID_SINGLE)
		mtd->cdev.name = xstrdup(devname);
	else
		mtd->cdev.name = basprintf("%s%d", devname,
					     mtd->dev.id);

	INIT_LIST_HEAD(&mtd->partitions);
	INIT_LIST_HEAD(&mtd->partitions_entry);

	mtd->cdev.priv = mtd;
	mtd->cdev.dev = &mtd->dev;
	mtd->cdev.mtd = mtd;

	if (IS_ENABLED(CONFIG_PARAMETER)) {
		dev_add_param_uint64_ro(&mtd->dev, "size", &mtd->size, "%llu");
		dev_add_param_uint32_ro(&mtd->dev, "erasesize", &mtd->erasesize, "%u");
		dev_add_param_uint32_ro(&mtd->dev, "writesize", &mtd->writesize, "%u");
		dev_add_param_uint32_ro(&mtd->dev, "oobsize", &mtd->oobsize, "%u");
	}

	ret = devfs_create(&mtd->cdev);
	if (ret)
		goto err;

	if (mtd->parent && !(mtd->cdev.flags & DEVFS_PARTITION_FIXED)) {
		struct mtd_info *mtdpart;

		list_for_each_entry(mtdpart, &mtd->parent->partitions, partitions_entry) {
			if (mtdpart->master_offset + mtdpart->size <= mtd->master_offset)
				continue;
			if (mtd->master_offset + mtd->size <= mtdpart->master_offset)
				continue;
			dev_err(&mtd->dev, "New partition %s conflicts with %s\n",
					mtd->name, mtdpart->name);
			goto err1;
		}

		list_add_sort(&mtd->partitions_entry, &mtd->parent->partitions, mtd_part_compare);
	}

	if (mtd_can_have_bb(mtd))
		mtd->cdev_bb = mtd_add_bb(mtd, NULL);

	if (!mtd->parent) {
		struct device_node *np = mtd_get_of_node(mtd);

		dev_add_param_string(&mtd->dev, "partitions", mtd_partition_set, mtd_partition_get, &mtd->partition_string, mtd);
		if (IS_ENABLED(CONFIG_OFDEVICE) && np) {
			of_parse_partitions(&mtd->cdev, np);
			ret = of_register_fixup(mtd_partition_fixup, mtd);
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
	unregister_device(&mtd->dev);

	return ret;
}

int del_mtd_device(struct mtd_info *mtd)
{
	struct mtddev_hook *hook;

	list_for_each_entry(hook, &mtd_register_hooks, hook)
		if (hook->del_mtd_device)
			hook->del_mtd_device(mtd, &hook->priv);

	devfs_remove(&mtd->cdev);
	if (mtd->cdev_bb)
		mtd_del_bb(mtd);
	unregister_device(&mtd->dev);
	free(mtd->param_size.value);
	free(mtd->cdev.name);
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

/**
 * mtd_ooblayout_ecc - Get the OOB region definition of a specific ECC section
 * @mtd: MTD device structure
 * @section: ECC section. Depending on the layout you may have all the ECC
 *	     bytes stored in a single contiguous section, or one section
 *	     per ECC chunk (and sometime several sections for a single ECC
 *	     ECC chunk)
 * @oobecc: OOB region struct filled with the appropriate ECC position
 *	    information
 *
 * This function returns ECC section information in the OOB area. If you want
 * to get all the ECC bytes information, then you should call
 * mtd_ooblayout_ecc(mtd, section++, oobecc) until it returns -ERANGE.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_ecc(struct mtd_info *mtd, int section,
		      struct mtd_oob_region *oobecc)
{
	memset(oobecc, 0, sizeof(*oobecc));

	if (section < 0)
		return -EINVAL;

	if (!mtd->ooblayout || !mtd->ooblayout->ecc)
		return -ENOTSUPP;

	return mtd->ooblayout->ecc(mtd, section, oobecc);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_ecc);

/**
 * mtd_ooblayout_free - Get the OOB region definition of a specific free
 *			section
 * @mtd: MTD device structure
 * @section: Free section you are interested in. Depending on the layout
 *	     you may have all the free bytes stored in a single contiguous
 *	     section, or one section per ECC chunk plus an extra section
 *	     for the remaining bytes (or other funky layout).
 * @oobfree: OOB region struct filled with the appropriate free position
 *	     information
 *
 * This function returns free bytes position in the OOB area. If you want
 * to get all the free bytes information, then you should call
 * mtd_ooblayout_free(mtd, section++, oobfree) until it returns -ERANGE.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_free(struct mtd_info *mtd, int section,
		       struct mtd_oob_region *oobfree)
{
	memset(oobfree, 0, sizeof(*oobfree));

	if (section < 0)
		return -EINVAL;

	if (!mtd->ooblayout || !mtd->ooblayout->free)
		return -ENOTSUPP;

	return mtd->ooblayout->free(mtd, section, oobfree);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_free);

/**
 * mtd_ooblayout_find_region - Find the region attached to a specific byte
 * @mtd: mtd info structure
 * @byte: the byte we are searching for
 * @sectionp: pointer where the section id will be stored
 * @oobregion: used to retrieve the ECC position
 * @iter: iterator function. Should be either mtd_ooblayout_free or
 *	  mtd_ooblayout_ecc depending on the region type you're searching for
 *
 * This function returns the section id and oobregion information of a
 * specific byte. For example, say you want to know where the 4th ECC byte is
 * stored, you'll use:
 *
 * mtd_ooblayout_find_region(mtd, 3, &section, &oobregion, mtd_ooblayout_ecc);
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ooblayout_find_region(struct mtd_info *mtd, int byte,
				int *sectionp, struct mtd_oob_region *oobregion,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	int pos = 0, ret, section = 0;

	memset(oobregion, 0, sizeof(*oobregion));

	while (1) {
		ret = iter(mtd, section, oobregion);
		if (ret)
			return ret;

		if (pos + oobregion->length > byte)
			break;

		pos += oobregion->length;
		section++;
	}

	/*
	 * Adjust region info to make it start at the beginning at the
	 * 'start' ECC byte.
	 */
	oobregion->offset += byte - pos;
	oobregion->length -= byte - pos;
	*sectionp = section;

	return 0;
}

/**
 * mtd_ooblayout_find_eccregion - Find the ECC region attached to a specific
 *				  ECC byte
 * @mtd: mtd info structure
 * @eccbyte: the byte we are searching for
 * @sectionp: pointer where the section id will be stored
 * @oobregion: OOB region information
 *
 * Works like mtd_ooblayout_find_region() except it searches for a specific ECC
 * byte.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_find_eccregion(struct mtd_info *mtd, int eccbyte,
				 int *section,
				 struct mtd_oob_region *oobregion)
{
	return mtd_ooblayout_find_region(mtd, eccbyte, section, oobregion,
					 mtd_ooblayout_ecc);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_find_eccregion);

/**
 * mtd_ooblayout_get_bytes - Extract OOB bytes from the oob buffer
 * @mtd: mtd info structure
 * @buf: destination buffer to store OOB bytes
 * @oobbuf: OOB buffer
 * @start: first byte to retrieve
 * @nbytes: number of bytes to retrieve
 * @iter: section iterator
 *
 * Extract bytes attached to a specific category (ECC or free)
 * from the OOB buffer and copy them into buf.
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ooblayout_get_bytes(struct mtd_info *mtd, u8 *buf,
				const u8 *oobbuf, int start, int nbytes,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	struct mtd_oob_region oobregion;
	int section, ret;

	ret = mtd_ooblayout_find_region(mtd, start, &section,
					&oobregion, iter);

	while (!ret) {
		int cnt;

		cnt = min_t(int, nbytes, oobregion.length);
		memcpy(buf, oobbuf + oobregion.offset, cnt);
		buf += cnt;
		nbytes -= cnt;

		if (!nbytes)
			break;

		ret = iter(mtd, ++section, &oobregion);
	}

	return ret;
}

/**
 * mtd_ooblayout_set_bytes - put OOB bytes into the oob buffer
 * @mtd: mtd info structure
 * @buf: source buffer to get OOB bytes from
 * @oobbuf: OOB buffer
 * @start: first OOB byte to set
 * @nbytes: number of OOB bytes to set
 * @iter: section iterator
 *
 * Fill the OOB buffer with data provided in buf. The category (ECC or free)
 * is selected by passing the appropriate iterator.
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ooblayout_set_bytes(struct mtd_info *mtd, const u8 *buf,
				u8 *oobbuf, int start, int nbytes,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	struct mtd_oob_region oobregion;
	int section, ret;

	ret = mtd_ooblayout_find_region(mtd, start, &section,
					&oobregion, iter);

	while (!ret) {
		int cnt;

		cnt = min_t(int, nbytes, oobregion.length);
		memcpy(oobbuf + oobregion.offset, buf, cnt);
		buf += cnt;
		nbytes -= cnt;

		if (!nbytes)
			break;

		ret = iter(mtd, ++section, &oobregion);
	}

	return ret;
}

/**
 * mtd_ooblayout_count_bytes - count the number of bytes in a OOB category
 * @mtd: mtd info structure
 * @iter: category iterator
 *
 * Count the number of bytes in a given category.
 *
 * Returns a positive value on success, a negative error code otherwise.
 */
static int mtd_ooblayout_count_bytes(struct mtd_info *mtd,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	struct mtd_oob_region oobregion;
	int section = 0, ret, nbytes = 0;

	while (1) {
		ret = iter(mtd, section++, &oobregion);
		if (ret) {
			if (ret == -ERANGE)
				ret = nbytes;
			break;
		}

		nbytes += oobregion.length;
	}

	return ret;
}

/**
 * mtd_ooblayout_get_eccbytes - extract ECC bytes from the oob buffer
 * @mtd: mtd info structure
 * @eccbuf: destination buffer to store ECC bytes
 * @oobbuf: OOB buffer
 * @start: first ECC byte to retrieve
 * @nbytes: number of ECC bytes to retrieve
 *
 * Works like mtd_ooblayout_get_bytes(), except it acts on ECC bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_get_eccbytes(struct mtd_info *mtd, u8 *eccbuf,
			       const u8 *oobbuf, int start, int nbytes)
{
	return mtd_ooblayout_get_bytes(mtd, eccbuf, oobbuf, start, nbytes,
				       mtd_ooblayout_ecc);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_get_eccbytes);

/**
 * mtd_ooblayout_set_eccbytes - set ECC bytes into the oob buffer
 * @mtd: mtd info structure
 * @eccbuf: source buffer to get ECC bytes from
 * @oobbuf: OOB buffer
 * @start: first ECC byte to set
 * @nbytes: number of ECC bytes to set
 *
 * Works like mtd_ooblayout_set_bytes(), except it acts on ECC bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_set_eccbytes(struct mtd_info *mtd, const u8 *eccbuf,
			       u8 *oobbuf, int start, int nbytes)
{
	return mtd_ooblayout_set_bytes(mtd, eccbuf, oobbuf, start, nbytes,
				       mtd_ooblayout_ecc);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_set_eccbytes);

/**
 * mtd_ooblayout_get_databytes - extract data bytes from the oob buffer
 * @mtd: mtd info structure
 * @databuf: destination buffer to store ECC bytes
 * @oobbuf: OOB buffer
 * @start: first ECC byte to retrieve
 * @nbytes: number of ECC bytes to retrieve
 *
 * Works like mtd_ooblayout_get_bytes(), except it acts on free bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_get_databytes(struct mtd_info *mtd, u8 *databuf,
				const u8 *oobbuf, int start, int nbytes)
{
	return mtd_ooblayout_get_bytes(mtd, databuf, oobbuf, start, nbytes,
				       mtd_ooblayout_free);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_get_databytes);

/**
 * mtd_ooblayout_set_databytes - set data bytes into the oob buffer
 * @mtd: mtd info structure
 * @databuf: source buffer to get data bytes from
 * @oobbuf: OOB buffer
 * @start: first ECC byte to set
 * @nbytes: number of ECC bytes to set
 *
 * Works like mtd_ooblayout_set_bytes(), except it acts on free bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_set_databytes(struct mtd_info *mtd, const u8 *databuf,
				u8 *oobbuf, int start, int nbytes)
{
	return mtd_ooblayout_set_bytes(mtd, databuf, oobbuf, start, nbytes,
				       mtd_ooblayout_free);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_set_databytes);

/**
 * mtd_ooblayout_count_freebytes - count the number of free bytes in OOB
 * @mtd: mtd info structure
 *
 * Works like mtd_ooblayout_count_bytes(), except it count free bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_count_freebytes(struct mtd_info *mtd)
{
	return mtd_ooblayout_count_bytes(mtd, mtd_ooblayout_free);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_count_freebytes);

/**
 * mtd_ooblayout_count_eccbytes - count the number of ECC bytes in OOB
 * @mtd: mtd info structure
 *
 * Works like mtd_ooblayout_count_bytes(), except it count ECC bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_count_eccbytes(struct mtd_info *mtd)
{
	return mtd_ooblayout_count_bytes(mtd, mtd_ooblayout_ecc);
}
EXPORT_SYMBOL_GPL(mtd_ooblayout_count_eccbytes);


/**
 * mtd_ecclayout_ecc - Default ooblayout_ecc iterator implementation
 * @mtd: MTD device structure
 * @section: ECC section. Depending on the layout you may have all the ECC
 *	     bytes stored in a single contiguous section, or one section
 *	     per ECC chunk (and sometime several sections for a single ECC
 *	     ECC chunk)
 * @oobecc: OOB region struct filled with the appropriate ECC position
 *	    information
 *
 * This function is just a wrapper around the mtd->ecclayout field and is
 * here to ease the transition to the mtd_ooblayout_ops approach.
 * All it does is convert the layout->eccpos information into proper oob
 * region definitions.
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ecclayout_ecc(struct mtd_info *mtd, int section,
			     struct mtd_oob_region *oobecc)
{
	int eccbyte = 0, cursection = 0, length = 0, eccpos = 0;

	if (!mtd->ecclayout)
		return -ENOTSUPP;

	/*
	 * This logic allows us to reuse the ->ecclayout information and
	 * expose them as ECC regions (as done for the OOB free regions).
	 *
	 * TODO: this should be dropped as soon as we get rid of the
	 * ->ecclayout field.
	 */
	for (eccbyte = 0; eccbyte < mtd->ecclayout->eccbytes; eccbyte++) {
		eccpos = mtd->ecclayout->eccpos[eccbyte];

		if (eccbyte < mtd->ecclayout->eccbytes - 1) {
			int neccpos = mtd->ecclayout->eccpos[eccbyte + 1];

			if (eccpos + 1 == neccpos) {
				length++;
				continue;
			}
		}

		if (section == cursection)
			break;

		length = 0;
		cursection++;
	}

	if (cursection != section || eccbyte >= mtd->ecclayout->eccbytes)
		return -ERANGE;

	oobecc->length = length + 1;
	oobecc->offset = eccpos - length;

	return 0;
}

/**
 * mtd_ecclayout_ecc - Default ooblayout_free iterator implementation
 * @mtd: MTD device structure
 * @section: Free section. Depending on the layout you may have all the free
 *	     bytes stored in a single contiguous section, or one section
 *	     per ECC chunk (and sometime several sections for a single ECC
 *	     ECC chunk)
 * @oobfree: OOB region struct filled with the appropriate free position
 *	     information
 *
 * This function is just a wrapper around the mtd->ecclayout field and is
 * here to ease the transition to the mtd_ooblayout_ops approach.
 * All it does is convert the layout->oobfree information into proper oob
 * region definitions.
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ecclayout_free(struct mtd_info *mtd, int section,
			      struct mtd_oob_region *oobfree)
{
	struct nand_ecclayout *layout = mtd->ecclayout;

	if (!layout)
		return -ENOTSUPP;

	if (section >= MTD_MAX_OOBFREE_ENTRIES_LARGE ||
	    !layout->oobfree[section].length)
		return -ERANGE;

	oobfree->offset = layout->oobfree[section].offset;
	oobfree->length = layout->oobfree[section].length;

	return 0;
}

static const struct mtd_ooblayout_ops mtd_ecclayout_wrapper_ops = {
	.ecc = mtd_ecclayout_ecc,
	.free = mtd_ecclayout_free,
};

/**
 * mtd_set_ecclayout - Attach an ecclayout to an MTD device
 * @mtd: MTD device structure
 * @ecclayout: The ecclayout to attach to the device
 *
 * Returns zero on success, a negative error code otherwise.
 */
void mtd_set_ecclayout(struct mtd_info *mtd, struct nand_ecclayout *ecclayout)
{
	if (!mtd || !ecclayout)
		return;

	mtd->ecclayout = ecclayout;
	mtd_set_ooblayout(mtd, &mtd_ecclayout_wrapper_ops);
}
EXPORT_SYMBOL_GPL(mtd_set_ecclayout);

void mtd_print_oob_info(struct mtd_info *mtd)
{
	struct mtd_oob_region region;
	int ret, i = 0, j, rowsize;
	unsigned char *oob;

	if (!mtd->ooblayout)
		return;

	oob = malloc(mtd->oobsize);
	if (!oob)
		return;

	memset(oob, ' ', mtd->oobsize);

	printf("---- ECC regions ----\n");
	while (1) {
		ret = mtd->ooblayout->ecc(mtd, i, &region);
		if (ret)
			break;
		printf("ecc:  offset: %4d length: %4d\n",
		       region.offset, region.length);
		i++;

		for (j = 0; j < region.length; j++) {
			unsigned char *p = oob + region.offset + j;

			if (*p != ' ')
				printf("oob offset %d already set to '%c'\n",
				       region.offset + j, *p);
			*p = 'e';
		}
	}

	i = 0;

	printf("---- free regions ----\n");
	while (1) {
		ret = mtd->ooblayout->free(mtd, i, &region);
		if (ret)
			break;

		printf("free: offset: %4d length: %4d\n",
		       region.offset, region.length);
		i++;

		for (j = 0; j < region.length; j++) {
			unsigned char *p = oob + region.offset + j;

			if (*p != ' ')
				printf("oob offset %d already set to '%c'\n",
				       region.offset + j, *p);
			*p = 'f';
		}
	}

	j = 0;
	rowsize = 16;

	printf("---- OOB area ----\n");
	while (1) {
		printf("%-4d", j);

		for (i = 0; i < rowsize; i++) {
			if (i + j >= mtd->oobsize)
				break;
			if (i == rowsize / 2)
				printf(" ");
			printf(" %c", oob[j + i]);
		}

		printf("\n");
		j += rowsize;

		if (j >= mtd->oobsize)
			break;
	}

	free(oob);
}
