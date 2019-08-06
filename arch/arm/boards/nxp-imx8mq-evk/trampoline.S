/* SPDX-License-Identifier: (GPL-2.0) */
#include <linux/linkage.h>
#include <asm/sections.h>
	.section .trampoline,"a"
	.globl  trampoline_start
trampoline_start:
	ldr	w19, trampoline_end
	br      x19
	.globl  trampoline_end
trampoline_end:
