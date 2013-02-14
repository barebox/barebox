/*
 * Copyright (C) 2008, 2009 Nokia Corporation
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
 * Author: Artem Bityutskiy
 *
 * MTD library.
 */

#ifndef __LIBMTD_H__
#define __LIBMTD_H__

/* Maximum MTD device name length */
#define MTD_NAME_MAX 127
/* Maximum MTD device type string length */
#define MTD_TYPE_MAX 64

/**
 * struct mtd_dev_info - information about an MTD device.
 * @node: node pointing to device
 * @type: flash type (constants like %MTD_NANDFLASH defined in mtd-abi.h)
 * @type_str: static R/O flash type string
 * @name: device name
 * @size: device size in bytes
 * @eb_cnt: count of eraseblocks
 * @eb_size: eraseblock size
 * @min_io_size: minimum input/output unit size
 * @subpage_size: sub-page size
 * @oob_size: OOB size (zero if the device does not have OOB area)
 * @region_cnt: count of additional erase regions
 * @writable: zero if the device is read-only
 * @bb_allowed: non-zero if the MTD device may have bad eraseblocks
 */
struct mtd_dev_info
{
	const char *node;
	int type;
	const char type_str[MTD_TYPE_MAX + 1];
	long long size;
	int eb_cnt;
	int eb_size;
	int min_io_size;
	int subpage_size;
	int oob_size;
	int region_cnt;
	unsigned int writable:1;
	unsigned int bb_allowed:1;
};

/**
 * mtd_get_dev_info - get information about an MTD device.
 * @desc: MTD library descriptor
 * @node: name of the MTD device node
 * @mtd: the MTD device information is returned here
 *
 * This function gets information about MTD device defined by the @node device
 * node file and saves this information in the @mtd object. Returns %0 in case
 * of success and %-1 in case of failure. If MTD subsystem is not present in the
 * system, or the MTD device does not exist, errno is set to @ENODEV.
 */
int mtd_get_dev_info(const char *node, struct mtd_dev_info *mtd);

/**
 * libmtd_erase - erase an eraseblock.
 * @desc: MTD library descriptor
 * @mtd: MTD device description object
 * @fd: MTD device node file descriptor
 * @eb: eraseblock to erase
 *
 * This function erases eraseblock @eb of MTD device described by @fd. Returns
 * %0 in case of success and %-1 in case of failure.
 */
int libmtd_erase(const struct mtd_dev_info *mtd, int fd, int eb);

/**
 * mtd_torture - torture an eraseblock.
 * @desc: MTD library descriptor
 * @mtd: MTD device description object
 * @fd: MTD device node file descriptor
 * @eb: eraseblock to torture
 *
 * This function tortures eraseblock @eb. Returns %0 in case of success and %-1
 * in case of failure.
 */
int mtd_torture(const struct mtd_dev_info *mtd, int fd, int eb);

/**
 * mtd_is_bad - check if eraseblock is bad.
 * @mtd: MTD device description object
 * @fd: MTD device node file descriptor
 * @eb: eraseblock to check
 *
 * This function checks if eraseblock @eb is bad. Returns %0 if not, %1 if yes,
 * and %-1 in case of failure.
 */
int mtd_is_bad(const struct mtd_dev_info *mtd, int fd, int eb);

/**
 * mtd_mark_bad - mark an eraseblock as bad.
 * @mtd: MTD device description object
 * @fd: MTD device node file descriptor
 * @eb: eraseblock to mark as bad
 *
 * This function marks eraseblock @eb as bad. Returns %0 in case of success and
 * %-1 in case of failure.
 */
int mtd_mark_bad(const struct mtd_dev_info *mtd, int fd, int eb);

/**
 * mtd_read - read data from an MTD device.
 * @mtd: MTD device description object
 * @fd: MTD device node file descriptor
 * @eb: eraseblock to read from
 * @offs: offset withing the eraseblock to read from
 * @buf: buffer to read data to
 * @len: how many bytes to read
 *
 * This function reads @len bytes of data from eraseblock @eb and offset @offs
 * of the MTD device defined by @mtd and stores the read data at buffer @buf.
 * Returns %0 in case of success and %-1 in case of failure.
 */
int libmtd_read(const struct mtd_dev_info *mtd, int fd, int eb, int offs,
	     void *buf, int len);

/**
 * mtd_write - write data to an MTD device.
 * @mtd: MTD device description object
 * @fd: MTD device node file descriptor
 * @eb: eraseblock to write to
 * @offs: offset withing the eraseblock to write to
 * @buf: buffer to write
 * @len: how many bytes to write
 *
 * This function writes @len bytes of data to eraseblock @eb and offset @offs
 * of the MTD device defined by @mtd. Returns %0 in case of success and %-1 in
 * case of failure.
 */
int libmtd_write(const struct mtd_dev_info *mtd, int fd, int eb, int offs,
	      void *buf, int len);

#endif /* __LIBMTD_H__ */
