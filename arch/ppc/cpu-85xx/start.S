/*
 * Copyright 2004, 2007-2012 Freescale Semiconductor, Inc.
 * Copyright (C) 2003  Motorola,Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * U-Boot Startup Code for Motorola 85xx PowerPC based Embedded Boards
 *
 * The processor starts at 0xfffffffc and the code is first executed in the
 * last 4K page(0xfffff000-0xffffffff) in flash/rom.
 */

#include <config.h>
#include <asm/config.h>

#include <asm/processor.h>
#include <asm/ppc_asm.tmpl>
#include <asm/ppc_defs.h>
#include <asm/cache.h>

#include <mach/mpc85xx.h>
#include <mach/mmu.h>

#undef	MSR_KERNEL
#define MSR_KERNEL ( MSR_ME )	/* Machine Check */

/*
 * Set up GOT: Global Offset Table
 *
 * Use r14 to access the GOT
 */
	START_GOT
	GOT_ENTRY(_GOT2_TABLE_)
	GOT_ENTRY(_FIXUP_TABLE_)

	GOT_ENTRY(_start)
	GOT_ENTRY(_start_of_vectors)
	GOT_ENTRY(_end_of_vectors)
	GOT_ENTRY(transfer_to_handler)
	GOT_ENTRY(__init_end)
	GOT_ENTRY(__bss_start)
	GOT_ENTRY(__bss_stop)
	END_GOT

/*
 * e500 Startup -- after reset only the last 4KB of the effective
 * address space is mapped in the MMU L2 TLB1 Entry0. The .bootpg
 * section is located at THIS LAST page and basically does three
 * things: clear some registers, set up exception tables and
 * add more TLB entries for 'larger spaces'(e.g. the boot rom) to
 * continue the boot procedure.
 * Once the boot rom is mapped by TLB entries we can proceed
 * with normal startup.
 */

	.section .bootpg,"ax"
	.globl _start_e500

_start_e500:
/* Enable debug exception */
	li	r1,MSR_DE
	mtmsr 	r1

#ifdef FSL_ERRATUM_A005125
	msync
	isync
	mfspr	r3, SPRN_HDBCR0
	oris	r3, r3, 0x0080
	mtspr	SPRN_HDBCR0, r3
#endif

/* clear registers/arrays not reset by hardware */

	/* L1 */
	li	r0,2
	mtspr	L1CSR0,r0	/* invalidate d-cache */
	mtspr	L1CSR1,r0	/* invalidate i-cache */

	mfspr	r1,DBSR
	mtspr	DBSR,r1		/* Clear all valid bits */


	.macro	create_tlb1_entry esel ts tsize epn wimg rpn \
                              perm phy_high scratch
	lis	\scratch, FSL_BOOKE_MAS0(1, \esel, 0)@h
	ori	\scratch, \scratch, FSL_BOOKE_MAS0(1, \esel, 0)@l
	mtspr	MAS0, \scratch
	lis	\scratch, FSL_BOOKE_MAS1(1, 1, 0, \ts, \tsize)@h
	ori	\scratch, \scratch, FSL_BOOKE_MAS1(1, 1, 0, \ts, \tsize)@l
	mtspr	MAS1, \scratch
	lis	\scratch, FSL_BOOKE_MAS2(\epn, \wimg)@h
	ori	\scratch, \scratch, FSL_BOOKE_MAS2(\epn, \wimg)@l
	mtspr	MAS2, \scratch
	lis	\scratch, FSL_BOOKE_MAS3(\rpn, 0, \perm)@h
	ori	\scratch, \scratch, FSL_BOOKE_MAS3(\rpn, 0, \perm)@l
	mtspr	MAS3, \scratch
	lis	\scratch, \phy_high@h
	ori	\scratch, \scratch, \phy_high@l
	mtspr	MAS7, \scratch
	isync
	msync
	tlbwe
	isync
	.endm

	/* Setup interrupt vectors */
	lis	r1,TEXT_BASE@h
	mtspr	IVPR,r1

	lis	r3,(TEXT_BASE & 0xffff)@h
	ori	r3,r3,(TEXT_BASE & 0xffff)@l

	addi	r4,r3,CriticalInput - _start + _START_OFFSET
	mtspr	IVOR0,r4	/* 0: Critical input */
	addi	r4,r3,MachineCheck - _start + _START_OFFSET
	mtspr	IVOR1,r4	/* 1: Machine check */
	addi	r4,r3,DataStorage - _start + _START_OFFSET
	mtspr	IVOR2,r4	/* 2: Data storage */
	addi	r4,r3,InstStorage - _start + _START_OFFSET
	mtspr	IVOR3,r4	/* 3: Instruction storage */
	addi	r4,r3,ExtInterrupt - _start + _START_OFFSET
	mtspr	IVOR4,r4	/* 4: External interrupt */
	addi	r4,r3,Alignment - _start + _START_OFFSET
	mtspr	IVOR5,r4	/* 5: Alignment */
	addi	r4,r3,ProgramCheck - _start + _START_OFFSET
	mtspr	IVOR6,r4	/* 6: Program check */
	addi	r4,r3,FPUnavailable - _start + _START_OFFSET
	mtspr	IVOR7,r4	/* 7: floating point unavailable */
	addi	r4,r3,SystemCall - _start + _START_OFFSET
	mtspr	IVOR8,r4	/* 8: System call */
	/* 9: Auxiliary processor unavailable(unsupported) */
	addi	r4,r3,Decrementer - _start + _START_OFFSET
	mtspr	IVOR10,r4	/* 10: Decrementer */
	addi	r4,r3,IntervalTimer - _start + _START_OFFSET
	mtspr	IVOR11,r4	/* 11: Interval timer */
	addi	r4,r3,WatchdogTimer - _start + _START_OFFSET
	mtspr	IVOR12,r4	/* 12: Watchdog timer */
	addi	r4,r3,DataTLBError - _start + _START_OFFSET
	mtspr	IVOR13,r4	/* 13: Data TLB error */
	addi	r4,r3,InstructionTLBError - _start + _START_OFFSET
	mtspr	IVOR14,r4	/* 14: Instruction TLB error */
	addi	r4,r3,DebugBreakpoint - _start + _START_OFFSET
	mtspr	IVOR15,r4	/* 15: Debug */

	/* Clear and set up some registers. */
	li      r0,0x0000
	lis	r1,0xffff
	mtspr	DEC,r0			/* prevent dec exceptions */
	mttbl	r0			/* prevent fit & wdt exceptions */
	mttbu	r0
	mtspr	TSR,r1			/* clear all timer exception status */
	mtspr	TCR,r0			/* disable all */
	mtspr	ESR,r0			/* clear exception syndrome register */
	mtspr	MCSR,r0			/* machine check syndrome register */
	mtxer	r0			/* clear integer exception register */

	/* Enable Time Base and Select Time Base Clock */
	lis	r0,HID0_EMCP@h		/* Enable machine check */
	ori	r0,r0,HID0_TBEN@l	/* Enable Timebase */
	mtspr	HID0,r0

	li	r0,(HID1_ASTME|HID1_ABE)@l	/* Addr streaming & broadcast */
	mfspr	r3,PVR
	andi.	r3,r3, 0xff
	cmpwi	r3,0x50@l	/* if we are rev 5.0 or greater set MBDD */
	blt 1f
	/* Set MBDD bit also */
	ori r0, r0, HID1_MBDD@l
1:
	mtspr	HID1,r0

	/* Enable Branch Prediction */
#if defined(CFG_BTB)
	lis	r0,BUCSR_ENABLE@h
	ori	r0,r0,BUCSR_ENABLE@l
	mtspr	SPRN_BUCSR,r0
#endif

/*
 * Search for the TLB that covers the code we're executing, and shrink it
 * so that it covers only this 4K page. That will ensure that any other
 * TLB we create won't interfere with it. We assume that the TLB exists,
 * which is why we don't check the Valid bit of MAS1. We also assume
 * it is in TLB1.
 *
 * This is necessary, for example, when booting from the on-chip ROM,
 * which (oddly) creates a single 4GB TLB that covers CCSR and DDR.
 */
	bl	nexti		/* Find our address */
nexti:	mflr	r1		/* R1 = our PC */
	li	r2, 0
	mtspr	MAS6, r2	/* Assume the current PID and AS are 0 */
	isync
	msync
	tlbsx	0, r1		/* This must succeed */

	mfspr	r14, MAS0	/* Save ESEL for later */
	rlwinm	r14, r14, 16, 0xfff

	/* Set the size of the TLB to 4KB */
	mfspr	r3, MAS1
	li	r2, 0xF80
	andc	r3, r3, r2	/* Clear the TSIZE bits */
	ori	r3, r3, MAS1_TSIZE(BOOKE_PAGESZ_4K)@l
	oris	r3, r3, MAS1_IPROT@h
	mtspr	MAS1, r3

	/*
	 * Set the base address of the TLB to our PC.  We assume that
	 * virtual == physical. We also assume that MAS2_EPN == MAS3_RPN.
	 */
	lis	r3, MAS2_EPN@h
	ori	r3, r3, MAS2_EPN@l	/* R3 = MAS2_EPN */

	and	r1, r1, r3	/* Our PC, rounded down to the nearest page */

	mfspr	r2, MAS2
	andc	r2, r2, r3
	or	r2, r2, r1
	mtspr	MAS2, r2	/* Set the EPN to our PC base address */

	mfspr	r2, MAS3
	andc	r2, r2, r3
	or	r2, r2, r1
	mtspr	MAS3, r2	/* Set the RPN to our PC base address */

	isync
	msync
	tlbwe

	/*
	 * Clear out any other TLB entries that may exist, to avoid conflicts.
	 * Our TLB entry is in r14.
	 */
	li	r0, TLBIVAX_ALL | TLBIVAX_TLB0
	tlbivax 0, r0
	tlbsync

	mfspr	r4, SPRN_TLB1CFG
	rlwinm	r4, r4, 0, TLBnCFG_NENTRY_MASK

	li	r3, 0
	mtspr	MAS1, r3
1:	cmpw	r3, r14
	rlwinm	r5, r3, 16, MAS0_ESEL_MSK
	addi	r3, r3, 1
	beq	2f		/* skip the entry we're executing from */

	oris	r5, r5, MAS0_TLBSEL(1)@h
	mtspr	MAS0, r5

	isync
	tlbwe
	isync
	msync

2:	cmpw	r3, r4
	blt	1b

#if defined(PPC_E500_DEBUG_TLB)
/*
 * TLB entry for debuggging in AS1
 * Create temporary TLB entry in AS0 to handle debug exception
 * As on debug exception MSR is cleared i.e. Address space is changed
 * to 0. A TLB entry (in AS0) is required to handle debug exception generated
 * in AS1.
 *
 * TLB entry is created for IVPR + IVOR15 to map on valid OP code address
 * because flash's virtual address maps to 0xff800000 - 0xffffffff.
 * and this window is outside of 4K boot window.
 */
	create_tlb1_entry PPC_E500_DEBUG_TLB, \
		0, BOOKE_PAGESZ_4M, \
		TEXT_BASE & 0xffc00000,  MAS2_I|MAS2_G, \
		0xffc00000, MAS3_SX|MAS3_SW|MAS3_SR, \
		0, r6

#endif

	/* Enable/invalidate the I-Cache */
	lis	r2,(L1CSR1_ICFI|L1CSR1_ICLFR)@h
	ori	r2,r2,(L1CSR1_ICFI|L1CSR1_ICLFR)@l
	mtspr	SPRN_L1CSR1,r2
1:
	mfspr	r3,SPRN_L1CSR1
	and.	r1,r3,r2
	bne	1b

	lis	r3,(L1CSR1_CPE|L1CSR1_ICE)@h
	ori	r3,r3,(L1CSR1_CPE|L1CSR1_ICE)@l
	mtspr	SPRN_L1CSR1,r3
	isync
2:
	mfspr	r3,SPRN_L1CSR1
	andi.	r1,r3,L1CSR1_ICE@l
	beq	2b

	/* Enable/invalidate the D-Cache */
	lis	r2,(L1CSR0_DCFI|L1CSR0_DCLFR)@h
	ori	r2,r2,(L1CSR0_DCFI|L1CSR0_DCLFR)@l
	mtspr	SPRN_L1CSR0,r2
1:
	mfspr	r3,SPRN_L1CSR0
	and.	r1,r3,r2
	bne	1b

	lis	r3,(L1CSR0_CPE|L1CSR0_DCE)@h
	ori	r3,r3,(L1CSR0_CPE|L1CSR0_DCE)@l
	mtspr	SPRN_L1CSR0,r3
	isync
2:
	mfspr	r3,SPRN_L1CSR0
	andi.	r1,r3,L1CSR0_DCE@l
	beq	2b

create_init_ram_area:
	lis	r6,FSL_BOOKE_MAS0(1, 15, 0)@h
	ori	r6,r6,FSL_BOOKE_MAS0(1, 15, 0)@l

	/* create a temp mapping in AS=1 to the 4M boot window */
	create_tlb1_entry 15, \
		1, BOOKE_PAGESZ_4M, \
		TEXT_BASE & 0xffc00000, MAS2_I|MAS2_G, \
		0xffc00000, MAS3_SX|MAS3_SW|MAS3_SR, \
		0, r6

	/* create a temp mapping in AS=1 to the stack */
	create_tlb1_entry 14, \
		1, BOOKE_PAGESZ_16K, \
		CFG_INIT_RAM_ADDR, 0, \
		CFG_INIT_RAM_ADDR, MAS3_SX|MAS3_SW|MAS3_SR, \
		0, r6

	lis	r6,MSR_IS|MSR_DS|MSR_DE@h
	ori	r6,r6,MSR_IS|MSR_DS|MSR_DE@l
	lis	r7,switch_as@h
	ori	r7,r7,switch_as@l

	mtspr	SPRN_SRR0,r7
	mtspr	SPRN_SRR1,r6
	rfi

switch_as:
/* L1 DCache is used for initial RAM */

	/* Allocate initial RAM in data cache.  */
	lis	r3,CFG_INIT_RAM_ADDR@h
	ori	r3,r3,CFG_INIT_RAM_ADDR@l
	mfspr	r2, L1CFG0
	andi.	r2, r2, 0x1ff
	/* cache size * 1024 / (2 * L1 line size) */
	slwi	r2, r2, (10 - 1 - L1_CACHE_SHIFT)
	mtctr	r2
	li	r0,0
