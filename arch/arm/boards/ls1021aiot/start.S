/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>

#define STACK_TOP 0x10020000

ENTRY_PROC(start_ls1021aiot)
	ldr r3, =STACK_TOP
	mov sp, r3
	b ls1021aiot_entry
ENTRY_PROC_END(start_ls1021aiot)
