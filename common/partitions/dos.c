/*
 * Copyright (C) 2009...2011 Juergen Beisert, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <disks.h>
#include <init.h>
#include <asm/unaligned.h>

#include "parser.h"

/**
 * Guess the size of the disk, based on the partition table entries
 * @param dev device to create partitions for
 * @param table partition table
 * @return sector count
 */
static int disk_guess_size(struct device_d *dev, struct partition_entry *table)
{
	uint64_t size = 0;
	int i;

	for (i = 0; i < 4; i++) {
		if (table[i].partition_start != 0) {
			size += get_unaligned_le32(&table[i].partition_start) - size;
			size += get_unaligned_le32(&table[i].partition_size);
		}
	}

	return (int)size;
}

/**
 * Check if a DOS like partition describes this block device
 * @param blk Block device to register to
 * @param pd Where to store the partition information
 *
 * It seems at least on ARM this routine canot use temp. stack space for the
 * sector. So, keep the malloc/free.
 */
static void dos_partition(void *buf, struct block_device *blk,
			  struct partition_desc *pd)
{
	struct partition_entry *table;
	struct partition pentry;
	uint8_t *buffer = buf;
	int i;

	table = (struct partition_entry *)&buffer[446];

	/* valid for x86 BIOS based disks only */
	if (IS_ENABLED(CONFIG_DISK_BIOS) && blk->num_blocks == 0)
		blk->num_blocks = disk_guess_size(blk->dev, table);

	for (i = 0; i < 4; i++) {
		pentry.first_sec = get_unaligned_le32(&table[i].partition_start);
		pentry.size = get_unaligned_le32(&table[i].partition_size);

		if (pentry.first_sec != 0) {
			pd->parts[pd->used_entries].first_sec = pentry.first_sec;
			pd->parts[pd->used_entries].size = pentry.size;
			pd->used_entries++;
		} else {
			dev_dbg(blk->dev, "Skipping empty partition %d\n", i);
		}
	}
}

static struct partition_parser dos = {
	.parse = dos_partition,
	.type = filetype_mbr,
};

static int dos_partition_init(void)
{
	return partition_parser_register(&dos);
}
postconsole_initcall(dos_partition_init);