1:
	dcbz	r0,r3
	dcbtls	0,r0,r3
	addi	r3,r3,CACHELINE_SIZE
	bdnz	1b

	/*
	 * Jump out the last 4K page and continue to 'normal' start.
	 * Calculate absolute address in FLASH and jump there.
	 */
	lis	r3,TEXT_BASE@h
	ori	r3,r3,TEXT_BASE@l
	addi	r3,r3,_start_cont - _start + _START_OFFSET
	mtlr	r3
	blr

	.text
	.globl	_start
_start:
	.long	0x62626F78		/* Magic Number */

	.align	4
	.globl	_start_cont
_start_cont:
	/* Setup the stack in initial RAM,could be L2-as-SRAM or L1 dcache */
	lis	r3,(CFG_INIT_RAM_ADDR)@h
	ori	r3,r3,((CFG_INIT_SP_OFFSET-16)&~0xf)@l
	li	r0,0
	stw	r0,0(r3)	/* Terminate Back Chain */
	stw	r0,+4(r3)	/* NULL return address. */
	mr	r1,r3		/* Transfer to SP(r1) */

	GET_GOT
	bl	cpu_init_early_f

	/* switch back to AS = 0 */
	lis	r3,(MSR_CE|MSR_ME|MSR_DE)@h
	ori	r3,r3,(MSR_CE|MSR_ME|MSR_DE)@l
	mtmsr	r3
	isync

	bl      initdram
	b       relocate_code
	isync

	. = EXC_OFF_SYS_RESET
	.globl	_start_of_vectors
_start_of_vectors:

/* Critical input. */
	CRIT_EXCEPTION(0x0100, CriticalInput, CritcalInputException)

/* Machine check */
	MCK_EXCEPTION(0x200, MachineCheck, MachineCheckException)

/* Data Storage exception. */
	STD_EXCEPTION(0x0300, DataStorage, UnknownException)

/* Instruction Storage exception. */
	STD_EXCEPTION(0x0400, InstStorage, UnknownException)

/* External Interrupt exception. */
	STD_EXCEPTION(0x0500, ExtInterrupt, UnknownException)

/* Alignment exception. */
	. = 0x0600
Alignment:
	EXCEPTION_PROLOG(SRR0, SRR1)
	mfspr	r4,DAR
	stw	r4,_DAR(r21)
	mfspr	r5,DSISR
	stw	r5,_DSISR(r21)
	addi	r3,r1,STACK_FRAME_OVERHEAD
	EXC_XFER_TEMPLATE(Alignment, AlignmentException, MSR_KERNEL, COPY_EE)

/* Program check exception */
	. = 0x0700
ProgramCheck:
	EXCEPTION_PROLOG(SRR0, SRR1)
	addi	r3,r1,STACK_FRAME_OVERHEAD
	EXC_XFER_TEMPLATE(ProgramCheck, ProgramCheckException,
		MSR_KERNEL, COPY_EE)

	/* No FPU on MPC85xx.  This exception is not supposed to happen.  */
	STD_EXCEPTION(0x0800, FPUnavailable, UnknownException)

	. = 0x0900
/*
 * r0 - SYSCALL number
 * r3-... arguments
 */
SystemCall:
	addis	r11,r0,0	/* get functions table addr */
	ori	r11,r11,0	/* Note: this code is patched in trap_init */
	addis	r12,r0,0	/* get number of functions */
	ori	r12,r12,0

	cmplw	0,r0,r12
	bge	1f

	rlwinm	r0,r0,2,0,31	/* fn_addr = fn_tbl[r0] */
	add	r11,r11,r0
	lwz	r11,0(r11)

	li	r20,0xd00-4	/* Get stack pointer */
	lwz	r12,0(r20)
	subi	r12,r12,12	/* Adjust stack pointer */
	li	r0,0xc00+_end_back-SystemCall
	cmplw	0,r0,r12	/* Check stack overflow */
	bgt	1f
	stw	r12,0(r20)

	mflr	r0
	stw	r0,0(r12)
	mfspr	r0,SRR0
	stw	r0,4(r12)
	mfspr	r0,SRR1
	stw	r0,8(r12)

	li	r12,0xc00+_back-SystemCall
	mtlr	r12
	mtspr	SRR0,r11

1:	SYNC
	rfi
_back:

	mfmsr	r11		/* Disable interrupts */
	li	r12,0
	ori	r12,r12,MSR_EE
	andc	r11,r11,r12
	SYNC			/* Some chip revs need this... */
	mtmsr	r11
	SYNC

	li	r12,0xd00-4	/* restore regs */
	lwz	r12,0(r12)

	lwz	r11,0(r12)
	mtlr	r11
	lwz	r11,4(r12)
	mtspr	SRR0,r11
	lwz	r11,8(r12)
	mtspr	SRR1,r11

	addi	r12,r12,12	/* Adjust stack pointer */
	li	r20,0xd00-4
	stw	r12,0(r20)

	SYNC
	rfi
