/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>
#include <asm/image.h>
#include "efi-header-aarch64.S"

#define IMAGE_FLAGS \
	(ARM64_IMAGE_FLAG_PAGE_SIZE_4K << ARM64_IMAGE_FLAG_PAGE_SIZE_SHIFT) | \
	(ARM64_IMAGE_FLAG_PHYS_BASE << ARM64_IMAGE_FLAG_PHYS_BASE_SHIFT)

.section .text_head_entry_start_dt_2nd
ENTRY("start_dt_2nd")
	efi_signature_nop          /* code0 */
	b 2f                       /* code1 */
	.xword 0x80000             /* Image load offset */
	.xword _barebox_image_size /* Effective Image size */
	.xword IMAGE_FLAGS	   /* Kernel flags */
	.xword 0                   /* reserved */
	.xword 0                   /* reserved */
	.xword 0                   /* reserved */
	.ascii ARM64_IMAGE_MAGIC   /* magic number */
	.int   .Lpe_header_offset  /* reserved (PE-COFF offset) */
	.asciz "barebox"	   /* unused for now */
2:
	adr x1, _text - .
	mov x5, #0			/* raw_entry */
	/* Stack now grows into the 0x80000 image load offset specified
	 * above. This is more than enough until FDT /memory is decoded.
	 */
#ifdef CONFIG_BOARD_GENERIC_DT_RAW_ENTRY
	/* Offset honoured iff _text ≡ 0x80000 (mod 1 MiB) */
	and x2, x1, #0xfffff
	cmp x2, #0x80000
	b.eq 4f
#else
	/* Offset can't have been honoured at address 0 */
	cbnz x1, 4f
#endif
	/* Else assume raw entry, stack 0x80000 past the image and FDT */
	mov x5, #1
	adrp x1, __image_end
	add x1, x1, :lo12:__image_end
	ldp w2, w3, [x1]			/* FDT magic, totalsize */
	mov w4, #0x0dd0
	movk w4, #0xedfe, lsl #16		/* FDT magic, read little-endian */
	rev w3, w3
	cmp w2, w4
	csel x3, x3, xzr, eq			/* skip FDT only if present */
	add x1, x1, x3
	add x1, x1, #0x80000
	and x1, x1, #~0xf
4:
	mov sp, x1
	mov x1, x5
	b dt_2nd_aarch64

	__EFI_PE_HEADER
ENTRY_PROC_END(start_dt_2nd)
