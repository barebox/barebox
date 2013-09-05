/*
 * This was originally from the Lubbock u-boot port.
 *
 * Most of this taken from Redboot hal_platform_setup.h with cleanup
 *
 * NOTE: I haven't clean this up considerably, just enough to get it
 * running. See hal_platform_setup.h for the source. See
 * board/cradle/lowlevel_init.S for another PXA250 setup that is
 * much cleaner.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <config.h>
#include <sizes.h>
#include <mach/pxa-regs.h>
#include <mach/regs-ost.h>
#include <mach/regs-intc.h>
#include <asm/barebox-arm-head.h>

#define GPSR0		0x40E00018	/* GPIO Pin Output Set Register GPIO <31:00> */
#define GPSR1		0x40E0001C	/* GPIO Pin Output Set Register GPIO <63:32> */
#define GPSR2		0x40E00020	/* GPIO Pin Output Set Register GPIO <80:64> */

#define GPCR0		0x40E00024	/* GPIO Pin Output Clear Register GPIO <31:00> */
#define GPCR1		0x40E00028	/* GPIO Pin Output Clear Register GPIO <63:32> */
#define GPCR2		0x40E0002C	/* GPIO Pin Output Clear Register GPIO <80:64> */

#define GPDR0		0x40E0000C	/* GPIO Pin Direction Register GPIO <31:0o> */
#define GPDR1		0x40E00010	/* GPIO Pin Direction Register GPIO <63:32> */
#define GPDR2		0x40E00014	/* GPIO Pin Direction Register GPIO <80:64> */

#define GAFR0_L		0x40E00054	/* GPIO Alternate Function Select Register GPIO <15:00> */
#define GAFR0_U		0x40E00058	/* GPIO Alternate Function Select Register GPIO <31:16> */
#define GAFR1_L		0x40E0005C	/* GPIO Alternate Function Select Register GPIO <47:32> */
#define GAFR1_U		0x40E00060	/* GPIO Alternate Function Select Register GPIO <63:48> */
#define GAFR2_L		0x40E00064	/* GPIO Alternate Function Select Register GPIO <79:64> */
#define GAFR2_U		0x40E00068	/* GPIO Alternate Function Select Register GPIO <95:80> */

/*
 *	Memory setup
 */
.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
		bl	arm_cpu_lowlevel_init

		@ Preserve r8/r7 i.e. kernel entry values

		@ Data cache might be active.
		@ Be sure to flush kernel binary out of the cache,
		@ whatever state it is, before it is turned off.
		@ This is done by fetching through currently executed
		@ memory to be sure we hit the same cache.
		bic	r2, pc, #0x1f
		add	r3, r2, #0x10000	@ 64 kb is quite enough...
1:		ldr	r0, [r2], #32
		teq	r2, r3
		bne	1b
		mcr	p15, 0, r0, c7, c10, 4	@ drain WB
		mcr	p15, 0, r0, c7, c7, 0	@ flush I & D caches

		@ disabling MMU and caches
		mrc	p15, 0, r0, c1, c0, 0	@ read control reg
		bic	r0, r0, #0x05		@ clear DC, MMU
		bic	r0, r0, #0x1000		@ clear Icache
		mcr	p15, 0, r0, c1, c0, 0
	/* set output */
	ldr	r0, =GPSR0
	ldr	r1, =CONFIG_GPSR0_VAL
	str	r1, [r0]

	ldr	r0, =GPSR1
	ldr	r1, =CONFIG_GPSR1_VAL
	str	r1, [r0]

	ldr	r0, =GPSR2
	ldr	r1, =CONFIG_GPSR2_VAL
	str	r1, [r0]

	/* set direction */
	ldr	r0, =GPDR0
	ldr	r1, =CONFIG_GPDR0_VAL
	str	r1, [r0]

	ldr	r0, =GPDR1
	ldr	r1, =CONFIG_GPDR1_VAL
	str	r1, [r0]

	ldr	r0, =GPDR2
	ldr	r1, =CONFIG_GPDR2_VAL
	str	r1, [r0]

	/* alternate function */
	ldr	r0, =GAFR0_L
	ldr	r1, =CONFIG_GAFR0_L_VAL
	str	r1, [r0]

	ldr	r0, =GAFR0_U
	ldr	r1, =CONFIG_GAFR0_U_VAL
	str	r1, [r0]

	ldr	r0, =GAFR1_L
	ldr	r1, =CONFIG_GAFR1_L_VAL
	str	r1, [r0]

	ldr	r0, =GAFR1_U
	ldr	r1, =CONFIG_GAFR1_U_VAL
	str	r1, [r0]

	ldr	r0, =GAFR2_L
	ldr	r1, =CONFIG_GAFR2_L_VAL
	str	r1, [r0]

	ldr	r0, =GAFR2_U
	ldr	r1, =CONFIG_GAFR2_U_VAL
	str	r1, [r0]

	/* enable GPIO pins */
	ldr	r0, =PSSR
	ldr	r1, =CONFIG_PSSR_VAL
	str	r1, [r0]

	/* -------------------------------------------------------------------- */
	/* Enable memory interface						*/
	/*									*/
	/* The sequence below is based on the recommended init steps		*/
	/* detailed in the Intel PXA250 Operating Systems Developers Guide,	*/
	/* Chapter 10.								*/
	/* -------------------------------------------------------------------- */

	/* -------------------------------------------------------------------- */
	/* Step 1: Wait for at least 200 microsedonds to allow internal		*/
	/*	   clocks to settle. Only necessary after hard reset...		*/
	/*	   FIXME: can be optimized later				*/
	/* -------------------------------------------------------------------- */

	ldr	r3, =OSCR			/* reset the OS Timer Count to zero */
	mov	r2, #0
	str	r2, [r3]
	ldr	r4, =0x300			/* really 0x2E1 is about 200usec, */
						/* so 0x300 should be plenty */
