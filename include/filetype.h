/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __FILE_TYPE_H
#define __FILE_TYPE_H

#include <linux/string.h>
#include <linux/types.h>
#include <asm/byteorder.h>

/*
 * List of file types we know
 */
enum filetype {
	filetype_unknown,
	filetype_empty,
	filetype_arm_zimage,
	filetype_lzo_compressed,
	filetype_lz4_compressed,
	filetype_arm_barebox,
	filetype_uimage,
	filetype_ubi,
	filetype_jffs2,
	filetype_squashfs,
	filetype_gzip,
	filetype_bzip2,
	filetype_oftree,
	filetype_aimage,
	filetype_sh,
	filetype_mips_barebox,
	filetype_fat,
	filetype_mbr,
	filetype_bmp,
	filetype_png,
	filetype_qoi,
	filetype_ext,
	filetype_gpt,
	filetype_ubifs,
	filetype_bpk,
	filetype_barebox_env,
	filetype_ch_image,
	filetype_ch_image_be,
	filetype_exe,
	filetype_xz_compressed,
	filetype_mxs_bootstream,
	filetype_socfpga_xload,
	filetype_kwbimage_v0,
	filetype_kwbimage_v1,
	filetype_android_sparse,
	filetype_arm64_linux_image,
	filetype_arm64_efi_linux_image,
	filetype_riscv_linux_image,
	filetype_riscv_efi_linux_image,
	filetype_riscv_barebox_image,
	filetype_elf,
	filetype_imx_image_v1,
	filetype_imx_image_v2,
	filetype_imx_image_v3,
	filetype_layerscape_image,
	filetype_layerscape_qspi_image,
	filetype_ubootvar,
	filetype_stm32_image_fsbl_v1,
	filetype_stm32_image_ssbl_v1,
	filetype_zynq_image,
	filetype_mxs_sd_image,
	filetype_rockchip_rkns_image,
	filetype_fip,
	filetype_qemu_fw_cfg,
	filetype_nxp_fspi_image,
	filetype_zstd_compressed,
	filetype_max,
};

#define FILE_TYPE_SAFE_BUFSIZE		2048

struct cdev;

const char *file_type_to_string(enum filetype f);
const char *file_type_to_short_string(enum filetype f);
enum filetype file_detect_partition_table(const void *_buf, size_t bufsize);
enum filetype file_detect_compression_type(const void *_buf, size_t bufsize);
enum filetype file_detect_fs_type(const void *_buf, size_t bufsize);
enum filetype file_detect_type(const void *_buf, size_t bufsize);
int file_name_detect_type(const char *filename, enum filetype *type);
int file_name_detect_type_offset(const char *filename, loff_t pos, enum filetype *type,
				 enum filetype (*detect)(const void *buf, size_t bufsize));
int cdev_detect_type(struct cdev *cdev, enum filetype *type);
enum filetype is_fat_or_mbr(const unsigned char *sector, unsigned long *bootsec);
int is_fat_boot_sector(const void *_buf);
bool filetype_is_barebox_image(enum filetype ft);

static inline bool file_is_compressed_file(enum filetype ft)
{
	switch (ft) {
	case filetype_lzo_compressed:
	case filetype_lz4_compressed:
	case filetype_gzip:
	case filetype_bzip2:
	case filetype_xz_compressed:
	case filetype_zstd_compressed:
		return true;
	default:
		return false;
	}
}

#define ARM_HEAD_SIZE			0x30
#define ARM_HEAD_MAGICWORD_OFFSET	0x20
#define ARM_HEAD_SIZE_OFFSET		0x2C

#ifdef CONFIG_ARM
static inline int is_barebox_arm_head(const char *head)
{
	return !strcmp(head + ARM_HEAD_MAGICWORD_OFFSET, "barebox");
}
#else
static inline int is_barebox_arm_head(const char *head)
{
	return 0;
}
#endif

#define MIPS_HEAD_SIZE			0x20
#define MIPS_HEAD_MAGICWORD_OFFSET	0x10
#define MIPS_HEAD_SIZE_OFFSET		0x1C

#ifdef CONFIG_MIPS
static inline int is_barebox_mips_head(const char *head)
{
	return !strncmp(head + MIPS_HEAD_MAGICWORD_OFFSET, "barebox", 7);
}
#else
static inline int is_barebox_mips_head(const char *head)
{
	return 0;
}
#endif

static inline int is_barebox_head(const char *head)
{
	return is_barebox_arm_head(head) || is_barebox_mips_head(head);
}

static inline bool is_arm64_linux_bootimage(const void *header)
{
	return le32_to_cpup(header + 56) == 0x644d5241;
}

static inline bool is_riscv_linux_bootimage(const void *header)
{
	return le32_to_cpup(header + 56) == 0x05435352;
}

#endif /* __FILE_TYPE_H */
