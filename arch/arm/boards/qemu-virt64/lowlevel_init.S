/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>
#include <asm/barebox-arm64.h>

/* The DRAM is already setup */
#define STACK_TOP 0x80000000

ENTRY_PROC(barebox_arm_reset_vector)
	mov x0, #STACK_TOP
	mov sp, x0
	b qemu_virt64_start
ENTRY_PROC_END(barebox_arm_reset_vector)
