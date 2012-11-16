#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/mtd/mtd.h>

struct mtd_part {
	struct mtd_info mtd;
	struct mtd_info *master;
	uint64_t offset;
	struct list_head list;
};

#define PART(x)  ((struct mtd_part *)(x))

static int mtd_part_read(struct mtd_info *mtd, loff_t from, size_t len,
                size_t *retlen, u_char *buf)
{
	struct mtd_part *part = PART(mtd);
	struct mtd_ecc_stats stats;
	int res;

	stats = part->master->ecc_stats;

	if (from >= mtd->size)
		len = 0;
	else if (from + len > mtd->size)
		len = mtd->size - from;
	res = part->master->read(part->master, from + part->offset,
				len, retlen, buf);
	return res;
}

static int mtd_part_write(struct mtd_info *mtd, loff_t to, size_t len,
                size_t *retlen, const u_char *buf)
{
	struct mtd_part *part = PART(mtd);

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (to >= mtd->size)
		len = 0;
	else if (to + len > mtd->size)
		len = mtd->size - to;
	return part->master->write(part->master, to + part->offset,
					len, retlen, buf);
}

static int mtd_part_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct mtd_part *part = PART(mtd);
	int ret;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (instr->addr >= mtd->size)
		return -EINVAL;
	instr->addr += part->offset;
	ret = part->master->erase(part->master, instr);
	if (ret) {
		if (instr->fail_addr != 0xffffffff)
			instr->fail_addr -= part->offset;
		instr->addr -= part->offset;
	}
	return ret;
}

static int mtd_part_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_part *part = PART(mtd);
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += part->offset;
	return mtd_block_isbad(part->master, ofs);
}

static int mtd_part_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_part *part = PART(mtd);
	int res;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	if (ofs >= mtd->size)
		return -EINVAL;
	ofs += part->offset;
	res = part->master->block_markbad(part->master, ofs);
	if (!res)
		mtd->ecc_stats.badblocks++;
	return res;
}

struct mtd_info *mtd_add_partition(struct mtd_info *mtd, off_t offset, size_t size,
		unsigned long flags, const char *name)
{
	struct mtd_part *slave;
	struct mtd_info *slave_mtd;
	int start = 0, end = 0, i;

	slave = xzalloc(sizeof(*slave));
	slave_mtd = &slave->mtd;

	memcpy(slave_mtd, mtd, sizeof(*slave));

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

	slave_mtd->numeraseregions = end - start;

	slave_mtd->read = mtd_part_read;
	slave_mtd->write = mtd_part_write;
	slave_mtd->erase = mtd_part_erase;
	slave_mtd->block_isbad = mtd->block_isbad ? mtd_part_block_isbad : NULL;
	slave_mtd->block_markbad = mtd->block_markbad ? mtd_part_block_markbad : NULL;
	slave_mtd->size = size;
	slave_mtd->name = strdup(name);

	slave->offset = offset;
	slave->master = mtd;

	return slave_mtd;
}

void mtd_del_partition(struct mtd_info *mtd)
{
	struct mtd_part *part = PART(mtd);

	free(mtd->name);
	free(part);
}
