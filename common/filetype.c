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
#include <asm/unaligned.h>
#include <fcntl.h>
#include <fs.h>
#include <malloc.h>
#include <errno.h>

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
	[filetype_aimage] = "Android boot image",
	[filetype_sh] = "Bourne Shell",
	[filetype_mips_barebox] = "MIPS barebox image",
	[filetype_fat] = "FAT filesytem",
	[filetype_mbr] = "MBR sector",
	[filetype_bmp] = "BMP image",
	[filetype_png] = "PNG image",
};

const char *file_type_to_string(enum filetype f)
{
	if (f < ARRAY_SIZE(filetype_str))
		return filetype_str[f];

	return NULL;
}

#define MBR_StartSector		8	/* MBR: Offset of Starting Sector in Partition Table Entry */
#define BS_55AA			510	/* Boot sector signature (2) */
#define MBR_Table		446	/* MBR: Partition table offset (2) */
#define BS_FilSysType32		82	/* File system type (1) */
#define BS_FilSysType		54	/* File system type (1) */

enum filetype is_fat_or_mbr(const unsigned char *sector, unsigned long *bootsec)
{
	/*
	 * bootsec can be used to return index of the first sector in the
	 * first partition
	 */
	if (bootsec)
		*bootsec = 0;

	/*
	 * Check record signature (always placed at offset 510 even if the
	 * sector size is > 512)
	 */
	if (get_unaligned_le16(&sector[BS_55AA]) != 0xAA55)
		return filetype_unknown;

	/* Check "FAT" string */
	if ((get_unaligned_le32(&sector[BS_FilSysType]) & 0xFFFFFF) == 0x544146)
		return filetype_fat;

	if ((get_unaligned_le32(&sector[BS_FilSysType32]) & 0xFFFFFF) == 0x544146)
		return filetype_fat;

	if (bootsec)
		/*
		 * This must be an MBR, so return the starting sector of the
		 * first partition so we could check if there is a FAT boot
		 * sector there
		 */
		*bootsec = get_unaligned_le16(&sector[MBR_Table + MBR_StartSector]);

	return filetype_mbr;
}

enum filetype file_detect_type(void *_buf)
{
	u32 *buf = _buf;
	u64 *buf64 = _buf;
	u8 *buf8 = _buf;
	enum filetype type;

	if (strncmp(buf8, "#!/bin/sh", 9) == 0)
		return filetype_sh;
	if (is_barebox_arm_head(_buf))
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
	if (strncmp(buf8, "ANDROID!", 8) == 0)
		return filetype_aimage;
	if (strncmp(buf8 + 0x10, "barebox", 7) == 0)
		return filetype_mips_barebox;
	type = is_fat_or_mbr(buf8, NULL);
	if (type != filetype_unknown)
		return type;
	if (strncmp(buf8, "BM", 2) == 0)
		return filetype_bmp;
	if (buf64[0] == le64_to_cpu(0x0a1a0a0d474e5089ull))
		return filetype_png;

	return filetype_unknown;
}

enum filetype file_name_detect_type(const char *filename)
{
	int fd, ret;
	void *buf;
	enum filetype type = filetype_unknown;
	unsigned long bootsec;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return fd;

	buf = xzalloc(512);

	ret = read(fd, buf, 512);
	if (ret < 0)
		goto err_out;

	type = file_detect_type(buf);

	if (type == filetype_mbr) {
		/*
		 * Get the first partition start sector
		 * and check for FAT in it
		 */
		is_fat_or_mbr(buf, &bootsec);
		ret = lseek(fd, (bootsec) * 512, SEEK_SET);
		if (ret < 0)
			goto err_out;
		ret = read(fd, buf, 512);
		if (ret < 0)
			goto err_out;
		type = is_fat_or_mbr((u8 *)buf, NULL);
	}

err_out:
	close(fd);
	free(buf);

	return type;
}
