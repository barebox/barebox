#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>

static int mtd_part_read(struct mtd_info *mtd, loff_t from, size_t len,
                size_t *retlen, u_char *buf)
{
	int res;

	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	res = mtd->master->read(mtd->master, from + mtd->master_offset,
				len, retlen, buf);
	return res;
}

static int mtd_part_read_oob(struct mtd_info *mtd, loff_t from,
		struct mtd_oob_ops *ops)
{
	int res;

	if (from >= mtd->size)
		return -EINVAL;
	if (ops->datbuf && from + ops->len > mtd->size)
		return -EINVAL;

	res = mtd->master->read_oob(mtd->master, from + mtd->master_offset, ops);
	if (unlikely(res)) {
		if (mtd_is_bitflip(res))
			mtd->ecc_stats.corrected++;
		if (mtd_is_eccerr(res))
			mtd->ecc_stats.failed++;
	}
	return res;
}

static int mtd_part_write(struct mtd_info *mtd, loff_t to, size_t len,
                size_t *retlen, const u_char *buf)
{
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	return mtd->master->write(mtd->master, to + mtd->master_offset,
					len, retlen, buf);
}

static int mtd_part_write_oob(struct mtd_info *mtd, loff_t to,
		struct mtd_oob_ops *ops)
{
	if (to >= mtd->size)
		return -EINVAL;
	if (ops->datbuf && to + ops->len > mtd->size)
		return -EINVAL;
	return mtd->master->write_oob(mtd->master, to + mtd->master_offset, ops);
}

static int mtd_part_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int ret;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (instr->addr >= mtd->size)
		return -EINVAL;
	instr->addr += mtd->master_offset;
	ret = mtd->master->erase(mtd->master, instr);
	if (ret) {
		if (instr->fail_addr != 0xffffffff)
			instr->fail_addr -= mtd->master_offset;
		instr->addr -= mtd->master_offset;
	}
	return ret;
}

static int mtd_part_lock(struct mtd_info *mtd, loff_t offset, size_t len)
{
	if (!mtd->master->lock)
		return -ENOSYS;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	if (offset + len > mtd->size)
		return -EINVAL;

	offset += mtd->master_offset;

	return mtd->master->lock(mtd->master, offset, len);
}

static int mtd_part_unlock(struct mtd_info *mtd, loff_t offset, size_t len)
{
	if (!mtd->master->unlock)
		return -ENOSYS;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	if (offset + len > mtd->size)
		return -EINVAL;

	offset += mtd->master_offset;

	return mtd->master->unlock(mtd->master, offset, len);
}

static int mtd_part_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += mtd->master_offset;
	return mtd_block_isbad(mtd->master, ofs);
}

static int mtd_part_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int res;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += mtd->master_offset;
	res = mtd->master->block_markbad(mtd->master, ofs);
	if (!res)
		mtd->ecc_stats.badblocks++;
	return res;
}

static int mtd_part_block_markgood(struct mtd_info *mtd, loff_t ofs)
{
	int res;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += mtd->master_offset;
	res = mtd->master->block_markgood(mtd->master, ofs);
	if (!res)
		mtd->ecc_stats.badblocks--;
	return res;
}

struct mtd_info *mtd_add_partition(struct mtd_info *mtd, off_t offset,
		uint64_t size, unsigned long flags, const char *name)
{
	struct mtd_info *part;
	int ret;

	part = xzalloc(sizeof(*part));

	part->type = mtd->type;
	part->flags = mtd->flags;
	part->parent = &mtd->class_dev;
	part->writesize = mtd->writesize;
	part->writebufsize = mtd->writebufsize;
	part->oobsize = mtd->oobsize;
	part->oobavail = mtd->oobavail;
	part->bitflip_threshold = mtd->bitflip_threshold;
	part->ecclayout = mtd->ecclayout;
	part->ecc_strength = mtd->ecc_strength;
	part->subpage_sft = mtd->subpage_sft;
	part->cdev.flags = flags;

	if (mtd->numeraseregions > 1) {
		/* Deal with variable erase size stuff */
		int i, max = mtd->numeraseregions;
		u64 end = offset + size;
		struct mtd_erase_region_info *regions = mtd->eraseregions;

		/* Find the first erase regions which is part of this
		 * partition. */
		for (i = 0; i < max && regions[i].offset <= offset; i++)
			;
		/* The loop searched for the region _behind_ the first one */
		if (i > 0)
			i--;

		/* Pick biggest erasesize */
		for (; i < max && regions[i].offset < end; i++) {
			if (part->erasesize < regions[i].erasesize) {
				part->erasesize = regions[i].erasesize;
			}
		}
		BUG_ON(part->erasesize == 0);
	} else {
		/* Single erase size */
		part->erasesize = mtd->erasesize;
	}

	part->read = mtd_part_read;
	if (IS_ENABLED(CONFIG_MTD_WRITE)) {
		part->write = mtd_part_write;
		part->erase = mtd_part_erase;
		part->lock = mtd_part_lock;
		part->unlock = mtd_part_unlock;
		part->block_markbad = mtd->block_markbad ? mtd_part_block_markbad : NULL;
		part->block_markgood = mtd->block_markgood ? mtd_part_block_markgood : NULL;
	}

	if (mtd->write_oob)
		part->write_oob = mtd_part_write_oob;
	if (mtd->read_oob)
		part->read_oob = mtd_part_read_oob;

	part->block_isbad = mtd->block_isbad ? mtd_part_block_isbad : NULL;
	part->size = size;
	part->name = xstrdup(name);

	part->master_offset = offset;
	part->master = mtd;

	if (!strncmp(mtd->cdev.name, name, strlen(mtd->cdev.name)))
		part->cdev.partname = xstrdup(name + strlen(mtd->cdev.name) + 1);

	ret = add_mtd_device(part, part->name, DEVICE_ID_SINGLE);
	if (ret)
		goto err;

	part->cdev.master = &part->master->cdev;

	return part;
err:
	free(part->cdev.partname);
	free(part->name);
	free(part);

	return ERR_PTR(ret);
}

int mtd_del_partition(struct mtd_info *part)
{
	if (!part->master)
		return -EINVAL;

	del_mtd_device(part);

	free(part->cdev.partname);
	free(part->name);
	free(part);

	return 0;
}
