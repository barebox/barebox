/*
 * Copyright (C) 2009 Nokia Corporation
 * Copyright (C) 2012 Wolfram Sang, Pengutronix e.K. <w.sang@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * Author: Artem Bityutskiy
 * Author: Wolfram Sang
 *
 * This file is part of the MTD library. Based on pre-2.6.30 kernels support,
 * now adapted to barebox.
 *
 * NOTE: No support for 64 bit sizes yet!
 */

#include <common.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <crc.h>
#include <fs.h>
#include <fcntl.h>
#include <ioctl.h>
#include <linux/stat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/mtd-abi.h>
#include <mtd/libmtd.h>
#include <mtd/utils.h>

#define PROGRAM_NAME "libmtd"

static inline int mtd_ioctl_error(const struct mtd_dev_info *mtd, int eb,
				  const char *sreq)
{
	return sys_errmsg("%s ioctl failed for eraseblock %d (%s)",
			  sreq, eb, mtd->node);
}

static int mtd_valid_erase_block(const struct mtd_dev_info *mtd, int eb)
{
	if (eb < 0 || eb >= mtd->eb_cnt) {
		errmsg("bad eraseblock number %d, %s has %d eraseblocks",
		       eb, mtd->node, mtd->eb_cnt);
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int libmtd_erase(const struct mtd_dev_info *mtd, int fd, int eb)
{
	int ret;
	struct erase_info_user ei;

	ret = mtd_valid_erase_block(mtd, eb);
	if (ret)
		return ret;

	ei.start = (__u64)eb * mtd->eb_size;
	ei.length = mtd->eb_size;

	ret = ioctl(fd, MEMERASE, &ei);
	if (ret < 0)
		return mtd_ioctl_error(mtd, eb, "MEMERASE");
	return 0;
}

/* Patterns to write to a physical eraseblock when torturing it */
static uint8_t patterns[] = {0xa5, 0x5a, 0x0};

/**
 * check_pattern - check if buffer contains only a certain byte pattern.
 * @buf: buffer to check
 * @patt: the pattern to check
 * @size: buffer size in bytes
 *
 * This function returns %1 in there are only @patt bytes in @buf, and %0 if
 * something else was also found.
 */
static int check_pattern(const void *buf, uint8_t patt, int size)
{
	int i;

	for (i = 0; i < size; i++)
		if (((const uint8_t *)buf)[i] != patt)
			return 0;
	return 1;
}

int mtd_torture(const struct mtd_dev_info *mtd, int fd, int eb)
{
	int err, i, patt_count;
	void *buf;

	normsg("run torture test for PEB %d", eb);
	patt_count = ARRAY_SIZE(patterns);

	buf = xmalloc(mtd->eb_size);

	for (i = 0; i < patt_count; i++) {
		err = libmtd_erase(mtd, fd, eb);
		if (err)
			goto out;

		/* Make sure the PEB contains only 0xFF bytes */
		err = libmtd_read(mtd, fd, eb, 0, buf, mtd->eb_size);
		if (err)
			goto out;

		err = check_pattern(buf, 0xFF, mtd->eb_size);
		if (err == 0) {
			errmsg("erased PEB %d, but a non-0xFF byte found", eb);
			errno = EIO;
			goto out;
		}

		/* Write a pattern and check it */
		memset(buf, patterns[i], mtd->eb_size);
		err = libmtd_write(mtd, fd, eb, 0, buf, mtd->eb_size);
		if (err)
			goto out;

		memset(buf, ~patterns[i], mtd->eb_size);
		err = libmtd_read(mtd, fd, eb, 0, buf, mtd->eb_size);
		if (err)
			goto out;

		err = check_pattern(buf, patterns[i], mtd->eb_size);
		if (err == 0) {
			errmsg("pattern %x checking failed for PEB %d",
				patterns[i], eb);
			errno = EIO;
			goto out;
		}
	}

	err = 0;
	normsg("PEB %d passed torture test, do not mark it a bad", eb);

out:
	free(buf);
	return -1;
}

int mtd_is_bad(const struct mtd_dev_info *mtd, int fd, int eb)
{
	int ret;
	loff_t seek;

	ret = mtd_valid_erase_block(mtd, eb);
	if (ret)
		return ret;

	if (!mtd->bb_allowed)
		return 0;

	seek = (loff_t)eb * mtd->eb_size;
	ret = ioctl(fd, MEMGETBADBLOCK, &seek);
	if (ret == -1)
		return mtd_ioctl_error(mtd, eb, "MEMGETBADBLOCK");
	return ret;
}

int mtd_mark_bad(const struct mtd_dev_info *mtd, int fd, int eb)
{
	int ret;
	loff_t seek;

	if (!mtd->bb_allowed) {
		errno = EINVAL;
		return -1;
	}

	ret = mtd_valid_erase_block(mtd, eb);
	if (ret)
		return ret;

	seek = (loff_t)eb * mtd->eb_size;
	ret = ioctl(fd, MEMSETBADBLOCK, &seek);
	if (ret == -1)
		return mtd_ioctl_error(mtd, eb, "MEMSETBADBLOCK");
	return 0;
}

int libmtd_read(const struct mtd_dev_info *mtd, int fd, int eb, int offs,
	     void *buf, int len)
{
	int ret, rd = 0;
	off_t seek;

	ret = mtd_valid_erase_block(mtd, eb);
	if (ret)
		return ret;

	if (offs < 0 || offs + len > mtd->eb_size) {
		errmsg("bad offset %d or length %d, %s eraseblock size is %d",
		       offs, len, mtd->node, mtd->eb_size);
		errno = EINVAL;
		return -1;
	}

	/* Seek to the beginning of the eraseblock */
	seek = (off_t)eb * mtd->eb_size + offs;
	if (lseek(fd, seek, SEEK_SET) != seek)
		return sys_errmsg("cannot seek %s to offset %llu",
				  mtd->node, (unsigned long long)seek);

	while (rd < len) {
		ret = read(fd, buf, len);
		if (ret < 0)
			return sys_errmsg("cannot read %d bytes from %s (eraseblock %d, offset %d)",
					  len, mtd->node, eb, offs);
		rd += ret;
	}

	return 0;
}

int libmtd_write(const struct mtd_dev_info *mtd, int fd, int eb, int offs,
	      void *buf, int len)
{
	int ret;
	off_t seek;

	ret = mtd_valid_erase_block(mtd, eb);
	if (ret)
		return ret;

	if (offs < 0 || offs + len > mtd->eb_size) {
		errmsg("bad offset %d or length %d, %s eraseblock size is %d",
		       offs, len, mtd->node, mtd->eb_size);
		errno = EINVAL;
		return -1;
	}
	if (offs % mtd->subpage_size) {
		errmsg("write offset %d is not aligned to %s min. I/O size %d",
		       offs, mtd->node, mtd->subpage_size);
		errno = EINVAL;
		return -1;
	}
	if (len % mtd->subpage_size) {
		errmsg("write length %d is not aligned to %s min. I/O size %d",
		       len, mtd->node, mtd->subpage_size);
		errno = EINVAL;
		return -1;
	}

	/* Seek to the beginning of the eraseblock */
	seek = (off_t)eb * mtd->eb_size + offs;
	if (lseek(fd, seek, SEEK_SET) != seek)
		return sys_errmsg("cannot seek %s to offset %llu",
				  mtd->node, (unsigned long long)seek);

	ret = write(fd, buf, len);
	if (ret != len)
		return sys_errmsg("cannot write %d bytes to %s (eraseblock %d, offset %d)",
				  len, mtd->node, eb, offs);

	return 0;
}

/**
 * mtd_get_dev_info - fill the mtd_dev_info structure
 * @node: name of the MTD device node
 * @mtd: the MTD device information is returned here
 */
int mtd_get_dev_info(const char *node, struct mtd_dev_info *mtd)
{
	struct mtd_info_user ui;
	int fd, ret;
	loff_t offs = 0;

	memset(mtd, '\0', sizeof(struct mtd_dev_info));

	mtd->node = node;

	fd = open(node, O_RDWR);
	if (fd < 0)
		return sys_errmsg("cannot open \"%s\"", node);

	if (ioctl(fd, MEMGETINFO, &ui)) {
		sys_errmsg("MEMGETINFO ioctl request failed");
		goto out_close;
	}

	ret = ioctl(fd, MEMGETBADBLOCK, &offs);
	if (ret == -1) {
		if (errno != EOPNOTSUPP) {
			sys_errmsg("MEMGETBADBLOCK ioctl failed");
			goto out_close;
		}
		errno = 0;
		mtd->bb_allowed = 0;
	} else
		mtd->bb_allowed = 1;

	mtd->type = ui.type;
	mtd->size = ui.size;
	mtd->eb_size = ui.erasesize;
	mtd->min_io_size = ui.writesize;
	mtd->oob_size = ui.oobsize;

	if (mtd->min_io_size <= 0) {
		errmsg("%s has insane min. I/O unit size %d",
		       node, mtd->min_io_size);
		goto out_close;
	}
	if (mtd->eb_size <= 0 || mtd->eb_size < mtd->min_io_size) {
		errmsg("%s has insane eraseblock size %d",
		       node, mtd->eb_size);
		goto out_close;
	}
	if (mtd->size <= 0 || mtd->size < mtd->eb_size) {
		errmsg("%s has insane size %lld",
		       node, mtd->size);
		goto out_close;
	}

	mtd->eb_cnt = ui.size / ui.erasesize;

	switch(mtd->type) {
	case MTD_ABSENT:
		errmsg("%s (%s) is removable and is not present",
		       mtd->node, node);
		goto out_close;
	case MTD_RAM:
		strcpy((char *)mtd->type_str, "ram");
		break;
	case MTD_ROM:
		strcpy((char *)mtd->type_str, "rom");
		break;
	case MTD_NORFLASH:
		strcpy((char *)mtd->type_str, "nor");
		break;
	case MTD_NANDFLASH:
		strcpy((char *)mtd->type_str, "nand");
		break;
	case MTD_DATAFLASH:
		strcpy((char *)mtd->type_str, "dataflash");
		break;
	case MTD_UBIVOLUME:
		strcpy((char *)mtd->type_str, "ubi");
		break;
	default:
		goto out_close;
	}

	if (ui.flags & MTD_WRITEABLE)
		mtd->writable = 1;
	mtd->subpage_size = mtd->min_io_size;

	close(fd);

	return 0;

out_close:
	close(fd);
	return -1;
}
