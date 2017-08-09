#ifndef __FILE_TYPE_H
#define __FILE_TYPE_H

#include <linux/string.h>
#include <linux/types.h>

/*
 * List of file types we know
 */
enum filetype {
	filetype_unknown,
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
	filetype_kwbimage_v1,
	filetype_max,
};

#define FILE_TYPE_SAFE_BUFSIZE		2048

const char *file_type_to_string(enum filetype f);
const char *file_type_to_short_string(enum filetype f);
enum filetype file_detect_partition_table(const void *_buf, size_t bufsize);
enum filetype file_detect_type(const void *_buf, size_t bufsize);
enum filetype file_name_detect_type(const char *filename);
enum filetype cdev_detect_type(const char *name);
enum filetype is_fat_or_mbr(const unsigned char *sector, unsigned long *bootsec);
int is_fat_boot_sector(const void *_buf);
bool filetype_is_barebox_image(enum filetype ft);

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

#endif /* __FILE_TYPE_H */