_end_back:

	STD_EXCEPTION(0x0a00, Decrementer, UnknownException)
	STD_EXCEPTION(0x0b00, IntervalTimer, UnknownException)
	STD_EXCEPTION(0x0c00, WatchdogTimer, UnknownException)

	STD_EXCEPTION(0x0d00, DataTLBError, UnknownException)
	STD_EXCEPTION(0x0e00, InstructionTLBError, UnknownException)

	CRIT_EXCEPTION(0x0f00, DebugBreakpoint, DebugException )

	.globl	_end_of_vectors
_end_of_vectors:

	. = . + (0x100 - ( . & 0xff ))	/* align for debug */

/*
 * This code finishes saving the registers to the exception frame
 * and jumps to the appropriate handler for the exception.
 * Register r21 is pointer into trap frame, r1 has new stack pointer.
 */
	.globl	transfer_to_handler
transfer_to_handler:
	stw	r22,_NIP(r21)
	lis	r22,MSR_POW@h
	andc	r23,r23,r22
	stw	r23,_MSR(r21)
	SAVE_GPR(7, r21)
	SAVE_4GPRS(8, r21)
	SAVE_8GPRS(12, r21)
	SAVE_8GPRS(24, r21)

	mflr	r23
	andi.	r24,r23,0x3f00	/* get vector offset  */
	stw	r24,TRAP(r21)
	li	r22,0
	stw	r22,RESULT(r21)
	mtspr	SPRG2,r22	/* r1 is now kernel sp */

	lwz	r24,0(r23)	/* virtual address of handler 	*/
	lwz	r23,4(r23)	/* where to go when done 	*/
	mtspr	SRR0,r24
	mtspr	SRR1,r20
	mtlr	r23
	SYNC
	rfi			/* jump to handler, enable MMU  */

int_return:
	mfmsr	r28		/* Disable interrupts */
	li	r4,0
	ori	r4,r4,MSR_EE
	andc	r28,r28,r4
	SYNC			/* Some chip revs need this... */
	mtmsr	r28
	SYNC
	lwz	r2,_CTR(r1)
	lwz	r0,_LINK(r1)
	mtctr	r2
	mtlr	r0
	lwz	r2,_XER(r1)
	lwz	r0,_CCR(r1)
	mtspr	XER,r2
	mtcrf	0xFF,r0
	REST_10GPRS(3, r1)
	REST_10GPRS(13, r1)
	REST_8GPRS(23, r1)
	REST_GPR(31, r1)
	lwz	r2,_NIP(r1)	/* Restore environment */
	lwz	r0,_MSR(r1)
	mtspr	SRR0,r2
	mtspr	SRR1,r0
	lwz	r0,GPR0(r1)
	lwz	r2,GPR2(r1)
	lwz	r1,GPR1(r1)
	SYNC
	rfi

crit_return:
	mfmsr	r28		/* Disable interrupts */
	li	r4,0
	ori	r4,r4,MSR_EE
	andc	r28,r28,r4
	SYNC			/* Some chip revs need this... */
	mtmsr	r28
	SYNC
	lwz	r2,_CTR(r1)
	lwz	r0,_LINK(r1)
	mtctr	r2
	mtlr	r0
	lwz	r2,_XER(r1)
	lwz	r0,_CCR(r1)
	mtspr	XER,r2
	mtcrf	0xFF,r0
	REST_10GPRS(3, r1)
	REST_10GPRS(13, r1)
	REST_8GPRS(23, r1)
	REST_GPR(31, r1)
	lwz	r2,_NIP(r1)	/* Restore environment */
	lwz	r0,_MSR(r1)
	mtspr	SPRN_CSRR0,r2
	mtspr	SPRN_CSRR1,r0
	lwz	r0,GPR0(r1)
	lwz	r2,GPR2(r1)
	lwz	r1,GPR1(r1)
	SYNC
	rfci

mck_return:
	mfmsr	r28		/* Disable interrupts */
	li	r4,0
	ori	r4,r4,MSR_EE
	andc	r28,r28,r4
	SYNC			/* Some chip revs need this... */
	mtmsr	r28
	SYNC
	lwz	r2,_CTR(r1)
	lwz	r0,_LINK(r1)
	mtctr	r2
	mtlr	r0
	lwz	r2,_XER(r1)
	lwz	r0,_CCR(r1)
	mtspr	XER,r2
	mtcrf	0xFF,r0
	REST_10GPRS(3, r1)
	REST_10GPRS(13, r1)
	REST_8GPRS(23, r1)
	REST_GPR(31, r1)
	lwz	r2,_NIP(r1)	/* Restore environment */
	lwz	r0,_MSR(r1)
	mtspr	SPRN_MCSRR0,r2
	mtspr	SPRN_MCSRR1,r0
	lwz	r0,GPR0(r1)
	lwz	r2,GPR2(r1)
	lwz	r1,GPR1(r1)
	SYNC
	rfmci

/*
 * Cache functions.
 */
