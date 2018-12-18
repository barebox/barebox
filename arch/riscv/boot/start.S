/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for RISC-V CPU
 *
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on coreboot/src/arch/riscv/bootblock.S
 *      and barebox/arch/mips/boot/start.S
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm-generic/memory_layout.h>

#include "mach/debug_ll.h"

#include "asm/riscv_nmon.h"

	.text
	.section ".text_entry"
	.align 2

.globl _start
_start:
	debug_ll_ns16550_init

	riscv_nmon

	li	sp, STACK_BASE + STACK_SIZE

	/* copy barebox to link location */

	la	a0, _start	/* a0 <- _start actual address */
	li	a1, CONFIG_TEXT_BASE	/* a1 <- _start link address */

	beq	a0, a1, main_entry

	la	a2, __bss_start

#define LONGSIZE 4

copy_loop:
	/* copy from source address [a0] */
	lw	t0, LONGSIZE * 0(a0)
	lw	t1, LONGSIZE * 1(a0)
	lw	t2, LONGSIZE * 2(a0)
	lw	t3, LONGSIZE * 3(a0)
	/* copy to target address [a1] */
	sw	t0, LONGSIZE * 0(a1)
	sw	t1, LONGSIZE * 1(a1)
	sw	t2, LONGSIZE * 2(a1)
	sw	t3, LONGSIZE * 3(a1)
	addi	a0, a0, LONGSIZE * 4
	addi	a1, a1, LONGSIZE * 4
	bgeu	a2, a0, copy_loop

	/* Alas! At the moment I can't load main_entry __link__ address
	   into a0 with la. Use CONFIG_TEXT_BASE instead. This solution
	   leads to extra cycles for repeat sp initialization. */

	li	a0, CONFIG_TEXT_BASE
	jalr	a0
