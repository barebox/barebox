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
	mov	r4, r0
	mov	r5, r1
	mov	r6, r2
	bl	arm_early_mmu_cache_invalidate
	mov	r0, r4
	mov	r1, r5
	mov	r2, r6
	b	barebox_pbl_start
ENDPROC(__barebox_arm_entry)
