/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ePAPR-compliant entry point for QEMU e500 platforms (mpc8544ds/ppce500)
 *
 * QEMU sets up the following CPU state before jumping to firmware:
 * - r1 = 16MB - 8 (initial stack)
 * - r3 = FDT physical address
 * - r6 = EPAPR_MAGIC (0x45504150)
 * - r7 = Initial TLB map size
 * - TLB1[0] = Identity map covering RAM + FDT
 */

#include <asm/config.h>
#include <asm/processor.h>
#include <asm/ppc_asm.tmpl>
#include <asm/ppc_defs.h>
#include <asm/cache.h>
#include <mach/mmu.h>

#undef MSR_KERNEL
#define MSR_KERNEL (MSR_ME)	/* Machine Check */

	.section .text_entry
	.globl _start_qemu_e500
	.globl _start

_start:
_start_qemu_e500:
	/*
	 * Entry from QEMU with ePAPR boot convention:
	 * r3 = FDT pointer (must be preserved)
	 * r6 = EPAPR_MAGIC
	 * r1 = initial stack (16MB - 8)
	 * TLB already set up by QEMU for RAM
	 */

	/* Save FDT pointer to r31 (callee-saved) */
	mr	r31, r3

	/* Enable machine check exception */
	li	r0, MSR_KERNEL
	mtmsr	r0
	isync

	/* Invalidate L1 caches */
	li	r0, 2
	mtspr	L1CSR0, r0	/* invalidate d-cache */
	mtspr	L1CSR1, r0	/* invalidate i-cache */
	isync

	/* Clear debug status */
	mfspr	r0, DBSR
	mtspr	DBSR, r0
	isync

	/* Clear and set up timer/exception registers */
	li	r0, 0
	lis	r1, 0xffff
	mtspr	DEC, r0			/* prevent dec exceptions */
	mttbl	r0			/* prevent fit & wdt exceptions */
	mttbu	r0
	mtspr	TSR, r1			/* clear all timer exception status */
	mtspr	TCR, r0			/* disable all timers */
	mtspr	ESR, r0			/* clear exception syndrome register */
	mtspr	MCSR, r0		/* machine check syndrome register */
	mtxer	r0			/* clear integer exception register */

	/* Enable machine check and timebase in HID0 */
	lis	r0, HID0_EMCP@h
	ori	r0, r0, HID0_TBEN@l
	mtspr	HID0, r0

	/* Setup IVPR for exception vectors */
	lis	r0, TEXT_BASE@h
	ori	r0, r0, TEXT_BASE@l
	mtspr	IVPR, r0
	isync

	/*
	 * Set up TLB1[1]: Map CCSRBAR (16MB, cache-inhibited, guarded)
	 *
	 * mpc8544ds: CCSRBAR at 0x0_E0000000 (MAS7 = 0x0)
	 * ppce500:   CCSRBAR at 0xF_E0000000 (MAS7 = 0xF)
	 */
	lis	r0, FSL_BOOKE_MAS0(1, 1, 0)@h
	ori	r0, r0, FSL_BOOKE_MAS0(1, 1, 0)@l
	mtspr	MAS0, r0
	lis	r0, FSL_BOOKE_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_16M)@h
	ori	r0, r0, FSL_BOOKE_MAS1(1, 1, 0, 0, BOOKE_PAGESZ_16M)@l
	mtspr	MAS1, r0
	lis	r0, FSL_BOOKE_MAS2(CFG_CCSRBAR, (MAS2_I | MAS2_G))@h
	ori	r0, r0, FSL_BOOKE_MAS2(CFG_CCSRBAR, (MAS2_I | MAS2_G))@l
	mtspr	MAS2, r0
	lis	r0, FSL_BOOKE_MAS3(CFG_CCSRBAR, 0, (MAS3_SX | MAS3_SW | MAS3_SR))@h
	ori	r0, r0, FSL_BOOKE_MAS3(CFG_CCSRBAR, 0, (MAS3_SX | MAS3_SW | MAS3_SR))@l
	mtspr	MAS3, r0
	li	r0, CFG_CCSRBAR_PHYS_HIGH
	mtspr	MAS7, r0
	isync
	msync
	tlbwe
	isync

	/*
	 * Clear BSS before calling C code.
	 */
	lis	r3, __bss_start@h
	ori	r3, r3, __bss_start@l
	lis	r4, __bss_stop@h
	ori	r4, r4, __bss_stop@l
	li	r0, 0
	cmplw	0, r3, r4
	beq	2f
1:	stw	r0, 0(r3)
	addi	r3, r3, 4
	cmplw	0, r3, r4
	blt	1b
2:

	/* Restore stack pointer (was clobbered by timer setup above) */
	lis	r1, (16 * 1024 * 1024 - 8)@h
	ori	r1, r1, (16 * 1024 * 1024 - 8)@l

	/* Clear LR for clean stack traces */
	li	r0, 0
	mtlr	r0

	/* Jump to C entry with FDT pointer in r3 */
	mr	r3, r31
	lis	r4, qemu_e500_entry@h
	ori	r4, r4, qemu_e500_entry@l
	mtctr	r4
	bctr

	/* Should never return */
	b	.

/*
 * Low-level helper functions required by barebox
 */

/* get_svr: Return System Version Register */
	.globl get_svr
get_svr:
	mfspr	r3, SVR
	blr

/* invalidate_icache: Invalidate instruction cache */
	.globl invalidate_icache
invalidate_icache:
	mfspr	r0, L1CSR1
	ori	r0, r0, L1CSR1_ICFI
	msync
	isync
	mtspr	L1CSR1, r0
	isync
	blr

/* flush_dcache: Flush data cache */
	.globl flush_dcache
flush_dcache:
	mfspr	r3, SPRN_L1CFG0
	rlwinm	r5, r3, 9, 3
	li	r4, 32
	subfic	r6, r5, 2
	slw	r5, r4, r5
	rlwinm	r7, r3, 0, 0xff
	mulli	r7, r7, 13
	slw	r7, r7, r6

	mfspr	r8, SPRN_HID0
	ori	r9, r8, HID0_DCFA@l
	mtspr	SPRN_HID0, r9
	isync

	lis	r4, 0
	mtctr	r7

1:	lwz	r3, 0(r4)
	add	r4, r4, r5
	bdnz	1b

	mtspr	SPRN_HID0, r8
	isync
	blr

/* trap_init: Initialize exception vectors - simplified for QEMU */
	.globl trap_init
trap_init:
	blr

/*
 * void e500_write_tlb(mas0, mas1, mas2, mas3, mas7)
 */
	.globl e500_write_tlb
e500_write_tlb:
	mtspr	MAS0, r3
	mtspr	MAS1, r4
	mtspr	MAS2, r5
	mtspr	MAS3, r6
	mtspr	MAS7, r7
	isync
	tlbwe
	msync
	isync
	blr

/* _text_base: Text base address variable */
	.globl _text_base
_text_base:
	.long TEXT_BASE
