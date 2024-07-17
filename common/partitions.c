// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009...2011 Juergen Beisert, Pengutronix
 */

/**
 * @file
 * @brief Generic support for partition tables on disk like media
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <block.h>
#include <asm/unaligned.h>
#include <disks.h>
#include <filetype.h>
#include <linux/err.h>
#include <partitions.h>

static LIST_HEAD(partition_parser_list);

/**
 * Register one partition on the given block device
 * @param blk Block device to register to
 * @param part Partition description
 * @param no Partition number
 * @return 0 on success
 */
static int register_one_partition(struct block_device *blk, struct partition *part)
{
	char *partition_name;
	int ret;
	struct cdev *cdev;
	struct devfs_partition partinfo = {
		.offset = part->first_sec * SECTOR_SIZE,
		.size = part->size * SECTOR_SIZE,
	};

	partition_name = basprintf("%s.%d", blk->cdev.name, part->num);
	if (!partition_name)
		return -ENOMEM;

	partinfo.name = partition_name;

	dev_dbg(blk->dev, "Registering partition %s on drive %s (partuuid=%s)\n",
				partition_name, blk->cdev.name, part->partuuid);
	cdev = cdevfs_add_partition(&blk->cdev, &partinfo);
	if (IS_ERR(cdev)) {
		ret = PTR_ERR(cdev);
		goto out;
	}

	cdev->flags |= DEVFS_PARTITION_FROM_TABLE | part->flags;
	cdev->typeflags |= part->typeflags;
	cdev->typeuuid = part->typeuuid;
	strcpy(cdev->partuuid, part->partuuid);

	free(partition_name);

	if (!part->name[0])
		return 0;

	partition_name = xasprintf("%s.%s", blk->cdev.name, part->name);
	ret = devfs_create_link(cdev, partition_name);
	if (ret)
		dev_warn(blk->dev, "Failed to create link from %s to %s\n",
			 partition_name, blk->cdev.name);
	free(partition_name);

	return 0;
out:
	free(partition_name);
	return ret;
}

static struct partition_parser *partition_parser_get_by_filetype(uint8_t *buf)
{
	enum filetype type;
	struct partition_parser *parser;

	/* first new partition table as EFI GPT */
	type = file_detect_partition_table(buf, SECTOR_SIZE * 2);

	list_for_each_entry(parser, &partition_parser_list, list) {
		if (parser->type == type)
			return parser;
	}

	/* if not parser found search for old one
	 * so if EFI GPT not enable take it as MBR
	 * useful for compatibility
	 */
	type = file_detect_partition_table(buf, SECTOR_SIZE);
	if (type == filetype_fat && !is_fat_boot_sector(buf))
		type = filetype_mbr;

	list_for_each_entry(parser, &partition_parser_list, list) {
		if (parser->type == type)
			return parser;
	}

	return NULL;
}

struct partition_desc *partition_table_new(struct block_device *blk, const char *type)
{
	struct partition_desc *pdesc;
	struct partition_parser *parser;

	list_for_each_entry(parser, &partition_parser_list, list) {
		if (!strcmp(parser->name, type))
			goto found;
	}

	pr_err("Cannot find partition parser \"%s\"\n", type);

	return ERR_PTR(-ENOSYS);

found:
	pdesc = parser->create(blk);
	if (IS_ERR(pdesc))
		return ERR_CAST(pdesc);

	pdesc->parser = parser;

	return pdesc;
}

struct partition_desc *partition_table_read(struct block_device *blk)
{
	struct partition_parser *parser;
	struct partition_desc *pdesc = NULL;
	uint8_t *buf;
	int ret;

	buf = malloc(2 * SECTOR_SIZE);

	ret = block_read(blk, buf, 0, 2);
	if (ret != 0) {
		dev_err(blk->dev, "Cannot read MBR/partition table: %pe\n", ERR_PTR(ret));
		goto err;
	}

	parser = partition_parser_get_by_filetype(buf);
	if (!parser)
		goto err;

	pdesc = parser->parse(buf, blk);
	if (!pdesc)
		goto err;

	pdesc->parser = parser;
err:
	free(buf);

	return pdesc;
}

int partition_table_write(struct partition_desc *pdesc)
{
	if (!pdesc->parser->write)
		return -ENOSYS;

	return pdesc->parser->write(pdesc);
}

