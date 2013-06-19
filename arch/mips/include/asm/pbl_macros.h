/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2011, 2012 Antony Pavlov <antonynpavlov@gmail.com>
 * ADR macro copyrighted (C) 2009 by Shinya Kuribayashi <skuribay@pobox.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_PBL_MACROS_H
#define __ASM_PBL_MACROS_H

#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm-generic/memory_layout.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>

	.macro	pbl_sleep reg count
	.set push
	.set noreorder
	li	\reg, \count
254:
	bgtz	\reg, 254b
	 addi	\reg, -1
	.set	pop
	.endm

	.macro	pbl_probe_mem ret1 ret2 addr
	.set	push
	.set	noreorder
	la	\ret1, \addr
	sw	zero, 0(\ret1)
	li	\ret2, 0x12345678
	sw	\ret2, 0(\ret1)
	lw	\ret2, 0(\ret1)
	li	\ret1, 0x12345678
	.set	pop
	.endm

	/*
	 * ADR macro instruction (inspired by ARM)
	 *
	 * ARM architecture doesn't have PC-relative jump instruction
	 * like MIPS' B/BAL insns. When ARM makes PC-relative jumps,
	 * it uses ADR insn. ADR is used to get a destination address
	 * of 'label' against current PC. With this, ARM can safely
	 * make PC-relative jumps.
	 */
	.macro	ADR rd label temp
	.set	push
	.set	noreorder
	move	\temp, ra			# preserve ra beforehand
	bal	255f
	 nop
255:	addiu	\rd, ra, \label - 255b		# label is assumed to be
	move	ra, \temp			# within pc +/- 32KB
	.set	pop
	.endm

	.macro	copy_to_link_location start_addr
	.set	push
	.set	noreorder

	/* copy barebox to link location */
	ADR	a0, \start_addr, t1	/* a0 <- pc-relative
					position of start_addr */

	la	a1, \start_addr	/* a1 <- link (RAM) start_addr address */

	beq	a0, a1, copy_loop_exit
	 nop

	la	t0, \start_addr
	la	t1, __bss_start
	subu	t2, t1, t0	/* t2 <- size of pbl */
	addu	a2, a0, t2	/* a2 <- source end address */

copy_loop:
	/* copy from source address [a0] */
	lw	t4, LONGSIZE * 0(a0)
	lw	t5, LONGSIZE * 1(a0)
	lw	t6, LONGSIZE * 2(a0)
	lw	t7, LONGSIZE * 3(a0)
	/* copy to target address [a1] */
	sw	t4, LONGSIZE * 0(a1)
	sw	t5, LONGSIZE * 1(a1)
	sw	t6, LONGSIZE * 2(a1)
	sw	t7, LONGSIZE * 3(a1)
	addi	a0, LONGSIZE * 4
	subu	t3, a0, a2
	blez	t3, copy_loop
	 addi	a1, LONGSIZE * 4

copy_loop_exit:

	.set	pop
	.endm

	.macro	mips_disable_interrupts
	.set	push
	.set	noreorder
	mfc0	k0, CP0_STATUS
	li	k1, ~ST0_IE
	and	k0, k1
	mtc0	k0, CP0_STATUS
	.set	pop
	.endm

	.macro	mips_barebox_10h
	.set	push
	.set	noreorder

	b	1f
	 nop

	.org	0x10
	.ascii	"barebox " UTS_RELEASE " " UTS_VERSION
	.byte	0

	.align	4
1:
	.set	pop
	.endm

	/*
	 * Dominic Sweetman, See MIPS Run, Morgan Kaufmann, 2nd edition, 2006
	 *
	 * 11.2.2 Stack Argument Structure in o32
	 * ...
	 * At the point where a function is called, sp must be
	 * eight-byte-aligned, matching the alignment of the largest
	 * basic types -- a long long integer or a floating-point double.
	 * The eight-byte alignment is not required by 32-bit MIPS integer
	 * hardware, but it's essential for compatibility with CPUs with
	 * 64-bit registers, and thus part of the rules. Subroutines fit
	 * in with this by always adjusting the stack pointer by a multiple
	 * of eight.
	 * ...
	 * SGI's n32 and n64 standards call for the stack to be maintained
	 * with 16-byte alignment.
	 *
	 */

#if (STACK_BASE + STACK_SIZE) % 16 != 0
#error stack pointer must be 16-byte-aligned
#endif

	.macro	stack_setup
	.set	push
	.set	noreorder

	/* set stack pointer; reserve four 32-bit argument slots */
	la	sp, STACK_BASE + STACK_SIZE - 16

	.set	pop
	.endm

#endif /* __ASM_PBL_MACROS_H */
