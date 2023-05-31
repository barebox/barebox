/* SPDX-License-Identifier: GPL-2.0-only */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>
#include <asm/image.h>

/* Linker will point these at board-specific symbols */
.globl __pbl_board_stack_top
.globl __pbl_board_entry

.section .text_head_prologue_common, "x"
ENTRY(__barebox_arm64_head)
	nop
	adr x9, __pbl_board_stack_top
	ldr x9, [x9]
	cbz x9, 1f
	mov sp, x9
1:
#ifdef CONFIG_PBL_BREAK
	brk #17
	nop
#else
	nop
	nop
#endif
	b __pbl_board_entry
	.org 0x20
	.asciz "barebox"
	.word 0xffffffff
	.word _barebox_image_size	/* image size to copy */
	.rept 8
	.word 0x55555555
	.endr
ENDPROC(__barebox_arm64_head)
