/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef DISKS_H
#define DISKS_H

#include <linux/types.h>
#include <linux/errno.h>

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

#ifdef CONFIG_PARTITION_MANIPULATION
int reparse_partition_table(struct block_device *blk);
#else
static inline int reparse_partition_table(struct block_device *blk)
{
	return -ENOSYS;
}
#endif

#endif /* DISKS_H */
