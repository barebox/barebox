/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 FORTH-ICS/CARV
 *  Nick Kossifidis <mick@ics.forth.gr>
 */

#include <asm/asm.h>
#include <asm/irq.h>
#include <linux/linkage.h>

.section .text.strap_entry
ENTRY(strap_entry)
	addi sp, sp, -PT_SIZE_ON_STACK
        pt_regs_push sp
	csrr t1, sstatus
	csrr t2, sepc
	csrr t3, stval
	csrr t4, scause

	REG_S t1, PT_STATUS(sp)
	REG_S t2, PT_EPC(sp)
	REG_S t3, PT_BADADDR(sp)
	REG_S t4, PT_CAUSE(sp)
	mv a0, sp
	jal handle_trap
	csrw sepc, a0
        pt_regs_pop sp
	addi sp, sp, PT_SIZE_ON_STACK
	sret
ENDPROC(strap_entry)