.globl invalidate_icache
invalidate_icache:
	mfspr	r0,L1CSR1
	ori	r0,r0,L1CSR1_ICFI
	msync
	isync
	mtspr	L1CSR1,r0
	isync
	blr

.globl invalidate_dcache
invalidate_dcache:
	mfspr	r0,L1CSR0
	ori	r0,r0,L1CSR0_DCFI
	msync
	isync
	mtspr	L1CSR0,r0
	isync
	blr

	.globl	icache_enable
icache_enable:
	mflr	r8
	bl	invalidate_icache
	mtlr	r8
	isync
	mfspr	r4,L1CSR1
	ori	r4,r4,0x0001
	oris	r4,r4,0x0001
	mtspr	L1CSR1,r4
	isync
	blr

	.globl	icache_disable
icache_disable:
	mfspr	r0,L1CSR1
	lis	r3,0
	ori	r3,r3,L1CSR1_ICE
	andc	r0,r0,r3
	mtspr	L1CSR1,r0
	isync
	blr

	.globl	icache_status
icache_status:
	mfspr	r3,L1CSR1
	andi.	r3,r3,L1CSR1_ICE
	blr

	.globl	dcache_enable
dcache_enable:
	mflr	r8
	bl	invalidate_dcache
	mtlr	r8
	isync
	mfspr	r0,L1CSR0
	ori	r0,r0,0x0001
	oris	r0,r0,0x0001
	msync
	isync
	mtspr	L1CSR0,r0
	isync
	blr

	.globl	dcache_disable
dcache_disable:
	mfspr	r3,L1CSR0
	lis	r4,0
	ori	r4,r4,L1CSR0_DCE
	andc	r3,r3,r4
	mtspr	L1CSR0,r3
	isync
	blr

	.globl	dcache_status
dcache_status:
	mfspr	r3,L1CSR0
	andi.	r3,r3,L1CSR0_DCE
	blr

	.globl get_pir
get_pir:
	mfspr	r3,PIR
	blr

	.globl get_pvr
get_pvr:
	mfspr	r3,PVR
	blr

	.globl get_svr
get_svr:
	mfspr	r3,SVR
	blr

	.globl wr_tcr
wr_tcr:
	mtspr	TCR,r3
	blr

/*
 * Function:	 in8
 * Description:	 Input 8 bits
 */
	.globl	in8
in8:
	lbz	r3,0x0000(r3)
	blr

/*
 * Function:	 out8
 * Description:	 Output 8 bits
 */
	.globl	out8
out8:
	stb	r4,0x0000(r3)
	sync
	blr

/*
 * Function:	 out16
 * Description:	 Output 16 bits
 */
	.globl	out16
out16:
	sth	r4,0x0000(r3)
	sync
	blr

/*
 * Function:	 out16r
 * Description:	 Byte reverse and output 16 bits
 */
	.globl	out16r
out16r:
	sthbrx	r4,r0,r3
	sync
	blr

/*
 * Function:	 out32
 * Description:	 Output 32 bits
 */
	.globl	out32
out32:
	stw	r4,0x0000(r3)
	sync
	blr

/*
 * Function:	 out32r
 * Description:	 Byte reverse and output 32 bits
 */
	.globl	out32r
out32r:
	stwbrx	r4,r0,r3
	sync
	blr

/*
 * Function:	 in16
 * Description:	 Input 16 bits
 */
	.globl	in16
in16:
	lhz	r3,0x0000(r3)
	blr

/*
 * Function:	 in16r
 * Description:	 Input 16 bits and byte reverse
 */
	.globl	in16r
in16r:
	lhbrx	r3,r0,r3
	blr

/*
 * Function:	 in32
 * Description:	 Input 32 bits
 */
	.globl	in32
in32:
	lwz	3,0x0000(3)
	blr

/*
 * Function:	 in32r
 * Description:	 Input 32 bits and byte reverse
 */
	.globl	in32r
in32r:
	lwbrx	r3,r0,r3
	blr

/*
 * void e500_write_tlb(mas0, mas1, mas2, mas3, mas7)
 */
	.globl	e500_write_tlb
e500_write_tlb:
	mtspr	MAS0,r3
	mtspr	MAS1,r4
	mtspr	MAS2,r5
	mtspr	MAS3,r6
	li	r3,0
	isync
	tlbwe
	msync
	isync
	blr

/*
 * void relocate_code (end of ram)
 *
 * This "function" does not return, instead it continues in RAM
 * after relocating the monitor code.
 *
 * r3 = end_of_ram
 * r4 = src
 * r5 = length in bytes
 * r6 = cachelinesize
 */
	.globl	relocate_code
