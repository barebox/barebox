/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 1996-2000 Russell King */

/*
 *  arch/arm/include/asm/assembler.h
 *
 *  This file contains arm architecture specific defines
 *  for the different processors.
 *
 *  Do not include any C declarations in this file - it is included by
 *  assembler source.
 */
#ifndef __ASSEMBLY__
#error "Only include this from assembly code"
#endif

#include <asm/ptrace.h>

/*
 * Endian independent macros for shifting bytes within registers.
 */
#ifndef __ARMEB__
#define pull            lsr
#define push            lsl
#define get_byte_0      lsl #0
#define get_byte_1	lsr #8
#define get_byte_2	lsr #16
#define get_byte_3	lsr #24
#define put_byte_0      lsl #0
#define put_byte_1	lsl #8
#define put_byte_2	lsl #16
#define put_byte_3	lsl #24
#else
#define pull            lsl
#define push            lsr
#define get_byte_0	lsr #24
#define get_byte_1	lsr #16
#define get_byte_2	lsr #8
#define get_byte_3      lsl #0
#define put_byte_0	lsl #24
#define put_byte_1	lsl #16
#define put_byte_2	lsl #8
#define put_byte_3      lsl #0
#endif

/*
 * Data preload for architectures that support it
 */
#if __LINUX_ARM_ARCH__ >= 5
#define PLD(code...)	code
#else
#define PLD(code...)
#endif

/*
 * This can be used to enable code to cacheline align the destination
 * pointer when bulk writing to memory.  Experiments on StrongARM and
 * XScale didn't show this a worthwhile thing to do when the cache is not
 * set to write-allocate (this would need further testing on XScale when WA
 * is used).
 *
 * On Feroceon there is much to gain however, regardless of cache mode.
 */
#ifdef CONFIG_CPU_FEROCEON
#define CALGN(code...) code
#else
#define CALGN(code...)
#endif

/*
 * Enable and disable interrupts
 */
#if __LINUX_ARM_ARCH__ >= 6
	.macro	disable_irq
	cpsid	i
	.endm

	.macro	enable_irq
	cpsie	i
	.endm
#else
	.macro	disable_irq
	msr	cpsr_c, #PSR_I_BIT | SVC_MODE
	.endm

	.macro	enable_irq
	msr	cpsr_c, #SVC_MODE
	.endm
#endif

/*
 * Save the current IRQ state and disable IRQs.  Note that this macro
 * assumes FIQs are enabled, and that the processor is in SVC mode.
 */
	.macro	save_and_disable_irqs, oldcpsr
	mrs	\oldcpsr, cpsr
	disable_irq
	.endm

/*
 * Restore interrupt state previously stored in a register.  We don't
 * guarantee that this will preserve the flags.
 */
	.macro	restore_irqs, oldcpsr
	msr	cpsr_c, \oldcpsr
	.endm

#define USER(x...)				\
9999:	x;					\
	.section __ex_table,"a";		\
	.align	3;				\
	.long	9999b,9001f;			\
	.previous


/*
 * Select code when configured for BE.
 */
#ifdef CONFIG_CPU_BIG_ENDIAN
#define CPU_BE(code...) code
#else
#define CPU_BE(code...)
#endif

/*
 * Select code when configured for LE.
 */
#ifdef CONFIG_CPU_BIG_ENDIAN
#define CPU_LE(code...)
#else
#define CPU_LE(code...) code
#endif

#ifdef CONFIG_CPU_64
/*
 * Pseudo-ops for PC-relative adr/ldr/str <reg>, <symbol> where
 * <symbol> is within the range +/- 4 GB of the PC.
 */
	/*
	 * @dst: destination register (64 bit wide)
	 * @sym: name of the symbol
	 */
	.macro	adr_l, dst, sym
	adrp	\dst, \sym
	add	\dst, \dst, :lo12:\sym
	.endm

	/*
	 * @dst: destination register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: optional 64-bit scratch register to be used if <dst> is a
	 *       32-bit wide register, in which case it cannot be used to hold
	 *       the address
	 */
	.macro	ldr_l, dst, sym, tmp=
	.ifb	\tmp
	adrp	\dst, \sym
	ldr	\dst, [\dst, :lo12:\sym]
	.else
	adrp	\tmp, \sym
	ldr	\dst, [\tmp, :lo12:\sym]
	.endif
	.endm

	/*
	 * @src: source register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: mandatory 64-bit scratch register to calculate the address
	 *       while <src> needs to be preserved.
	 */
	.macro	str_l, src, sym, tmp
	adrp	\tmp, \sym
	str	\src, [\tmp, :lo12:\sym]
	.endm

#else

	.macro		__adldst_l, op, reg, sym, tmp, c
	.if		__LINUX_ARM_ARCH__ < 7
	ldr\c		\tmp, .La\@
	.subsection	1
	.align		2
.La\@:	.long		\sym - .Lpc\@
	.previous
	.else
	.ifnb		\c
 THUMB(	ittt		\c			)
	.endif
	movw\c		\tmp, #:lower16:\sym - .Lpc\@
	movt\c		\tmp, #:upper16:\sym - .Lpc\@
	.endif

