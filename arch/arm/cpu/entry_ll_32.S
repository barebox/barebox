/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <asm/sections.h>

/*
 * r0: memory base
 * r1: memory size
 * r2: board data
 * r3: new value for SP
 */
.section .text.__barebox_arm_entry
ENTRY(__barebox_arm_entry)
	mov	sp, r3
	b	barebox_pbl_start
ENDPROC(__barebox_arm_entry)