relocate_code:
	mr	r9, r3			/* Save end of RAM 	*/

	GET_GOT
	lis	r4,TEXT_BASE@h
	ori	r4,r4,TEXT_BASE@l
	lwz	r5,GOT(__bss_stop)	/* size */
	sub	r5,r5,r4
	sub	r3, r3, r5
	lwz	r5,GOT(__init_end)	/* Copy to init_end only */
	sub	r5,r5,r4
	lis	r10, 0xffff0000@h	/* Round down to 64KB */
	ori	r10, r10, 0xffff0000@l
	and	r3, r3, r10		/* Destination */
	mr	r1, r3
	mr	r10, r3
	li	r6,CACHELINE_SIZE

	/*
	 * Fix GOT pointer:
	 *
	 * New GOT-PTR = (old GOT-PTR - TEXT_BASE) + Destination Address
	 *
	 * Offset:
	 */
	sub	r15,r10,r4

	/* First our own GOT */
	add	r14,r14,r15
	/* then the one used by the C code */
	add	r30,r30,r15

	/*
	 * Now relocate code
	 */

	cmplw	cr1,r3,r4
	addi	r0,r5,3
	srwi.	r0,r0,2
	beq	cr1,4f		/* In place copy is not necessary	*/
	beq	7f		/* Protect against 0 count		*/
	mtctr	r0
	bge	cr1,2f

	la	r8,-4(r4)
	la	r7,-4(r3)
1:	lwzu	r0,4(r8)
	stwu	r0,4(r7)
	bdnz	1b
	b	4f

2:	slwi	r0,r0,2
	add	r8,r4,r0
	add	r7,r3,r0
3:	lwzu	r0,-4(r8)
	stwu	r0,-4(r7)
	bdnz	3b

	/*
	 * Now flush the cache: note that we must start from a cache aligned
	 * address. Otherwise we might miss one cache line.
	 */
4:	cmpwi	r6,0
	add	r5,r3,r5
	beq	7f		/* Always flush prefetch queue in any case */
	subi	r0,r6,1
	andc	r3,r3,r0
	mr	r4,r3
5:	dcbst	0,r4
	add	r4,r4,r6
	cmplw	r4,r5
	blt	5b
	sync			/* Wait for all dcbst to complete on bus */
	mr	r4,r3
6:	icbi	0,r4
	add	r4,r4,r6
	cmplw	r4,r5
	blt	6b
7:	sync			/* Wait for all icbi to complete on bus */
	isync

	/*
	 * We are done. Do not return, instead branch to second part of board
	 * initialization, now running from RAM.
	 */

	addi	r0,r10,in_ram - _start + _START_OFFSET

	/*
	 * As IVPR is going to point RAM address,
	 * Make sure IVOR15 has valid opcode to support debugger
	 */
	mtspr	IVOR15,r0

	/*
	 * Re-point the IVPR at RAM
	 */
	mtspr	IVPR,r10

	mtlr	r0
	blr				/* NEVER RETURNS! */

	.globl	in_ram
in_ram:

	/*
	 * Relocation Function, r14 point to got2+0x8000
	 *
	 * Adjust got2 pointers, no need to check for 0, this code
	 * already puts a few entries in the table.
	 */
	li	r0,__got2_entries@sectoff@l
	la	r3,GOT(_GOT2_TABLE_)
	lwz	r11,GOT(_GOT2_TABLE_)
	mtctr	r0
	sub	r11,r3,r11
	addi	r3,r3,-4
1:	lwzu	r0,4(r3)
	cmpwi	r0,0
	beq-	2f
	add	r0,r0,r11
	stw	r0,0(r3)
2:	bdnz	1b

	/*
	 * Now adjust the fixups and the pointers to the fixups
	 * in case we need to move ourselves again.
	 */
	li	r0,__fixup_entries@sectoff@l
	lwz	r3,GOT(_FIXUP_TABLE_)
	cmpwi	r0,0
	mtctr	r0
	addi	r3,r3,-4
	beq	4f
3:	lwzu	r4,4(r3)
	lwzux	r0,r4,r11
	cmpwi	r0,0
	add	r0,r0,r11
	stw	r4,0(r3)
	beq-	5f
	stw	r0,0(r4)
5:	bdnz	3b
4:
clear_bss:
	/*
	 * Now clear BSS segment
	 */
	lwz	r3,GOT(__bss_start)
	lwz	r4,GOT(__bss_stop)

	cmplw	0,r3,r4
	beq	6f

	li	r0,0
5:
	stw	r0,0(r3)
	addi	r3,r3,4
	cmplw	0,r3,r4
	blt	5b
6:
	mr 	r3, r10		/* Destination Address      */
	bl 	board_init_r

	/*
	 * Copy exception vector code to low memory
	 *
	 * r3: dest_addr
	 * r7: source address, r8: end address, r9: target address
	 */
	.globl	trap_init
trap_init:
	mflr	r4			/* save link register		*/
	GET_GOT
	lwz	r7,GOT(_start_of_vectors)
	lwz	r8,GOT(_end_of_vectors)

	li	r9,0x100		/* reset vector always at 0x100 */

	cmplw	0,r7,r8
	bgelr				/* return if r7>=r8 - just in case */