static bool overlap(uint64_t s1, uint64_t e1, uint64_t s2, uint64_t e2)
{
	if (e1 < s2)
		return false;
	if (s1 > e2)
		return false;
	return true;
}

int partition_create(struct partition_desc *pdesc, const char *name,
		     const char *fs_type, uint64_t lba_start, uint64_t lba_end)
{
	struct partition *part;

	if (!pdesc->parser->mkpart)
		return -ENOSYS;

	if (lba_end < lba_start) {
		pr_err("lba_end < lba_start: %llu < %llu\n", lba_end, lba_start);
		return -EINVAL;
	}

	if (lba_end >= pdesc->blk->num_blocks) {
		pr_err("lba_end exceeds device: %llu >= %llu\n", lba_end, pdesc->blk->num_blocks);
		return -EINVAL;
	}

	list_for_each_entry(part, &pdesc->partitions, list) {
		if (overlap(part->first_sec,
				part->first_sec + part->size - 1,
				lba_start, lba_end)) {
			pr_err("new partition %llu-%llu overlaps with partition %s (%llu-%llu)\n",
			       lba_start, lba_end, part->name, part->first_sec,
				part->first_sec + part->size - 1);
			return -EINVAL;
		}
	}

	return pdesc->parser->mkpart(pdesc, name, fs_type, lba_start, lba_end);
}

int partition_remove(struct partition_desc *pdesc, int num)
{
	struct partition *part;

	if (!pdesc->parser->rmpart)
		return -ENOSYS;

	list_for_each_entry(part, &pdesc->partitions, list) {
		if (part->num == num)
			return pdesc->parser->rmpart(pdesc, part);
	}

	pr_err("Partition %d doesn't exist\n", num);
	return -ENOENT;
}

void partition_table_free(struct partition_desc *pdesc)
{
	pdesc->parser->partition_free(pdesc);
}

void partition_desc_init(struct partition_desc *pd, struct block_device *blk)
{
	pd->blk = blk;
	INIT_LIST_HEAD(&pd->partitions);
}

/**
 * Try to collect partition information on the given block device
 * @param blk Block device to examine
 * @return 0 most of the time, negative value else
 *
 * It is not a failure if no partition information is found
 */
int parse_partition_table(struct block_device *blk)
{
	int i = 0;
	int rc = 0;
	struct partition *part;
	struct partition_desc *pdesc;

	pdesc = partition_table_read(blk);
	if (!pdesc)
		return 0;

	/* at least one partition description found */
	list_for_each_entry(part, &pdesc->partitions, list) {
		rc = register_one_partition(blk, part);
		if (rc != 0)
			dev_err(blk->dev,
				"Failed to register partition %d on %s (%d)\n",
				i, blk->cdev.name, rc);
		if (rc != -ENODEV)
			rc = 0;

		i++;
	}

	partition_table_free(pdesc);

	return rc;
}

#ifdef CONFIG_PARTITION_MANIPULATION
int reparse_partition_table(struct block_device *blk)
{
	struct cdev *cdev = &blk->cdev;
	struct cdev *c, *tmp;

	list_for_each_entry(c, &cdev->partitions, partition_entry) {
		if (c->open) {
			pr_warn("%s is busy, will continue to use old partition table\n", c->name);
			return -EBUSY;
		}
	}

	list_for_each_entry_safe(c, tmp, &cdev->partitions, partition_entry) {
		if (c->flags & DEVFS_PARTITION_FROM_TABLE)
			cdevfs_del_partition(c);
	}

	return parse_partition_table(blk);
}
#endif

int partition_parser_register(struct partition_parser *p)
{
	list_add_tail(&p->list, &partition_parser_list);

	return 0;
}

/**
 * cdev_unallocated_space - return unallocated space
 * cdev: The cdev
 *
 * This function returns the space that is not allocated by any partition
 * at the start of a device.
 *
 * Return: The unallocated space at the start of the device in bytes
 */
loff_t cdev_unallocated_space(struct cdev *cdev)
{
	struct cdev *partcdev;
	loff_t start;

	if (!cdev)
		return 0;

	start = cdev->size;

	list_for_each_entry(partcdev, &cdev->partitions, partition_entry) {
		if (partcdev->offset < start)
			start = partcdev->offset;
	}

	return start;
}
