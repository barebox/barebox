/*
 * Header file for UBI support for U-Boot
 *
 * Adaptation from kernel to U-Boot
 *
 *  Copyright (C) 2005-2007 Samsung Electronics
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __UBOOT_UBI_H
#define __UBOOT_UBI_H

#include <common.h>
#include <malloc.h>
#include <asm-generic/div64.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/string.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/ubi.h>

#define crc32(seed, data, length)  crc32_no_comp(seed, (unsigned char const *)data, length)

/* configurable */
#define CONFIG_MTD_UBI_WL_THRESHOLD	4096
#define UBI_IO_DEBUG			0

/* upd.c */
static inline unsigned long copy_from_user(void *dest, const void *src,
					   unsigned long count)
{
	memcpy((void *)dest, (void *)src, count);
	return 0;
}

/* common */

#define GFP_NOFS			1

#define wake_up_process(...)	do { } while (0)

#define BUS_ID_SIZE		20

#define MAX_ERRNO		4095

#ifndef __UBIFS_H__
#include "ubi.h"
#endif

/* functions */

extern struct ubi_device *ubi_devices[];

int ubi_cdev_add(struct ubi_device *ubi);

#endif
