/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>

ENTRY_PROC(start_dt_2nd)
	adr x1, stack
	mov sp, x1
	b dt_2nd_aarch64
.word 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
stack:
ENTRY_PROC_END(start_dt_2nd)
