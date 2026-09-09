/* SPDX-License-Identifier: GPL-2.0-only */
#include <linux/linkage.h>
#include <asm/assembler.h>

/*
 * Returns nonzero once init_clock() has set current_clock. Reads it
 * PC-relatively and uses no stack, so it works before relocation,
 * before BSS is cleared and before a stack is set up. Clobbers x0 only.
 */
.section .text.clocksource_registered, "ax"
ENTRY(clocksource_registered)
	ldr_l	x0, current_clock
	cmp	x0, #0
	cset	w0, ne
	ret
ENDPROC(clocksource_registered)
