/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>
#include <asm/assembler64.h>

#define STACK_TOP 0x18040000

ENTRY_PROC(start_ls1028ardb)
	switch_el x3, 3f, 2f, 1f
3:
	mov x3, #STACK_TOP
	mov sp, x3
	b ls1028ardb_entry
2:
1:
	mov x3, 0x90000000
	mov sp, x3
	b ls1028ardb_entry
ENTRY_PROC_END(start_ls1028ardb)
