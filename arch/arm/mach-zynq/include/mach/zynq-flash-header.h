#ifndef __MACH_FLASH_HEADER_H
#define __MACH_FLASH_HEADER_H

#include <stdint.h>

#define REGINIT_OFFSET			0x0a0
#define IMAGE_OFFSET			0x8c0

#define WIDTH_DETECTION_MAGIC	0xAA995566
#define IMAGE_IDENTIFICATION	0x584C4E58	/* "XLNX" */

struct zynq_flash_header {
	uint32_t width_det;
	uint32_t image_id;
	uint32_t enc_stat;
	uint32_t user;
	uint32_t flash_offset;
	uint32_t length;
	uint32_t res0;
	uint32_t start_of_exec;
	uint32_t total_len;
	uint32_t res1;
	uint32_t checksum;
	uint32_t res2;
};

#endif /* __MACH_FLASH_HEADER_H */
