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
#include <libfile.h>
#include <malloc.h>
#include <errno.h>
#include <envfs.h>
#include <disks.h>
#include <image-sparse.h>
#include <elf.h>

#include <arm/mach-imx/include/mach/imx-header.h>

struct filetype_str {
	const char *name;	/* human readable filetype */
	const char *shortname;	/* short string without spaces for shell scripts */
};

static const struct filetype_str filetype_str[] = {
	[filetype_unknown] = { "unknown", "unknown" },
	[filetype_arm_zimage] = { "ARM Linux zImage", "arm-zimage" },
	[filetype_lzo_compressed] = { "LZO compressed", "lzo" },
	[filetype_lz4_compressed] = { "LZ4 compressed", "lz4" },
	[filetype_arm_barebox] = { "ARM barebox image", "arm-barebox" },
	[filetype_uimage] = { "U-Boot uImage", "u-boot" },
	[filetype_ubi] = { "UBI image", "ubi" },
	[filetype_jffs2] = { "JFFS2 image", "jffs2" },
	[filetype_squashfs] = { "Squashfs image", "squashfs" },
	[filetype_gzip] = { "GZIP compressed", "gzip" },
	[filetype_bzip2] = { "BZIP2 compressed", "bzip2" },
	[filetype_oftree] = { "open firmware Device Tree flattened Binary", "dtb" },
	[filetype_aimage] = { "android boot image", "android" },
	[filetype_sh] = { "bourne SHell", "sh" },
	[filetype_mips_barebox] = { "MIPS barebox image", "mips-barebox" },
	[filetype_fat] = { "FAT filesytem", "fat" },
	[filetype_mbr] = { "MBR sector", "mbr" },
	[filetype_bmp] = { "BMP image", "bmp" },
	[filetype_png] = { "PNG image", "png" },
	[filetype_ext] = { "EXT filesystem", "ext" },
	[filetype_gpt] = { "GUID Partition Table", "gpt" },
	[filetype_ubifs] = { "UBIFS image", "ubifs" },
	[filetype_bpk] = { "Binary PacKage", "bpk" },
	[filetype_barebox_env] = { "barebox environment file", "bbenv" },
	[filetype_ch_image] = { "TI OMAP CH boot image", "ch-image" },
	[filetype_ch_image_be] = {
			"TI OMAP CH boot image (big endian)", "ch-image-be" },
	[filetype_xz_compressed] = { "XZ compressed", "xz" },
	[filetype_exe] = { "MS-DOS executable", "exe" },
	[filetype_mxs_bootstream] = { "Freescale MXS bootstream", "mxsbs" },
	[filetype_socfpga_xload] = { "SoCFPGA prebootloader image", "socfpga-xload" },
	[filetype_kwbimage_v0] = { "MVEBU kwbimage (v0)", "kwb0" },
	[filetype_kwbimage_v1] = { "MVEBU kwbimage (v1)", "kwb1" },
	[filetype_android_sparse] = { "Android sparse image", "sparse" },
	[filetype_arm64_linux_image] = { "ARM aarch64 Linux image", "aarch64-linux" },
	[filetype_elf] = { "ELF", "elf" },
	[filetype_imx_image_v1] = { "i.MX image (v1)", "imx-image-v1" },
	[filetype_imx_image_v2] = { "i.MX image (v2)", "imx-image-v2" },
	[filetype_layerscape_image] = { "Layerscape image", "layerscape-PBL" },
	[filetype_layerscape_qspi_image] = { "Layerscape QSPI image", "layerscape-qspi-PBL" },
	[filetype_ubootvar] = { "U-Boot environmemnt variable data",
				"ubootvar" },
	[filetype_stm32_image_v1] = { "STM32 image (v1)", "stm32-image-v1" },
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

#define FAT_BS_reserved		14
#define FAT_BS_fats		16
#define FAT_BS_media		21

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

static inline int fat_valid_media(u8 media)
{
	return (0xf8 <= media || media == 0xf0);
}

static int is_fat_with_no_mbr(const unsigned char  *sect)
{
	if (!get_unaligned_le16(&sect[FAT_BS_reserved]))
		return 0;

	if (!sect[FAT_BS_fats])
		return 0;

	if (!fat_valid_media(sect[FAT_BS_media]))
		return 0;

	return 1;
}

int is_fat_boot_sector(const void *sect)
{
	struct partition_entry *p;
	int slot;

	p = (struct partition_entry *) (sect + MBR_Table);
	/* max 4 partitions */
	for (slot = 1; slot <= 4; slot++, p++) {
		if (p->boot_indicator && p->boot_indicator != 0x80) {
			/*
			 * Even without a valid boot inidicator value
			 * its still possible this is valid FAT filesystem
			 * without a partition table.
			 */
			if (slot == 1 && is_fat_with_no_mbr(sect))
				return 1;
			 else
				return -EINVAL;
		}
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

#define CH_TOC_section_name     0x14

enum filetype file_detect_type(const void *_buf, size_t bufsize)
{
	const u32 *buf = _buf;
	const u64 *buf64 = _buf;
	const u8 *buf8 = _buf;
	const u16 *buf16 = _buf;
	const struct imx_flash_header *imx_flash_header = _buf;
	enum filetype type;

	if (bufsize < 9)
		return filetype_unknown;

	if (strncmp(buf8, "#!/bin/sh", 9) == 0)
		return filetype_sh;
	if (buf[0] == ENVFS_32(ENVFS_MAGIC))
		return filetype_barebox_env;

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
	if (buf8[0] == 0xfd && buf8[1] == 0x37 && buf8[2] == 0x7a &&
			buf8[3] == 0x58 && buf8[4] == 0x5a && buf8[5] == 0x00)
		return filetype_xz_compressed;
	if (buf8[0] == 'h' && buf8[1] == 's' && buf8[2] == 'q' &&
			buf8[3] == 's')
		return filetype_squashfs;
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
	if (le32_to_cpu(buf[14]) == 0x644d5241)
		return filetype_arm64_linux_image;
	if ((buf8[0] == 0x5a || buf8[0] == 0x69 || buf8[0] == 0x78 ||
	     buf8[0] == 0x8b || buf8[0] == 0x9c) &&
	    buf8[0x1] == 0 && buf8[0x2] == 0 && buf8[0x3] == 0 &&
	    buf8[0x18] == 0 && buf8[0x1b] == 0 && buf8[0x1c] == 0) {
		unsigned char sum = 0;
		int i;

		for (i = 0; i <= 0x1e; ++i)
			sum += buf8[i];

		if (sum == buf8[0x1f] && buf8[0x8] == 0)
			return filetype_kwbimage_v0;

		if (sum == buf8[0x1f] &&
		    buf8[0x8] == 1 && buf8[0x1d] == 0 &&
		    (buf8[0x1e] == 0 || buf8[0x1e] == 1))
			return filetype_kwbimage_v1;
	}

	if (is_sparse_image(_buf))
		return filetype_android_sparse;

	if (buf[0] == 0x55aa55aa && buf[1] == 0x0001ee01)
		return filetype_layerscape_image;
	if (buf[0] == 0x01ee0100 && buf[1] == 0xaa55aa55)
		return filetype_layerscape_qspi_image;

	if (bufsize < 64)
		return filetype_unknown;

	if (le32_to_cpu(buf[5]) == 0x504d5453)
		return filetype_mxs_bootstream;

	if (buf[16] == 0x31305341)
		return filetype_socfpga_xload;

	if (is_barebox_arm_head(_buf))
		return filetype_arm_barebox;
	if (buf[9] == 0x016f2818 || buf[9] == 0x18286f01)
		return filetype_arm_zimage;

	if (buf8[0] == 'M' && buf8[1] == 'Z')
		return filetype_exe;

	if (bufsize < 256)
		return filetype_unknown;

	if (strncmp(buf8, "STM\x32", 4) == 0) {
		if (buf8[74] == 0x01)
			return filetype_stm32_image_v1;
	}

	if (bufsize < 512)
		return filetype_unknown;

	type = file_detect_partition_table(_buf, bufsize);
	if (type != filetype_unknown)
		return type;

	if (bufsize >= 1536 && buf16[512 + 28] == le16_to_cpu(0xef53))
		return filetype_ext;

	if (strncmp(buf8 + CH_TOC_section_name, "CHSETTINGS", 10) == 0)
		return filetype_ch_image;

	if (buf[5] == 0x43485345 && buf[6] == 0x5454494E &&
		buf[7] == 0x47530000)
		return filetype_ch_image_be;

	if (strncmp(buf8, ELFMAG, 4) == 0)
		return filetype_elf;

	if (imx_flash_header->dcd_barker == DCD_BARKER)
		return filetype_imx_image_v1;

	if (is_imx_flash_header_v2(_buf))
		return filetype_imx_image_v2;

	return filetype_unknown;
}

enum filetype file_name_detect_type_offset(const char *filename, loff_t pos)
{
	int fd, ret;
	void *buf;
	enum filetype type = filetype_unknown;

	fd = open_and_lseek(filename, O_RDONLY, pos);
	if (fd < 0)
		goto out;

	buf = xzalloc(FILE_TYPE_SAFE_BUFSIZE);

	ret = read(fd, buf, FILE_TYPE_SAFE_BUFSIZE);
	if (ret < 0)
		goto err_out;

	type = file_detect_type(buf, ret);

err_out:
	close(fd);
	free(buf);
out:
	return type;
}

enum filetype file_name_detect_type(const char *filename)
{
	return file_name_detect_type_offset(filename, 0);
}

enum filetype cdev_detect_type(const char *name)
{
	enum filetype type = filetype_unknown;
	int ret;
	struct cdev *cdev;
	void *buf;

	cdev = cdev_open(name, O_RDONLY);
	if (!cdev)
		return type;

	if (cdev->filetype != filetype_unknown) {
		type = cdev->filetype;
		goto cdev_close;
	}

	buf = xzalloc(FILE_TYPE_SAFE_BUFSIZE);
	ret = cdev_read(cdev, buf, FILE_TYPE_SAFE_BUFSIZE, 0, 0);
	if (ret < 0)
		goto err_out;

	type = file_detect_type(buf, ret);

err_out:
	free(buf);
cdev_close:
	cdev_close(cdev);
	return type;
}

bool filetype_is_barebox_image(enum filetype ft)
{
	switch (ft) {
	case filetype_arm_barebox:
	case filetype_mips_barebox:
	case filetype_ch_image:
	case filetype_ch_image_be:
	case filetype_layerscape_image:
	case filetype_layerscape_qspi_image:
		return true;
	default:
		return false;
	}
}
