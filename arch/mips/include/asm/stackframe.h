/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 99, 2001 Ralf Baechle
 * Copyright (C) 1994, 1995, 1996 Paul M. Antoine.
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2007  Maciej W. Rozycki
 */
#ifndef _ASM_STACKFRAME_H
#define _ASM_STACKFRAME_H

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/asm-offsets.h>

		.macro	SAVE_AT
		.set	push
		.set	noat
		LONG_S	$1, PT_R1(sp)
		.set	pop
		.endm

		.macro	SAVE_TEMP
		mfhi	v1
#ifdef CONFIG_32BIT
		LONG_S	$8, PT_R8(sp)
		LONG_S	$9, PT_R9(sp)
#endif
		LONG_S	$10, PT_R10(sp)
		LONG_S	$11, PT_R11(sp)
		LONG_S	$12, PT_R12(sp)

		LONG_S	v1, PT_HI(sp)
		mflo	v1

		LONG_S	$13, PT_R13(sp)
		LONG_S	$14, PT_R14(sp)
		LONG_S	$15, PT_R15(sp)
		LONG_S	$24, PT_R24(sp)

		LONG_S	v1, PT_LO(sp)

		.endm

		.macro	SAVE_STATIC
		LONG_S	$16, PT_R16(sp)
		LONG_S	$17, PT_R17(sp)
		LONG_S	$18, PT_R18(sp)
		LONG_S	$19, PT_R19(sp)
		LONG_S	$20, PT_R20(sp)
		LONG_S	$21, PT_R21(sp)
		LONG_S	$22, PT_R22(sp)
		LONG_S	$23, PT_R23(sp)
		LONG_S	$30, PT_R30(sp)
		.endm

		.macro	SAVE_SOME
		.set	push
		.set	noat
		.set	reorder
		.set	at=k0
		move	k1, sp
		PTR_SUBU k1, PT_SIZE
		.set	noat
		move	k0, sp
		move	sp, k1
		LONG_S	k0, PT_R29(sp)
		LONG_S	$3, PT_R3(sp)
		/*
		 * You might think that you don't need to save $0,
		 * but the FPU emulator and gdb remote debug stub
		 * need it to operate correctly
		 */
		LONG_S	$0, PT_R0(sp)
		mfc0	v1, CP0_STATUS
		LONG_S	$2, PT_R2(sp)
		LONG_S	v1, PT_STATUS(sp)
		LONG_S	$4, PT_R4(sp)
		mfc0	v1, CP0_CAUSE
		LONG_S	$5, PT_R5(sp)
		LONG_S	v1, PT_CAUSE(sp)
		LONG_S	$6, PT_R6(sp)
		MFC0	v1, CP0_EPC
		LONG_S	$7, PT_R7(sp)
#ifdef CONFIG_64BIT
		LONG_S	$8, PT_R8(sp)
		LONG_S	$9, PT_R9(sp)
#endif
		LONG_S	v1, PT_EPC(sp)
		LONG_S	$25, PT_R25(sp)
		LONG_S	$28, PT_R28(sp)
		LONG_S	$31, PT_R31(sp)
		.set	pop
		.endm

		.macro	SAVE_ALL
		SAVE_SOME
		SAVE_AT
		SAVE_TEMP
		SAVE_STATIC
		.endm

#endif /* _ASM_STACKFRAME_H */