#ifndef CONFIG_THUMB2_BAREBOX
	.set		.Lpc\@, . + 8			// PC bias
	.ifc		\op, add
	add\c		\reg, \tmp, pc
	.else
	\op\c		\reg, [pc, \tmp]
	.endif
#else
.Lb\@:	add\c		\tmp, \tmp, pc
	/*
	 * In Thumb-2 builds, the PC bias depends on whether we are currently
	 * emitting into a .arm or a .thumb section. The size of the add opcode
	 * above will be 2 bytes when emitting in Thumb mode and 4 bytes when
	 * emitting in ARM mode, so let's use this to account for the bias.
	 */
	.set		.Lpc\@, . + (. - .Lb\@)

	.ifnc		\op, add
	\op\c		\reg, [\tmp]
	.endif
#endif
	.endm

	/*
	 * mov_l - move a constant value or [relocated] address into a register
	 */
	.macro		mov_l, dst:req, imm:req, cond
	.if		__LINUX_ARM_ARCH__ < 7
	ldr\cond	\dst, =\imm
	.else
	movw\cond	\dst, #:lower16:\imm
	movt\cond	\dst, #:upper16:\imm
	.endif
	.endm

	/*
	 * adr_l - adr pseudo-op with unlimited range
	 *
	 * @dst: destination register
	 * @sym: name of the symbol
	 * @cond: conditional opcode suffix
	 */
	.macro		adr_l, dst:req, sym:req, cond
	__adldst_l	add, \dst, \sym, \dst, \cond
	.endm

	/*
	 * ldr_l - ldr <literal> pseudo-op with unlimited range
	 *
	 * @dst: destination register
	 * @sym: name of the symbol
	 * @cond: conditional opcode suffix
	 */
	.macro		ldr_l, dst:req, sym:req, cond
	__adldst_l	ldr, \dst, \sym, \dst, \cond
	.endm

	/*
	 * str_l - str <literal> pseudo-op with unlimited range
	 *
	 * @src: source register
	 * @sym: name of the symbol
	 * @tmp: mandatory scratch register
	 * @cond: conditional opcode suffix
	 */
	.macro		str_l, src:req, sym:req, tmp:req, cond
	__adldst_l	str, \src, \sym, \tmp, \cond
	.endm

	.macro		__ldst_va, op, reg, tmp, sym, cond, offset
#if __LINUX_ARM_ARCH__ >= 7 || \
    (defined(MODULE) && defined(CONFIG_ARM_MODULE_PLTS))
	mov_l		\tmp, \sym, \cond
#else
	/*
	 * Avoid a literal load, by emitting a sequence of ADD/LDR instructions
	 * with the appropriate relocations. The combined sequence has a range
	 * of -/+ 256 MiB, which should be sufficient for the core kernel and
	 * for modules loaded into the module region.
	 */
	.globl		\sym
	.reloc		.L0_\@, R_ARM_ALU_PC_G0_NC, \sym
	.reloc		.L1_\@, R_ARM_ALU_PC_G1_NC, \sym
	.reloc		.L2_\@, R_ARM_LDR_PC_G2, \sym
.L0_\@: sub\cond	\tmp, pc, #8 - \offset
.L1_\@: sub\cond	\tmp, \tmp, #4 - \offset
.L2_\@:
#endif
	\op\cond	\reg, [\tmp, #\offset]
	.endm

	/*
	 * ldr_va - load a 32-bit word from the virtual address of \sym
	 */
	.macro		ldr_va, rd:req, sym:req, cond, tmp, offset=0
	.ifnb		\tmp
	__ldst_va	ldr, \rd, \tmp, \sym, \cond, \offset
	.else
	__ldst_va	ldr, \rd, \rd, \sym, \cond, \offset
	.endif
	.endm

	/*
	 * str_va - store a 32-bit word to the virtual address of \sym
	 */
	.macro		str_va, rn:req, sym:req, tmp:req, cond
	__ldst_va	str, \rn, \tmp, \sym, \cond, 0
	.endm

	/*
	 * ldr_this_cpu - Load a 32-bit word from the per-CPU variable 'sym'
	 *		  into register 'rd', which may be the stack pointer,
	 *		  using 't1' and 't2' as general temp registers. These
	 *		  are permitted to overlap with 'rd' if != sp
	 */
	.macro		ldr_this_cpu, rd:req, sym:req, t1:req, t2:req
	ldr_va		\rd, \sym, tmp=\t1
	.endm

	/*
	 * rev_l - byte-swap a 32-bit value
	 *
	 * @val: source/destination register
	 * @tmp: scratch register
	 */
	.macro		rev_l, val:req, tmp:req
	.if		__LINUX_ARM_ARCH__ < 6
	eor		\tmp, \val, \val, ror #16
	bic		\tmp, \tmp, #0x00ff0000
	mov		\val, \val, ror #8
	eor		\val, \val, \tmp, lsr #8
	.else
	rev		\val, \val
	.endif
	.endm

	/*
	 * bl_r - branch and link to register
	 *
	 * @dst: target to branch to
	 * @c: conditional opcode suffix
	 */
	.macro		bl_r, dst:req, c
	.if		__LINUX_ARM_ARCH__ < 6
	mov\c		lr, pc
	mov\c		pc, \dst
	.else
	blx\c		\dst
	.endif
	.endm
#endif
