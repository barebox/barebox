/*
 * filetype.c - detect filetypes
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 */
#include <common.h>
#include <filetype.h>
#include <asm/byteorder.h>
#include <fcntl.h>
#include <fs.h>
#include <malloc.h>

static const char *filetype_str[] = {
	[filetype_unknown] = "unknown",
	[filetype_arm_zimage] = "arm Linux zImage",
	[filetype_lzo_compressed] = "lzo compressed",
	[filetype_arm_barebox] = "arm barebox image",
	[filetype_uimage] = "U-Boot uImage",
	[filetype_ubi] = "UBI image",
	[filetype_jffs2] = "JFFS2 image",
	[filetype_gzip] = "gzip compressed",
	[filetype_bzip2] = "bzip2 compressed",
	[filetype_oftree] = "open firmware flat device tree",
};

const char *file_type_to_string(enum filetype f)
{
	if (f < ARRAY_SIZE(filetype_str))
		return filetype_str[f];

	return NULL;
}

enum filetype file_detect_type(void *_buf)
{
	u32 *buf = _buf;
	u8 *buf8 = _buf;

	if (buf[8] == 0x65726162 && buf[9] == 0x00786f62)
		return filetype_arm_barebox;
	if (buf[9] == 0x016f2818 || buf[9] == 0x18286f01)
		return filetype_arm_zimage;
	if (buf8[0] == 0x89 && buf8[1] == 0x4c && buf8[2] == 0x5a &&
			buf8[3] == 0x4f)
		return filetype_lzo_compressed;
	if (buf[0] == be32_to_cpu(0x27051956))
		return filetype_uimage;
	if (buf[0] == 0x23494255)
		return filetype_ubi;
	if (buf[0] == 0x20031985)
		return filetype_jffs2;
	if (buf8[0] == 0x1f && buf8[1] == 0x8b && buf8[2] == 0x08)
		return filetype_gzip;
	if (buf8[0] == 'B' && buf8[1] == 'Z' && buf8[2] == 'h' &&
			buf8[3] > '0' && buf8[3] <= '9')
                return filetype_bzip2;
	if (buf[0] == be32_to_cpu(0xd00dfeed))
		return filetype_oftree;

	return filetype_unknown;
}

enum filetype file_name_detect_type(const char *filename)
{
	int fd, ret;
	void *buf;
	enum filetype type = filetype_unknown;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return fd;

	buf = xzalloc(512);

	ret = read(fd, buf, 512);
	if (ret < 0)
		goto err_out;

	type = file_detect_type(buf);

err_out:
	close(fd);
	free(buf);

	return type;
}
