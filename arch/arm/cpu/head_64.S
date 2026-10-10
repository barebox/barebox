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
	b 2f
	.org 0x20
	.asciz "barebox"
	.word 0xffffffff
	.word _barebox_image_size	/* image size to copy */
	.rept 8
	.word 0x55555555
	.endr
2:
	// detect which core we are on, and only allow core0 to pass thru to __pbl_board_entry
	mrs x0, mpidr_el1
	and x0, x0, #3
	cbz x0, 1f
0:
	// TODO, actual spin-tables, that allow the cores to be reclaimed
	wfe
	b 0b
1:
	b __pbl_board_entry
ENDPROC(__barebox_arm64_head)
