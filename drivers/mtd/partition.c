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

struct mtd_info *mtd_add_partition(struct mtd_info *mtd, off_t offset,
		uint64_t size, unsigned long flags, const char *name)
{
	struct mtd_info *part;
	int start = 0, end = 0, i;

	part = xzalloc(sizeof(*part));

	part->type = mtd->type;
	part->flags = mtd->flags;
	part->parent = &mtd->class_dev;
	part->erasesize = mtd->erasesize;
	part->writesize = mtd->writesize;
	part->writebufsize = mtd->writebufsize;
	part->oobsize = mtd->oobsize;
	part->oobavail = mtd->oobavail;
	part->bitflip_threshold = mtd->bitflip_threshold;
	part->ecclayout = mtd->ecclayout;
	part->ecc_strength = mtd->ecc_strength;
	part->subpage_sft = mtd->subpage_sft;

	/*
	 * find the number of eraseregions the partition includes.
	 * Do not bother to create the mtd_erase_region_infos as
	 * ubi is only interested in its number. UBI does not
	 * yet support multiple erase regions.
	 */
	for (i = mtd->numeraseregions - 1; i >= 0; i--) {
		struct mtd_erase_region_info *region = &mtd->eraseregions[i];
		if (offset >= region->offset &&
		    offset < region->offset + region->erasesize * region->numblocks)
			start = i;
		if (offset + size >= region->offset &&
		    offset + size <= region->offset + region->erasesize * region->numblocks)
			end = i;
	}

	part->numeraseregions = end - start;

	part->read = mtd_part_read;
	if (IS_ENABLED(CONFIG_MTD_WRITE)) {
		part->write = mtd_part_write;
		part->erase = mtd_part_erase;
		part->block_markbad = mtd->block_markbad ? mtd_part_block_markbad : NULL;
	}

	part->block_isbad = mtd->block_isbad ? mtd_part_block_isbad : NULL;
	part->size = size;
	part->name = strdup(name);

	part->master_offset = offset;
	part->master = mtd;

	if (!strncmp(mtd->cdev.name, name, strlen(mtd->cdev.name)))
		part->cdev.partname = xstrdup(name + strlen(mtd->cdev.name) + 1);

	add_mtd_device(part, part->name, DEVICE_ID_SINGLE);

	return part;
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