1:
	lwz	r0,0(r7)
	stw	r0,0(r9)
	addi	r7,r7,4
	addi	r9,r9,4
	cmplw	0,r7,r8
	bne	1b

	/*
	 * relocate `hdlr' and `int_return' entries
	 */
	li	r7,.L_CriticalInput - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_MachineCheck - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_DataStorage - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_InstStorage - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_ExtInterrupt - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_Alignment - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_ProgramCheck - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_FPUnavailable - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_Decrementer - _start + _START_OFFSET
	bl	trap_reloc
	li	r7,.L_IntervalTimer - _start + _START_OFFSET
	li	r8,_end_of_vectors - _start + _START_OFFSET
2:
	bl	trap_reloc
	addi	r7,r7,0x100		/* next exception vector	*/
	cmplw	0,r7,r8
	blt	2b

	/* Update IVORs as per relocated vector table address */
	li	r7,0x0100
	mtspr	IVOR0,r7	/* 0: Critical input */
	li	r7,0x0200
	mtspr	IVOR1,r7	/* 1: Machine check */
	li	r7,0x0300
	mtspr	IVOR2,r7	/* 2: Data storage */
	li	r7,0x0400
	mtspr	IVOR3,r7	/* 3: Instruction storage */
	li	r7,0x0500
	mtspr	IVOR4,r7	/* 4: External interrupt */
	li	r7,0x0600
	mtspr	IVOR5,r7	/* 5: Alignment */
	li	r7,0x0700
	mtspr	IVOR6,r7	/* 6: Program check */
	li	r7,0x0800
	mtspr	IVOR7,r7	/* 7: floating point unavailable */
	li	r7,0x0900
	mtspr	IVOR8,r7	/* 8: System call */
	/* 9: Auxiliary processor unavailable(unsupported) */
	li	r7,0x0a00
	mtspr	IVOR10,r7	/* 10: Decrementer */
	li	r7,0x0b00
	mtspr	IVOR11,r7	/* 11: Interval timer */
	li	r7,0x0c00
	mtspr	IVOR12,r7	/* 12: Watchdog timer */
	li	r7,0x0d00
	mtspr	IVOR13,r7	/* 13: Data TLB error */
	li	r7,0x0e00
	mtspr	IVOR14,r7	/* 14: Instruction TLB error */
	li	r7,0x0f00
	mtspr	IVOR15,r7	/* 15: Debug */

	lis	r7,0x0
	mtspr	IVPR,r7

	mtlr	r4			/* restore link register	*/
	blr


.globl _text_base
_text_base:
	.long TEXT_BASE

.globl unlock_ram_in_cache
unlock_ram_in_cache:
	/* invalidate the INIT_RAM section */
	lis	r3,(CFG_INIT_RAM_ADDR & ~(CACHELINE_SIZE-1))@h
	ori	r3,r3,(CFG_INIT_RAM_ADDR & ~(CACHELINE_SIZE-1))@l
	mfspr	r4,L1CFG0
	andi.	r4,r4,0x1ff
	slwi	r4,r4,(10 - 1 - L1_CACHE_SHIFT)
	mtctr	r4
1:	dcbi	r0,r3
	dcblc	r0,r3
	addi	r3,r3,CACHELINE_SIZE
	bdnz	1b
	sync

	/* Invalidate the TLB entries for the cache */
	lis	r3,CFG_INIT_RAM_ADDR@h
	ori	r3,r3,CFG_INIT_RAM_ADDR@l
	tlbivax	0,r3
	addi	r3,r3,0x1000
	tlbivax	0,r3
	addi	r3,r3,0x1000
	tlbivax	0,r3
	addi	r3,r3,0x1000
	tlbivax	0,r3
	isync
	blr

.globl flush_dcache
flush_dcache:
	mfspr	r3,SPRN_L1CFG0

	rlwinm	r5,r3,9,3	/* Extract cache block size */
	twlgti	r5,1		/* Only 32 and 64 byte cache blocks
				 * are currently defined.
				 */
	li	r4,32
	subfic	r6,r5,2		/* r6 = log2(1KiB / cache block size) -
				 *      log2(number of ways)
				 */
	slw	r5,r4,r5	/* r5 = cache block size */

	rlwinm	r7,r3,0,0xff	/* Extract number of KiB in the cache */
	mulli	r7,r7,13	/* An 8-way cache will require 13
				 * loads per set.
				 */
	slw	r7,r7,r6

	/* save off HID0 and set DCFA */
	mfspr	r8,SPRN_HID0
	ori	r9,r8,HID0_DCFA@l
	mtspr	SPRN_HID0,r9
	isync

	lis	r4,0
	mtctr	r7

1:	lwz	r3,0(r4)	/* Load... */
	add	r4,r4,r5
	bdnz	1b

	msync
	lis	r4,0
	mtctr	r7

1:	dcbf	0,r4		/* ...and flush. */
	add	r4,r4,r5
	bdnz	1b

	/* restore HID0 */
	mtspr	SPRN_HID0,r8
	isync

	blr

.globl setup_ivors
setup_ivors:

#include "fixed_ivor.S"
	blr
