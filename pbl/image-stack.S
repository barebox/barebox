/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Linked into an image only if it references __pbl_image_stack_top */
/* Not .bss, as that is cleared while running on it */
.section .data.pbl_image_stack, "aw"
	.balign 16
	.skip 0x1000
.globl __pbl_image_stack_top
__pbl_image_stack_top:
