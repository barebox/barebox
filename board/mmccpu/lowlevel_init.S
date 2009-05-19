/*
 * Board specific setup info
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <asm/arch/hardware.h>

#define SDRAM 0x20000000			/* address of the SDRAM */

/* values */
#define MASTER_PLL_MUL		54
#define MASTER_PLL_DIV		4

/* clocks */
#define MOR_VAL			0x00002001	/* CKGR_MOR - enable main osc. */
#define PLLAR_VAL		(0x2000BF00 | ((MASTER_PLL_MUL - 1)<< 16) | MASTER_PLL_DIV)
#define PLLBR_VAL		0x10483E0E	/* 48.054857 MHz for USB) */

#define MCKR1_VAL		0x00000100	/* slowclock */
#define MCKR2_VAL		0x00000102	/* PCK/2 = MCK Master Clock from PLLA */

#define WDTC_WDMR_VAL		0x3fff8fff	/* disable watchdog */
#define PIOD_PDR_VAL1		0xFFFF0000	/* define PDC[31:16] as DATA[31:16] */
#define PIOD_PPUDR_VAL		0xFFFF0000	/* no pull-up for D[31:16] */
#define MATRIX_EBI0CSA_VAL	0x0000010A	/* EBI0_CSA, CS1 SDRAM, CS3 NAND Flash, 1.8V memories */
#define MATRIX_EBI1CSA_VAL	0x00010100	/* EBI1_CSA, 3.3v, no pull-ups */

/* SDRAM */
#define SDRC_MR_VAL1		0		/* SDRAMC_MR Mode register */
#define SDRC_TR_VAL1		0x13c		/* SDRAMC_TR - Refresh Timer register */
#define SDRC_CR_VAL		0xc533827a	/* SDRAMC_CR - Configuration register */
#define SDRC_MDR_VAL		0		/* Memory Device Register -> SDRAM */
#define SDRC_MR_VAL2		0x00000002	/* SDRAMC_MR */
#define SDRAM_VAL1		0		/* SDRAM_BASE */
#define SDRC_MR_VAL3		4		/* SDRC_MR */
#define SDRAM_VAL2		0		/* SDRAM_BASE */
#define SDRAM_VAL3		0		/* SDRAM_BASE */
#define SDRAM_VAL4		0		/* SDRAM_BASE */
#define SDRAM_VAL5		0		/* SDRAM_BASE */
#define SDRAM_VAL6		0		/* SDRAM_BASE */
#define SDRAM_VAL7		0		/* SDRAM_BASE */
#define SDRAM_VAL8		0		/* SDRAM_BASE */
#define SDRAM_VAL9		0		/* SDRAM_BASE */
#define SDRC_MR_VAL4		3		/* SDRC_MR */
#define SDRAM_VAL10		0		/* SDRAM_BASE */
#define SDRC_MR_VAL5		0		/* SDRC_MR */
#define SDRAM_VAL11		0		/* SDRAM_BASE */
#define SDRC_TR_VAL2		0x30c		/* SDRAM_TR */
#define SDRAM_VAL12		0		/* SDRAM_BASE */

/* setup CS0 (NOR Flash) - 16-bit */
#if 1
#define SMC0_SETUP0_VAL		0x00080203	/* SMC_SETUP */
#define SMC0_PULSE0_VAL		0x0d050705	/* SMC_PULSE */
#define SMC0_CYCLE0_VAL		0x00100010	/* SMC_CYCLE */
#define SMC0_CTRL0_VAL		0x00161003	/* SMC_MODE */
#elif 0	/* slow setup */
#define SMC0_SETUP0_VAL		0x00080203	/* SMC_SETUP */
#define SMC0_PULSE0_VAL		0x0d050705	/* SMC_PULSE */
#define SMC0_CYCLE0_VAL		0x0d000d00	/* SMC_CYCLE */
#define SMC0_CTRL0_VAL		0x00111003	/* SMC_MODE */
#else	/* RONETIX' original values */
#define SMC0_SETUP0_VAL		0x0A0A0A0A	/* SMC_SETUP */
#define SMC0_PULSE0_VAL		0x0B0B0B0B	/* SMC_PULSE */
#define SMC0_CYCLE0_VAL		0x00160016	/* SMC_CYCLE */
#define SMC0_CTRL0_VAL		0x00161003	/* SMC_MODE */
#endif


#define RSTC_RMR_VAL		0xA5000301	/* user reset enable */


_TEXT_BASE:
	.word	TEXT_BASE

.globl board_init_lowlevel
board_init_lowlevel:

	mov	r5, pc		// r5 = POS1 + 4 current
POS1:
	ldr	r0, =POS1	// r0 = POS1 compile
	ldr	r2, _TEXT_BASE
	sub	r0, r0, r2	// r0 = POS1-_TEXT_BASE (POS1 relative)
	sub	r5, r5, r0	// r0 = TEXT_BASE-1
	sub	r5, r5, #4	// r1 = text base - current

	/* memory control configuration 1 */
	ldr	r0, =SMRDATA
	ldr	r2, =SMRDATA1
	ldr	r1, _TEXT_BASE
	sub	r0, r0, r1
	sub	r2, r2, r1
	add	r0, r0, r5
	add	r2, r2, r5
0:
	/* the address */
	ldr	r1, [r0], #4
	/* the value */
	ldr	r3, [r0], #4
	str	r3, [r1]
	cmp	r2, r0
	bne	0b

/*-----------------------------------------------------------------------------
;PMC Init Step 1.
;------------------------------------------------------------------------------
;- Enable the Main Oscillator
;----------------------------------------------------------------------------*/
	/* Test if main oscillator is enabled */
	ldr 	r0,=AT91C_PMC_SR
	ldr 	r1, [r0]
	ldr 	r2,=AT91C_PMC_MOSCS
	ands 	r1, r1, r2

	ldr	r1, =AT91C_CKGR_MOR

	/* Main oscillator Enable register PMC_MOR: */
	/* Enable main oscillator, OSCOUNT = 0xFF */
	ldr 	r0, =0x0000FF01
	str	r0, [r1]

	/* Reading the PMC Status register to detect when the */
	/* Main Oscillator is enabled */
	mov	r4, #AT91C_PMC_MOSCS
	ldr	r0,=AT91C_PMC_SR
MOSCS_Loop:
	ldr	r3, [r0]
	and	r3, r4, r3
	cmp	r3, #AT91C_PMC_MOSCS
	bne	MOSCS_Loop


/*-----------------------------------------------------------------------------
;PMC Init Step 2.
;------------------------------------------------------------------------------
;- Setup PLLA
;----------------------------------------------------------------------------*/
	ldr	r1, =AT91C_CKGR_PLLAR
	/* (18.432 MHz / 1) * 13 = 239 MHz */
	ldr 	r0, =PLLAR_VAL
	str	r0, [r1]

	/* Reading the PMC Status register to detect */
	/* when the PLLA is locked */
	mov	r4, #AT91C_PMC_LOCKA
	ldr	r0,=AT91C_PMC_SR
MOSCS_Loop1:
	ldr	r3, [r0]
	and	r3, r4, r3
	cmp	r3, #AT91C_PMC_LOCKA
	bne	MOSCS_Loop1


/*-----------------------------------------------------------------------------
;PMC Init Step 3.
;------------------------------------------------------------------------------
;- Switch on the Main Oscillator 18.432 MHz
;----------------------------------------------------------------------------*/
Init_MCKR:

	/* -Master Clock Controller register PMC_MCKR */
	ldr	r0, =MCKR1_VAL
	ldr	r1, =AT91C_PMC_MCKR
	str	r0, [r1]

	/* Reading the PMC Status register to detect */
	/* when the Master clock is ready */
	mov	r4, #AT91C_PMC_MCKRDY
MCKRDY_Loop:
	ldr	r1, =AT91C_PMC_SR
	ldr	r3, [r1]
	and	r3, r4, r3
	cmp	r3, #AT91C_PMC_MCKRDY
	bne	MCKRDY_Loop

	ldr	r0, =MCKR2_VAL
	ldr	r1, =AT91C_PMC_MCKR
	str	r0, [r1]

	/* Reading the PMC Status register to detect */
	/* when the Master clock is ready */
	mov	r4, #AT91C_PMC_MCKRDY
MCKRDY_Loop1:
	ldr	r1, =AT91C_PMC_SR
	ldr	r3, [r1]
	and	r3, r4, r3
	cmp	r3, #AT91C_PMC_MCKRDY
	bne	MCKRDY_Loop1

/*-----------------------------------------------------------------------------
;PMC Init Step 4.
;------------------------------------------------------------------------------
;- Setup PLLB
;----------------------------------------------------------------------------*/
	ldr	r1, = AT91C_PMC_PLLBR

	/* 48.054857 MHz = 18432000 * 72 / 14 / 2 for USB) */
	ldr 	r0, =PLLBR_VAL
	str	r0, [r1]

	/* Reading the PMC Status register to detect */
	/* when the PLLB is locked */
	mov	r4, #AT91C_PMC_LOCKB
MOSCS_Loop2:
	ldr	r1, = AT91C_PMC_SR
	ldr	r3, [r1]
	and	r3, r4, r3
	cmp	r3, #AT91C_PMC_LOCKB
	bne	MOSCS_Loop2

	/* memory control configuration 2 */
	ldr	r0, =SMRDATA1
	ldr	r2, =SMRDATA2
	ldr	r1, _TEXT_BASE
	sub	r0, r0, r1
	sub	r2, r2, r1
	add	r0, r0, r5
	add	r2, r2, r5

2:
	/* the address */
	ldr	r1, [r0], #4
	/* the value */
	ldr	r3, [r0], #4
	str	r3, [r1]
	cmp	r2, r0
	bne	2b

	/* everything is fine now */
	mov	pc, lr

	.ltorg

SMRDATA:
	.word AT91C_WDTC_WDMR
	.word WDTC_WDMR_VAL
	.word AT91C_PIOD_PDR
	.word PIOD_PDR_VAL1
	.word AT91C_PIOD_PPUDR
	.word PIOD_PPUDR_VAL
	.word AT91C_PIOD_ASR
	.word PIOD_PPUDR_VAL

	.word AT91C_CCFG_EBI0CSA
	.word MATRIX_EBI0CSA_VAL
	.word AT91C_CCFG_EBI1CSA
	.word MATRIX_EBI1CSA_VAL

	/* flash */
	.word AT91C_SMC0_CTRL0
	.word SMC0_CTRL0_VAL

	.word AT91C_SMC0_CYCLE0
	.word SMC0_CYCLE0_VAL

	.word AT91C_SMC0_PULSE0
	.word SMC0_PULSE0_VAL

	.word AT91C_SMC0_SETUP0
	.word SMC0_SETUP0_VAL

SMRDATA1:

	.word AT91C_SDRAMC0_MR
	.word SDRC_MR_VAL1
	.word AT91C_SDRAMC0_TR
	.word SDRC_TR_VAL1
	.word AT91C_SDRAMC0_CR
	.word SDRC_CR_VAL
	.word AT91C_SDRAMC0_MDR
	.word SDRC_MDR_VAL
	.word AT91C_SDRAMC0_MR
	.word SDRC_MR_VAL2
	.word SDRAM
	.word SDRAM_VAL1
	.word AT91C_SDRAMC0_MR
	.word SDRC_MR_VAL3
	.word SDRAM
	.word SDRAM_VAL2
	.word SDRAM
	.word SDRAM_VAL3
	.word SDRAM
	.word SDRAM_VAL4
	.word SDRAM
	.word SDRAM_VAL5
	.word SDRAM
	.word SDRAM_VAL6
	.word SDRAM
	.word SDRAM_VAL7
	.word SDRAM
	.word SDRAM_VAL8
	.word SDRAM
	.word SDRAM_VAL9
	.word AT91C_SDRAMC0_MR
	.word SDRC_MR_VAL4
	.word SDRAM
	.word SDRAM_VAL10
	.word AT91C_SDRAMC0_MR
	.word SDRC_MR_VAL5
	.word SDRAM
	.word SDRAM_VAL11
	.word AT91C_SDRAMC0_TR
	.word SDRC_TR_VAL2
	.word SDRAM
	.word SDRAM_VAL12
	/* User reset enable */
	.word AT91C_RSTC_RMR
	.word RSTC_RMR_VAL
	/* MATRIX_MCFG - REMAP all masters */

SMRDATA2:
	.word 0
