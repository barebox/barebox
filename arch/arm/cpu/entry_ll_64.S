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
	mov	x19, x0
	mov	x20, x1
	mov	x21, x2
	bl	arm_early_mmu_cache_invalidate
	mov	x0, x19
	mov	x1, x20
	mov	x2, x21
	b	barebox_pbl_start
ENDPROC(__barebox_arm_entry)
