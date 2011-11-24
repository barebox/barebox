/*
 * See file CREDITS for list of people who contributed to this
 * project.
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

#ifndef DISKS_H
#define DISKS_H

struct block_device;

/** Size of one sector in bytes */
#define SECTOR_SIZE 512

/** Size of one sector in bit shift */
#define SECTOR_SHIFT 9

/**
 * Description of one partition table entry (D*S type)
 */
struct partition_entry {
	uint8_t boot_indicator;	/*! Maybe marked as an active partition */
	uint8_t chs_begin[3];	/*! Start of the partition in cylinders, heads and sectors */
	uint8_t type;		/*! Filesystem type */
	uint8_t chs_end[3];	/*! End of the partition in cylinders, heads and sectors */
	uint32_t partition_start; /*! Start of the partition in LBA notation */
	uint32_t partition_size; /*! Start of the partition in LBA notation */
} __attribute__ ((packed));

extern int parse_partition_table(struct block_device*);

#endif /* DISKS_H */
