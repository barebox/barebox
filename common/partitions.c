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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/**
 * @file
 * @brief Generic support for partition tables on disk like media
 *
 * @todo Support for disks larger than 4 GiB
 * @todo Reliable size detection for BIOS based disks (on x86 only)
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <block.h>
#include <asm/unaligned.h>
#include <disks.h>
#include <dma.h>

struct partition {
	uint64_t first_sec;
	uint64_t size;
};

struct partition_desc {
	int used_entries;
	struct partition parts[8];
};

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
			size += get_unaligned(&table[i].partition_start) - size;
			size += get_unaligned(&table[i].partition_size);
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
static void __maybe_unused try_dos_partition(struct block_device *blk,
						struct partition_desc *pd)
{
	uint8_t *buffer;
	struct partition_entry *table;
	struct partition pentry;
	int i, rc;

	buffer = dma_alloc(SECTOR_SIZE);

	/* read in the MBR to get the partition table */
	rc = blk->ops->read(blk, buffer, 0, 1);
	if (rc != 0) {
		dev_err(blk->dev, "Cannot read MBR/partition table\n");
		goto on_error;
	}

	if ((buffer[510] != 0x55) || (buffer[511] != 0xAA)) {
		dev_info(blk->dev, "No partition table found\n");
		goto on_error;
	}

	table = (struct partition_entry *)&buffer[446];

	/* valid for x86 BIOS based disks only */
	if (IS_ENABLED(CONFIG_DISK_BIOS) && blk->num_blocks == 0)
		blk->num_blocks = disk_guess_size(blk->dev, table);

	for (i = 0; i < 4; i++) {
		pentry.first_sec = get_unaligned(&table[i].partition_start);
		pentry.size = get_unaligned(&table[i].partition_size);

		if (pentry.first_sec != 0) {
			pd->parts[pd->used_entries].first_sec = pentry.first_sec;
			pd->parts[pd->used_entries].size = pentry.size;
			pd->used_entries++;
		} else {
			dev_dbg(blk->dev, "Skipping empty partition %d\n", i);
		}
	}

on_error:
	dma_free(buffer);
}

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
	char partition_name[19];

	sprintf(partition_name, "%s.%d", blk->cdev.name, no);
	dev_dbg(blk->dev, "Registering partition %s on drive %s\n",
				partition_name, blk->cdev.name);
	return devfs_add_partition(blk->cdev.name,
				part->first_sec * SECTOR_SIZE,
				part->size * SECTOR_SIZE,
				0, partition_name);
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
	struct partition_desc pdesc = { .used_entries = 0, };
	int i;
	int rc = 0;

#ifdef CONFIG_PARTITION_DISK_DOS
	try_dos_partition(blk, &pdesc);
#endif
	if (!pdesc.used_entries)
		return 0;

	/* at least one partition description found */
	for (i = 0; i < pdesc.used_entries; i++) {
		rc = register_one_partition(blk, &pdesc.parts[i], i);
		if (rc != 0)
			dev_err(blk->dev,
				"Failed to register partition %d on %s (%d)\n",
				i, blk->cdev.name, rc);
		if (rc != -ENODEV)
			rc = 0;
	}

	return rc;
}
