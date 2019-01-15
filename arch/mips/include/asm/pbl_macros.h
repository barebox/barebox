/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2011, 2012 Antony Pavlov <antonynpavlov@gmail.com>
 * ADR macro copyrighted (C) 2009 by Shinya Kuribayashi <skuribay@pobox.com>
 */

#ifndef __ASM_PBL_MACROS_H
#define __ASM_PBL_MACROS_H

#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm-generic/memory_layout.h>
#include <asm/addrspace.h>
#include <asm/cacheops.h>

	.macro	pbl_reg_writel val addr
	.set push
	.set noreorder
	li	t9, \addr
	li	t8, \val
	sw	t8, 0(t9)
	.set	pop
	.endm

	.macro	pbl_reg_set val addr
	.set push
	.set noreorder
	li	t9, \addr
	li	t8, \val
	lw	t7, 0(t9)
	or	t7, t8
	sw	t7, 0(t9)
	.set	pop
	.endm

	.macro	pbl_reg_clr clr addr
	.set push
	.set noreorder
	li	t9, \addr
	li	t8, \clr
	lw	t7, 0(t9)
	not	t8, t8
	and	t7, t8
	sw	t7, 0(t9)
	.set	pop
	.endm

	.macro	pbl_blt addr label tmp
	.set	push
	.set	noreorder
	move	\tmp, ra			# preserve ra beforehand
	bal	253f
	 nop
253:
	bltu	ra, \addr, \label
	 move	ra, \tmp			# restore ra
	.set	pop
	.endm

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

#define WSIZE	4
copy_loop:
	/* copy from source address [a0] */
	lw	t4, WSIZE * 0(a0)
	lw	t5, WSIZE * 1(a0)
	lw	t6, WSIZE * 2(a0)
	lw	t7, WSIZE * 3(a0)
	/* copy to target address [a1] */
	sw	t4, WSIZE * 0(a1)
	sw	t5, WSIZE * 1(a1)
	sw	t6, WSIZE * 2(a1)
	sw	t7, WSIZE * 3(a1)
	addi	a0, WSIZE * 4
	subu	t3, a0, a2
	blez	t3, copy_loop
	 addi	a1, WSIZE * 4

copy_loop_exit:

	.set	pop
	.endm

	.macro	mips_disable_interrupts
	.set	push
	.set	noreorder
	mfc0	k0, CP0_STATUS
	li	k1, ~(ST0_ERL | ST0_IE)
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
	.ascii	"barebox"
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

#ifndef CONFIG_SYS_MIPS_CACHE_MODE
#define CONFIG_SYS_MIPS_CACHE_MODE CONF_CM_CACHABLE_NONCOHERENT
#endif

#define INDEX_BASE	CKSEG0

	.macro	f_fill64 dst, offset, val
	LONG_S	\val, (\offset +  0 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  1 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  2 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  3 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  4 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  5 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  6 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  7 * LONGSIZE)(\dst)
#if LONGSIZE == 4
	LONG_S	\val, (\offset +  8 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  9 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 10 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 11 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 12 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 13 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 14 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 15 * LONGSIZE)(\dst)
#endif
	.endm

	.macro cache_loop	curr, end, line_sz, op
10:	cache		\op, 0(\curr)
	PTR_ADDU	\curr, \curr, \line_sz
	bne		\curr, \end, 10b
	.endm

	.macro	l1_info		sz, line_sz, off
	.set	push
	.set	noat

	mfc0	$1, CP0_CONFIG, 1

	/* detect line size */
	srl	\line_sz, $1, \off + MIPS_CONF1_DL_SHF - MIPS_CONF1_DA_SHF
	andi	\line_sz, \line_sz, (MIPS_CONF1_DL >> MIPS_CONF1_DL_SHF)
	move	\sz, zero
	beqz	\line_sz, 10f
	li	\sz, 2
	sllv	\line_sz, \sz, \line_sz

	/* detect associativity */
	srl	\sz, $1, \off + MIPS_CONF1_DA_SHF - MIPS_CONF1_DA_SHF
	andi	\sz, \sz, (MIPS_CONF1_DA >> MIPS_CONF1_DA_SHF)
	addi	\sz, \sz, 1

	/* sz *= line_sz */
	mul	\sz, \sz, \line_sz

	/* detect log32(sets) */
	srl	$1, $1, \off + MIPS_CONF1_DS_SHF - MIPS_CONF1_DA_SHF
	andi	$1, $1, (MIPS_CONF1_DS >> MIPS_CONF1_DS_SHF)
	addiu	$1, $1, 1
	andi	$1, $1, 0x7

	/* sz <<= log32(sets) */
	sllv	\sz, \sz, $1

	/* sz *= 32 */
	li	$1, 32
	mul	\sz, \sz, $1
10:
	.set	pop
	.endm

/*
 * mips_cache_reset - low level initialisation of the primary caches
 *
 * This routine initialises the primary caches to ensure that they have good
 * parity.  It must be called by the ROM before any cached locations are used
 * to prevent the possibility of data with bad parity being written to memory.
 *
 * To initialise the instruction cache it is essential that a source of data
 * with good parity is available. This routine will initialise an area of
 * memory starting at location zero to be used as a source of parity.
 *
 */
	.macro	mips_cache_reset

	l1_info	t2, t8, MIPS_CONF1_IA_SHF
	l1_info	t3, t9, MIPS_CONF1_DA_SHF

	/*
	 * The TagLo registers used depend upon the CPU implementation, but the
	 * architecture requires that it is safe for software to write to both
	 * TagLo selects 0 & 2 covering supported cases.
	 */
	mtc0	zero, CP0_TAGLO
	mtc0	zero, CP0_TAGLO, 2

	/*
	 * The caches are probably in an indeterminate state, so we force good
	 * parity into them by doing an invalidate for each line.
	 */

	/*
	 * Initialize the I-cache first,
	 */
	blez		t2, 1f
	PTR_LI		t0, INDEX_BASE
	PTR_ADDU	t1, t0, t2
	/* clear tag to invalidate */
	cache_loop	t0, t1, t8, Index_Store_Tag_I

	/*
	 * then initialize D-cache.
	 */
1:	blez		t3, 3f
	PTR_LI		t0, INDEX_BASE
	PTR_ADDU	t1, t0, t3
	/* clear all tags */
	cache_loop	t0, t1, t9, Index_Store_Tag_D

3:	nop

	.endm

	.macro	dcache_enable
	mfc0	t0, CP0_CONFIG
	ori	t0, CONF_CM_CMASK
	xori	t0, CONF_CM_CMASK
	ori	t0, CONFIG_SYS_MIPS_CACHE_MODE
	mtc0	t0, CP0_CONFIG
	.endm

#endif /* __ASM_PBL_MACROS_H */
