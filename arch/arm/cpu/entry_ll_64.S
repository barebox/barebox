/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <asm/sections.h>

/*
 * x0: memory base
 * x1: memory size
 * x2: board data
 * x3: new value for SP
 */
.section .text.__barebox_arm_entry
ENTRY(__barebox_arm_entry)
	mov	sp, x3
	b	barebox_pbl_start
ENDPROC(__barebox_arm_entry)
