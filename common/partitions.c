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

#include "partitions/parser.h"

static LIST_HEAD(partition_parser_list);

/**
 * Register one partition on the given block device
 * @param blk Block device to register to
 * @param part Partition description
 * @param no Partition number
 * @return 0 on success
 */
static int register_one_partition(struct block_device *blk,
					struct partition *part, int no)
{
	char *partition_name;
	int ret;
	uint64_t start = part->first_sec * SECTOR_SIZE;
	uint64_t size = part->size * SECTOR_SIZE;
	struct cdev *cdev;

	partition_name = basprintf("%s.%d", blk->cdev.name, no);
	if (!partition_name)
		return -ENOMEM;
	dev_dbg(blk->dev, "Registering partition %s on drive %s (partuuid=%s)\n",
				partition_name, blk->cdev.name, part->partuuid);
	cdev = devfs_add_partition(blk->cdev.name,
				start, size, 0, partition_name);
	if (IS_ERR(cdev)) {
		ret = PTR_ERR(cdev);
		goto out;
	}

	cdev->flags |= DEVFS_PARTITION_FROM_TABLE;

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

/**
 * Try to collect partition information on the given block device
 * @param blk Block device to examine
 * @return 0 most of the time, negative value else
 *
 * It is not a failure if no partition information is found
 */
int parse_partition_table(struct block_device *blk)
{
	struct partition_desc *pdesc;
	int i;
	int rc = 0;
	struct partition_parser *parser;
	uint8_t *buf;

	pdesc = xzalloc(sizeof(*pdesc));
	buf = malloc(2 * SECTOR_SIZE);

	rc = block_read(blk, buf, 0, 2);
	if (rc != 0) {
		dev_err(blk->dev, "Cannot read MBR/partition table: %pe\n", ERR_PTR(rc));
		goto on_error;
	}

	parser = partition_parser_get_by_filetype(buf);
	if (!parser)
		goto on_error;

	parser->parse(buf, blk, pdesc);

	if (!pdesc->used_entries)
		goto on_error;

	/* at least one partition description found */
	for (i = 0; i < pdesc->used_entries; i++) {
		rc = register_one_partition(blk, &pdesc->parts[i], i);
		if (rc != 0)
			dev_err(blk->dev,
				"Failed to register partition %d on %s (%d)\n",
				i, blk->cdev.name, rc);
		if (rc != -ENODEV)
			rc = 0;
	}

on_error:
	free(buf);
	free(pdesc);
	return rc;
}

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
