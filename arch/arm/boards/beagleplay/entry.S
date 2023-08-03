/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>
#include <asm/image.h>

#define IMAGE_FLAGS \
	(ARM64_IMAGE_FLAG_PAGE_SIZE_4K << ARM64_IMAGE_FLAG_PAGE_SIZE_SHIFT) | \
	(ARM64_IMAGE_FLAG_PHYS_BASE << ARM64_IMAGE_FLAG_PHYS_BASE_SHIFT)

.section .text_head_entry_start_beagleplay
ENTRY("start_beagleplay")
	adr x1, 0	   	   /* code0 */
	b 2f                       /* code1 */
	.xword 0x80000             /* Image load offset */
	.xword _barebox_image_size /* Effective Image size */
	.xword IMAGE_FLAGS	   /* Kernel flags */
	.xword 0                   /* reserved */
	.xword 0                   /* reserved */
	.xword 0                   /* reserved */
	.ascii ARM64_IMAGE_MAGIC   /* magic number */
	.int   0                   /* reserved (PE-COFF offset) */
	.asciz "barebox"	   /* unused for now */
2:
	mov sp, x1
	/* Stack now grows into the 0x80000 image load offset specified
	 * above. This is more than enough until FDT /memory is decoded.
	 */
	b beagleplay
ENTRY_PROC_END(start_beagleplay)
