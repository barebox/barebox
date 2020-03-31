#include <config.h>
#include <linux/linkage.h>
#include <asm-generic/memory_layout.h>

/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */

@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC	0x13

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */

	.macro	bad_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	ldr	r2, =abort_stack
	ldmia	r2, {r2 - r3}			@ get pc, cpsr
	add	r0, sp, #S_FRAME_SIZE		@ restore sp_SVC

	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r3}			@ save sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp
	.endm

	.macro get_bad_stack
	ldr	r13, =abort_stack
	str	lr, [r13]			@ save caller lr / spsr
	mrs	lr, spsr
	str     lr, [r13, #4]

	mov	r13, #MODE_SVC			@ prepare SVC-Mode
	@ msr	spsr_c, r13
	msr	spsr, r13
	mov	lr, pc
	movs	pc, lr
	.endm

	.macro try_data_abort
	ldr	r13, =arm_ignore_data_abort	@ check try mode
	ldr	r13, [r13]
	cmp	r13, #0
	beq	do_abort_\@
	ldr	r13, =arm_data_abort_occurred
	str	r13, [r13]
	mrs	r13, spsr			@ read saved CPSR
	tst	r13, #1<<5			@ check Thumb mode
	subeq	lr, #4				@ next ARM instr
	subne	lr, #6				@ next Thumb instr
	movs	pc, lr
do_abort_\@:
	.endm

/*
 * exception handlers
 */
	.section ".text","ax"
	.arm

	.align  5
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl 	do_undefined_instruction

	.align	5
software_interrupt:
	get_bad_stack
	bad_save_user_regs
	bl 	do_software_interrupt

	.align	5
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl 	do_prefetch_abort

	.align	5
data_abort:
	try_data_abort
	get_bad_stack
	bad_save_user_regs
	bl 	do_data_abort

	.align	5
irq:
	get_bad_stack
	bad_save_user_regs
	bl 	do_irq

	.align	5
fiq:
	get_bad_stack
	bad_save_user_regs
	bl 	do_fiq

#if defined(CONFIG_RELOCATABLE) && defined(CONFIG_ARM_EXCEPTIONS)
/*
 * With relocatable binary support the runtime exception vectors do not match
 * the addresses in the binary. We have to fix them up during runtime
 */
ENTRY(arm_fixup_vectors)
	ldr	r0, =undefined_instruction
	ldr	r1, =_undefined_instruction
	str	r0, [r1]
	ldr	r0, =software_interrupt
	ldr	r1, =_software_interrupt
	str	r0, [r1]
	ldr	r0, =prefetch_abort
	ldr	r1, =_prefetch_abort
	str	r0, [r1]
	ldr	r0, =data_abort
	ldr	r1, =_data_abort
	str	r0, [r1]
	ldr	r0, =irq
	ldr	r1, =_irq
	str	r0, [r1]
	ldr	r0, =fiq
	ldr	r1, =_fiq
	str	r0, [r1]
	bx	lr
ENDPROC(arm_fixup_vectors)
#endif

.section .text_exceptions
.globl extable
extable:
1:	b 1b				/* barebox_arm_reset_vector */
#ifdef CONFIG_ARM_EXCEPTIONS
	ldr pc, _undefined_instruction	/* undefined instruction */
	ldr pc, _software_interrupt	/* software interrupt (SWI) */
	ldr pc, _prefetch_abort		/* prefetch abort */
	ldr pc, _data_abort		/* data abort */
1:	b 1b				/* (reserved) */
	ldr pc, _irq			/* irq (interrupt) */
	ldr pc, _fiq			/* fiq (fast interrupt) */
.globl _undefined_instruction
_undefined_instruction: .word undefined_instruction
.globl _software_interrupt
_software_interrupt: .word software_interrupt
.globl _prefetch_abort
_prefetch_abort: .word prefetch_abort
.globl _data_abort
_data_abort: .word data_abort
.globl _irq
_irq: .word irq
.globl _fiq
_fiq: .word fiq
#else
1:	b 1b				/* undefined instruction */
1:	b 1b				/* software interrupt (SWI) */
1:	b 1b				/* prefetch abort */
1:	b 1b				/* data abort */
1:	b 1b				/* (reserved) */
1:	b 1b				/* irq (interrupt) */
1:	b 1b				/* fiq (fast interrupt) */
#endif

.section .data
.align 4
.global arm_ignore_data_abort
arm_ignore_data_abort:
.word 0 /* When != 0 data aborts are ignored */
.global arm_data_abort_occurred
arm_data_abort_occurred:
.word 0 /* set != 0 by the data abort handler */
abort_stack:
.space 8
