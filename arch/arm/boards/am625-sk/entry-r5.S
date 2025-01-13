/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <linux/linkage.h>

#define SRAM_BASE 0x43c00000

#define STACK_TOP SRAM_BASE + 0x3d000

.arm
.section .text_head_entry_start_am625_sk_r5;

ENTRY(start_am625_sk_r5)
        ldr r3, =STACK_TOP
        mov sp, r3
        ldr pc, _reset_am625_sk

_reset_am625_sk: .word SRAM_BASE + am625_sk_r5_entry

ENDPROC(start_am625_sk_r5)

.section .text_head_entry_start_am625sip_sk_r5;

ENTRY(start_am625sip_sk_r5)
        ldr r3, =STACK_TOP
        mov sp, r3
        ldr pc, _reset_am625sip_sk

_reset_am625sip_sk: .word SRAM_BASE + am625sip_sk_r5_entry

ENDPROC(start_am625sip_sk_r5)
