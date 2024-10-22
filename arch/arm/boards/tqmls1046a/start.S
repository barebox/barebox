/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>

#define STACK_TOP 0x10020000

ENTRY_PROC(start_tqmls1046a)
	mov x3, #STACK_TOP
	mov sp, x3
	b tqmls1046a_entry
ENTRY_PROC_END(start_tqmls1046a)

ENTRY_PROC(start_tqmls1046a_8g)
	mov x3, #STACK_TOP
	mov sp, x3
	b tqmls1046a_8g_entry
ENTRY_PROC_END(start_tqmls1046a_8g)
