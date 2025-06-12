/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __RAMDISK_H_
#define __RAMDISK_H_

#include <linux/types.h>

struct ramdisk;
struct block_device;

struct ramdisk *ramdisk_init(int sector_size);
void ramdisk_free(struct ramdisk *ramdisk);

struct block_device *ramdisk_get_block_device(struct ramdisk *ramdisk);

void ramdisk_setup_ro(struct ramdisk *ramdisk, const void *data, size_t size);
void ramdisk_setup_rw(struct ramdisk *ramdisk, void *data, size_t size);

const void *ramdisk_mmap(struct ramdisk *ramdisk, loff_t offset, size_t size);
static inline void ramdisk_munmap(struct ramdisk *ramdisk,
				  void *ptr, loff_t offset,
				  size_t size) {}

#endif
