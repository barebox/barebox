/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>
#include <asm/assembler64.h>

/*
 * OCRAM is occupied by TF-A, so in EL2 and EL1 use a temporary stack in SDRAM
 */
#define STACK_TOP		0x10020000
#define STACK_TOP_TMP_SDRAM	0x90000000

ENTRY_PROC(start_tqmls1046a)
	mov x3, #STACK_TOP
	mov sp, x3
	b tqmls1046a_entry
ENTRY_PROC_END(start_tqmls1046a)

ENTRY_PROC(start_tqmls1046a_8g)
	switch_el x3, 3f, 2f, 1f
3:
	mov x3, #STACK_TOP
	mov sp, x3
	b tqmls1046a_8g_entry
2:
1:
	mov x3, STACK_TOP_TMP_SDRAM
	mov sp, x3
	b tqmls1046a_8g_entry
ENTRY_PROC_END(start_tqmls1046a_8g)
