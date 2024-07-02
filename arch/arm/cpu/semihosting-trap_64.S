/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/linkage.h>
#include <asm/unified.h>

.section .text.semihosting_trap
ENTRY(semihosting_trap)
	hlt	#0xf000
	ret
ENDPROC(semihosting_trap)
