// SPDX-License-Identifier: GPL-2.0-only
/*
 * fat-pbl.c - PBL FAT filesystem barebox driver
 *
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 */

#define pr_fmt(fmt) "fat-pbl: " fmt

#include <common.h>
#include <pbl/bio.h>
#include "integer.h"
#include "ff.h"
#include "diskio.h"

DRESULT disk_read(FATFS *fat, BYTE *buf, DWORD sector, BYTE count)
{
	int ret = pbl_bio_read(fat->userdata, sector, buf, count);
	return ret != count ? ret : 0;
}

ssize_t pbl_fat_load(struct pbl_bio *bio, const char *filename, void *dest, size_t len)
{
	FATFS	fs = {};
	FIL	file = {};
	UINT	nread;
	int	ret;

	fs.userdata = bio;

	/* mount fs */
	ret = f_mount(&fs);
	if (ret) {
		pr_debug("f_mount(%s) failed: %d\n", filename, ret);
		return ret;
	}

	ret = f_open(&fs, &file, filename, FA_OPEN_EXISTING | FA_READ);
	if (ret) {
		pr_debug("f_open(%s) failed: %d\n", filename, ret);
		return ret;
	}

	pr_debug("Reading file %s to 0x%p\n", filename, dest);

	ret = f_read(&file, dest, len, &nread);
	if (ret) {
		pr_debug("f_read failed: %d\n", ret);
		return ret;
	}

	return f_len(&file) <= len ? nread : -ENOSPC;
}
