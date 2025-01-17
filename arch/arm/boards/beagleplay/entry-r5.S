/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>

#define SRAM_BASE 0x43c00000

#define STACK_TOP SRAM_BASE + 0x3d000

.arm
.section .text_head_entry_start_beagleplay_r5;

ENTRY(start_beagleplay_r5)
        ldr r3, =STACK_TOP
        mov sp, r3
        ldr pc, _reset

_reset: .word SRAM_BASE + beagleplay_r5_entry

ENDPROC(start_beagleplay_r5)
