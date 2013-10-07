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
 */
#include <common.h>
#include <filetype.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <fcntl.h>
#include <fs.h>
#include <malloc.h>
#include <errno.h>

struct filetype_str {
	const char *name;	/* human readable filetype */
	const char *shortname;	/* short string without spaces for shell scripts */
};

static const struct filetype_str filetype_str[] = {
	[filetype_unknown] = { "unknown", "unkown" },
	[filetype_arm_zimage] = { "arm Linux zImage", "arm-zimage" },
	[filetype_lzo_compressed] = { "lzo compressed", "lzo" },
	[filetype_lz4_compressed] = { "lz4 compressed", "lz4" },
	[filetype_arm_barebox] = { "arm barebox image", "arm-barebox" },
	[filetype_uimage] = { "U-Boot uImage", "u-boot" },
	[filetype_ubi] = { "UBI image", "ubi" },
	[filetype_jffs2] = { "JFFS2 image", "jffs2" },
	[filetype_gzip] = { "gzip compressed", "gzip" },
	[filetype_bzip2] = { "bzip2 compressed", "bzip2" },
	[filetype_oftree] = { "open firmware flat device tree", "dtb" },
	[filetype_aimage] = { "Android boot image", "android" },
	[filetype_sh] = { "Bourne Shell", "sh" },
	[filetype_mips_barebox] = { "MIPS barebox image", "mips-barebox" },
	[filetype_fat] = { "FAT filesytem", "fat" },
	[filetype_mbr] = { "MBR sector", "mbr" },
	[filetype_bmp] = { "BMP image", "bmp" },
	[filetype_png] = { "PNG image", "png" },
	[filetype_ext] = { "ext filesystem", "ext" },
	[filetype_gpt] = { "GUID Partition Table", "gpt" },
	[filetype_bpk] = { "Binary PacKage", "bpk" },
};

const char *file_type_to_string(enum filetype f)
{
	if (f < ARRAY_SIZE(filetype_str))
		return filetype_str[f].name;

	return NULL;
}

const char *file_type_to_short_string(enum filetype f)
{
	if (f < ARRAY_SIZE(filetype_str))
		return filetype_str[f].shortname;

	return NULL;
}

#define MBR_StartSector		8	/* MBR: Offset of Starting Sector in Partition Table Entry */
#define BS_55AA			510	/* Boot sector signature (2) */
#define MBR_Table		446	/* MBR: Partition table offset (2) */
#define MBR_partition_size	16	/* MBR: Partition table offset (2) */
#define BS_FilSysType32		82	/* File system type (1) */
#define BS_FilSysType		54	/* File system type (1) */

#define MBR_PART_sys_ind	4
#define MBR_PART_start_sect	8
#define MBR_OSTYPE_EFI_GPT	0xee

static inline int pmbr_part_valid(const uint8_t *buf)
{
	if (buf[MBR_PART_sys_ind] == MBR_OSTYPE_EFI_GPT &&
		get_unaligned_le32(&buf[MBR_PART_start_sect]) == 1UL) {
		return 1;
	}

	return 0;
}

/**
 * is_gpt_valid(): test Protective MBR for validity and EFI PART
 * @mbr: pointer to a legacy mbr structure
 *
 * Description: Returns 1 if PMBR is valid and EFI PART, 0 otherwise.
 * Validity depends on three things:
 *  1) MSDOS signature is in the last two bytes of the MBR
 *  2) One partition of type 0xEE is found
 *  3) EFI GPT signature is at offset 512
 */
static int is_gpt_valid(const uint8_t *buf)
{
	int i;

	if (get_unaligned_le16(&buf[BS_55AA]) != 0xAA55)
		return 0;

	if (strncmp(&buf[512], "EFI PART", 8))
		return 0;

	buf += MBR_Table;

	for (i = 0; i < 4; i++, buf += MBR_partition_size) {
		if (pmbr_part_valid(buf))
			return 1;
	}
	return 0;
}

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

enum filetype file_detect_partition_table(const void *_buf, size_t bufsize)
{
	const u8 *buf8 = _buf;
	enum filetype type;

	if (bufsize < 512)
		return filetype_unknown;

	/*
	 * EFI GPT need to be detected before MBR otherwise
	 * we will detect a MBR
	 */
	if (bufsize >= 520 && is_gpt_valid(buf8))
		return filetype_gpt;

	type = is_fat_or_mbr(buf8, NULL);
	if (type != filetype_unknown)
		return type;

	return filetype_unknown;
}

enum filetype file_detect_type(const void *_buf, size_t bufsize)
{
	const u32 *buf = _buf;
	const u64 *buf64 = _buf;
	const u8 *buf8 = _buf;
	const u16 *buf16 = _buf;
	enum filetype type;

	if (bufsize < 9)
		return filetype_unknown;

	if (strncmp(buf8, "#!/bin/sh", 9) == 0)
		return filetype_sh;

	if (bufsize < 32)
		return filetype_unknown;

	if (strncmp(buf8, "BM", 2) == 0)
		return filetype_bmp;
	if (buf8[0] == 0x89 && buf8[1] == 0x4c && buf8[2] == 0x5a &&
			buf8[3] == 0x4f)
		return filetype_lzo_compressed;
	if (buf8[0] == 0x02 && buf8[1] == 0x21 && buf8[2] == 0x4c &&
			buf8[3] == 0x18)
		return filetype_lz4_compressed;
	if (buf[0] == be32_to_cpu(0x27051956))
		return filetype_uimage;
	if (buf[0] == 0x23494255)
		return filetype_ubi;
	if (buf[0] == le32_to_cpu(0x06101831))
		return filetype_ubifs;
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
	if (buf64[0] == le64_to_cpu(0x0a1a0a0d474e5089ull))
		return filetype_png;
	if (is_barebox_mips_head(_buf))
		return filetype_mips_barebox;
	if (buf[0] == be32_to_cpu(0x534F4659))
		return filetype_bpk;

	if (bufsize < 64)
		return filetype_unknown;

	if (is_barebox_arm_head(_buf))
		return filetype_arm_barebox;
	if (buf[9] == 0x016f2818 || buf[9] == 0x18286f01)
		return filetype_arm_zimage;

	if (bufsize < 512)
		return filetype_unknown;

	type = file_detect_partition_table(_buf, bufsize);
	if (type != filetype_unknown)
		return type;

	if (bufsize >= 1536 && buf16[512 + 28] == le16_to_cpu(0xef53))
		return filetype_ext;

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

	buf = xzalloc(FILE_TYPE_SAFE_BUFSIZE);

	ret = read(fd, buf, FILE_TYPE_SAFE_BUFSIZE);
	if (ret < 0)
		goto err_out;

	type = file_detect_type(buf, ret);

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
