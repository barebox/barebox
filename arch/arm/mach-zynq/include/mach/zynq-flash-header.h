#ifndef __MACH_FLASH_HEADER_H
#define __MACH_FLASH_HEADER_H

#include <asm-generic/sections.h>

#define __flash_header_section		__section(.flash_header_0x0)
#define __ps7reg_entry_section		__section(.ps7reg_entry_0x0A0)
#define __image_len_section		__section(.image_len_0x08c0)
#define FLASH_HEADER_OFFSET		0x0
#define IMAGE_OFFSET			0x8c0

#define DEST_BASE			0x8c0
#define FLASH_HEADER_BASE	(DEST_BASE + FLASH_HEADER_OFFSET)

struct zynq_reg_entry {
	__le32 addr;
	__le32 val;
};

#define WIDTH_DETECTION_MAGIC	0xAA995566
#define IMAGE_IDENTIFICATION	0x584C4E58	/* "XLNX" */

struct zynq_flash_header {
	__le32 width_det;
	__le32 image_id;
	__le32 enc_stat;
	__le32 user;
	__le32 flash_offset;
	__le32 length;
	__le32 res0;
	__le32 start_of_exec;
	__le32 total_len;
	__le32 res1;
	__le32 checksum;
	__le32 res2;
};

#endif /* __MACH_FLASH_HEADER_H */