1:
	ldr	r2, [r3]
	cmp	r4, r2
	bgt	1b

	cmp	pc, #0xa0000000
	bls	mem_init
	cmp	pc, #0xb0000000
	bhi	mem_init
	b	skip_mem_init

mem_init:
	ldr	r1, =MDCNFG			/* get memory controller base addr. */

	/* -------------------------------------------------------------------- */
	/* Step 2a: Initialize Asynchronous static memory controller		*/
	/* -------------------------------------------------------------------- */

	/* MSC registers: timing, bus width, mem type */

	/* MSC0: nCS(0,1) */
	ldr	r2, =CONFIG_MSC0_VAL
	str	r2, [r1, #MSC0_OFFSET]
	ldr	r2, [r1, #MSC0_OFFSET]		/* read back to ensure */
						/* that data latches */
	/* MSC1: nCS(2,3) */
	ldr	r2, =CONFIG_MSC1_VAL
	str	r2, [r1, #MSC1_OFFSET]
	ldr	r2, [r1, #MSC1_OFFSET]

	/* MSC2: nCS(4,5) */
	ldr	r2, =CONFIG_MSC2_VAL
	str	r2, [r1, #MSC2_OFFSET]
	ldr	r2, [r1, #MSC2_OFFSET]

	/* -------------------------------------------------------------------- */
	/* Step 2b: Initialize Card Interface					*/
	/* -------------------------------------------------------------------- */

	/* MECR: Memory Expansion Card Register	*/
	ldr	r2, =CONFIG_MECR_VAL
	str	r2, [r1, #MECR_OFFSET]
	ldr	r2, [r1, #MECR_OFFSET]

	/* MCMEM0: Card Interface slot 0 timing	*/
	ldr	r2, =CONFIG_MCMEM0_VAL
	str	r2, [r1, #MCMEM0_OFFSET]
	ldr	r2, [r1, #MCMEM0_OFFSET]

	/* MCMEM1: Card Interface slot 1 timing	*/
	ldr	r2, =CONFIG_MCMEM1_VAL
	str	r2, [r1, #MCMEM1_OFFSET]
	ldr	r2, [r1, #MCMEM1_OFFSET]

	/* MCATT0: Card Interface Attribute Space Timing, slot 0 */
	ldr	r2, =CONFIG_MCATT0_VAL
	str	r2, [r1, #MCATT0_OFFSET]
	ldr	r2, [r1, #MCATT0_OFFSET]

	/* MCATT1: Card Interface Attribute Space Timing, slot 1 */
	ldr	r2, =CONFIG_MCATT1_VAL
	str	r2, [r1, #MCATT1_OFFSET]
	ldr	r2, [r1, #MCATT1_OFFSET]

	/* MCIO0: Card Interface I/O Space Timing, slot 0 */
	ldr	r2, =CONFIG_MCIO0_VAL
	str	r2, [r1, #MCIO0_OFFSET]
	ldr	r2, [r1, #MCIO0_OFFSET]

	/* MCIO1: Card Interface I/O Space Timing, slot 1 */
	ldr	r2, =CONFIG_MCIO1_VAL
	str	r2, [r1, #MCIO1_OFFSET]
	ldr	r2, [r1, #MCIO1_OFFSET]

	/* -------------------------------------------------------------------- */
	/* Step 2c: Write FLYCNFG FIXME: what's that???				*/
	/* -------------------------------------------------------------------- */
	ldr	r2, =CONFIG_FLYCNFG_VAL
	str	r2, [r1, #FLYCNFG_OFFSET]
	str	r2, [r1, #FLYCNFG_OFFSET]

	/* -------------------------------------------------------------------- */
	/* Step 2d: Initialize Timing for Sync Memory (SDCLK0)			*/
	/* -------------------------------------------------------------------- */

	/* Before accessing MDREFR we need a valid DRI field, so we set	*/
	/* this to power on defaults + DRI field. */

	ldr	r4, [r1, #MDREFR_OFFSET]
	ldr	r2, =0xFFF
	bic	r4, r4, r2

	ldr	r3, =CONFIG_MDREFR_VAL
	and	r3, r3, r2

	orr	r4, r4, r3
	str	r4, [r1, #MDREFR_OFFSET]	/* write back MDREFR */

	orr	r4, r4, #MDREFR_K0RUN
	orr	r4, r4, #MDREFR_K0DB4
	orr	r4, r4, #MDREFR_K0FREE
	orr	r4, r4, #MDREFR_K2FREE
	orr	r4, r4, #MDREFR_K0DB2
	orr	r4, r4, #MDREFR_K1DB2
	bic	r4, r4, #MDREFR_K1FREE

	str	r4, [r1, #MDREFR_OFFSET]	/* write back MDREFR */
	ldr	r4, [r1, #MDREFR_OFFSET]

	/* Note: preserve the mdrefr value in r4 */


	/* -------------------------------------------------------------------- */
	/* Step 3: Initialize Synchronous Static Memory (Flash/Peripherals)	*/
	/* -------------------------------------------------------------------- */

	/* Initialize SXCNFG register. Assert the enable bits */

	/*
	 * Write SXMRS to cause an MRS command to all enabled banks of
	 * synchronous static memory. Note that SXLCR need not be
	 * written at this time.
	 */
	ldr	r2, =CONFIG_SXCNFG_VAL
	str	r2, [r1, #SXCNFG_OFFSET]

	/* -------------------------------------------------------------------- */
	/* Step 4: Initialize SDRAM						*/
	/* -------------------------------------------------------------------- */
	bic	r4, r4, #(MDREFR_K1FREE | MDREFR_K0FREE)

	orr	r4, r4, #MDREFR_K1RUN
	orr	r4, r4, #MDREFR_K2FREE
	bic	r4, r4, #MDREFR_K2DB2
	str	r4, [r1, #MDREFR_OFFSET]
	ldr	r4, [r1, #MDREFR_OFFSET]

	bic	r4, r4, #MDREFR_SLFRSH
	str	r4, [r1, #MDREFR_OFFSET]
	ldr	r4, [r1, #MDREFR_OFFSET]

	orr	r4, r4, #MDREFR_E1PIN
	str	r4, [r1, #MDREFR_OFFSET]
	ldr	r4, [r1, #MDREFR_OFFSET]

	nop
	nop


	/*
	 * Step 4d: write MDCNFG with MDCNFG:DEx deasserted
	 * (set to 0), to configure but not enable each SDRAM
	 * partition pair.
	 */
	ldr	r4, =CONFIG_MDCNFG_VAL
	bic	r4, r4,	#(MDCNFG_DE0|MDCNFG_DE1)
	bic	r4, r4,	#(MDCNFG_DE2|MDCNFG_DE3)

	str	r4, [r1, #MDCNFG_OFFSET]	/* write back MDCNFG */
	ldr	r4, [r1, #MDCNFG_OFFSET]


	/*
	 * Step 4e: Wait for the clock to the SDRAMs to stabilize,
	 * 100..200 usec.
	 */
	ldr	r3, =OSCR			/* reset the OS Timer Count to zero */
	mov	r2, #0
	str	r2, [r3]
	ldr	r4, =0x300			/* really 0x2E1 is about 200 usec,  */
						/* so 0x300 should be plenty */
1:
	ldr	r2, [r3]
	cmp	r4, r2
	bgt	1b


	/* Step 4f: Trigger a number (usually 8) refresh cycles by		*/
	/*		attempting non-burst read or write accesses to disabled */
	/*		SDRAM, as commonly specified in the power up sequence   */
	/*		documented in SDRAM data sheets. The address(es) used   */
	/*		for this purpose must not be cacheable.			*/
	ldr	r3, =CONFIG_DRAM_BASE
	str	r2, [r3]
	str	r2, [r3]
	str	r2, [r3]
	str	r2, [r3]
	str	r2, [r3]
	str	r2, [r3]
	str	r2, [r3]
	str	r2, [r3]


	/*
	 * Step 4g: Write MDCNFG with enable bits asserted
	 * (MDCNFG:DEx set to 1)
	 */
	ldr	r3, [r1, #MDCNFG_OFFSET]
	mov	r4, r3
	orr	r3, r3,	#MDCNFG_DE0
	str	r3, [r1, #MDCNFG_OFFSET]
	mov	r0, r3

	/* Step 4h: Write MDMRS. */
	ldr	r2, =CONFIG_MDMRS_VAL
	str	r2, [r1, #MDMRS_OFFSET]

	/* enable APD */
	ldr	r3, [r1, #MDREFR_OFFSET]
	orr	r3, r3, #MDREFR_APD
	str	r3, [r1, #MDREFR_OFFSET]

	/* We are finished with Intel's memory controller initialisation */
skip_mem_init:

wakeup:
	/* Are we waking from sleep? */
	ldr	r0, =RCSR
	ldr	r1, [r0]
	and	r1, r1, #(RCSR_GPR | RCSR_SMR | RCSR_WDR | RCSR_HWR)
	str	r1, [r0]
	teq	r1, #RCSR_SMR

	bne	initirqs

	ldr	r0, =PSSR
	mov	r1, #PSSR_PH
	str	r1, [r0]

	/* if so, resume at PSPR */
	ldr	r0, =PSPR
	ldr	r1, [r0]
	mov	pc, r1

	/* -------------------------------------------------------------------- */
	/* Disable (mask) all interrupts at interrupt controller		*/
	/* -------------------------------------------------------------------- */

initirqs:
	mov	r1, #0		/* clear int. level register (IRQ, not FIQ) */
	ldr	r2, =ICLR
	str	r1, [r2]

	ldr	r2, =ICMR	/* mask all interrupts at the controller */
	str	r1, [r2]

	/* -------------------------------------------------------------------- */
	/* Clock initialisation							*/
	/* -------------------------------------------------------------------- */

initclks:
	/* Disable the peripheral clocks, and set the core clock frequency */

	/* Turn Off on-chip peripheral clocks (except for memory) */
	/* for re-configuration. */
	ldr	r1, =CKEN
	ldr	r2, =CONFIG_CKEN
	str	r2, [r1]

	/* ... and write the core clock config register */
	ldr	r2, =CONFIG_CCCR
	ldr	r1, =CCCR
	str	r2, [r1]

	/* Turn on turbo mode */
	mrc	p14, 0, r2, c6, c0, 0
	orr	r2, r2, #0xB			/* Turbo, Fast-Bus, Freq change */
	mcr	p14, 0, r2, c6, c0, 0

	/* Re-write MDREFR */
	ldr	r1, =MDCNFG
	ldr	r2, [r1, #MDREFR_OFFSET]
	str	r2, [r1, #MDREFR_OFFSET]

	/* enable the 32Khz oscillator for RTC and PowerManager */
	ldr	r1, =OSCC
	mov	r2, #OSCC_OON
	str	r2, [r1]

	/* Interrupt init: Mask all interrupts */
	ldr	r0, =ICMR			/* enable no sources */
	mov	r1, #0
	str	r1, [r0]
	/* FIXME */

#ifdef NODEBUG
	/* Disable software and data breakpoints */
	mov	r0, #0
	mcr	p15, 0, r0, c14, c8, 0		/* ibcr0 */
	mcr	p15, 0, r0, c14, c9, 0		/* ibcr1 */
	mcr	p15, 0, r0, c14, c4, 0		/* dbcon */

	/* Enable all debug functionality */
	mov	r0, #0x80000000
	mcr	p14, 0, r0, c10, c0, 0		/* dcsr */
#endif

	/* -------------------------------------------------------------------- */
	/* End lowlevel_init							*/
	/* -------------------------------------------------------------------- */

endlowlevel_init:
	mov	r0, #0xa0000000
	mov	r1, #SZ_64M
	mov	r2, #0
	b	barebox_arm_entry
