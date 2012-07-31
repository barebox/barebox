/*
 * (C) Copyright 2009
 * Juergen Beisert <kernel@pengutronix.de>
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
 */

#include <config.h>
#include <mach/s3c-iomap.h>

	.section ".text_bare_init.s3c24x0_disable_wd","ax"

/*
 * Disable the watchdog, else it continues to bark
 */
.globl s3c24x0_disable_wd
s3c24x0_disable_wd:

	ldr r0, =S3C_WATCHDOG_BASE
	mov r1, #0x0
	str r1, [r0]
	mov pc, lr

/**
@page dev_s3c24xx_wd_handling Watchdog handling

The watchdog must be disabled very early, because if it resets the system
it is still active and will continue to reset the system. So, call this
routine very early in your board_init_lowlevel routine.
*/

/*
 * S3C2410 PLL configuration
 * -------------------------
 *
 * Basic frequency calculation
 *
 *            m * REFclk         s = SDIV
 * PLLclk = ------------         p = PDIV + 2
 *             p * 2^s           m = MDIV + 8
 *
 * After reset the PLL of the s3c2410 processor uses:
 *
 *         MPLL   UPLL
 *  MDIV   0x5c   0x28
 *  PDIV   0x08   0x08
 *  SDIV   0x0    0x0
 *
 *            100 * 12MHz     1200MHz
 * MPLLclk = ------------- = -------- = 120MHz
 *             10 * 2^0         10
 *
 *            48 * 12MHz      576MHz
 * UPLLclk = ------------- = -------- = 57,6MHz
 *             10 * 2^0        10
 *
 * Note: Do not use "r10" here in this code
 */

#ifdef CONFIG_S3C_PLL_INIT

	.section ".text_bare_init.s3c24x0_pll_init","ax"

.globl s3c24x0_pll_init
s3c24x0_pll_init:

	mov r0, #S3C_CLOCK_POWER_BASE

	/* configure internal clock ratio */
	mov r1, #BOARD_SPECIFIC_CLKDIVN
	str r1, [r0, #20]

	/* enable all devices on this chip */
	mov r1, #0xFFFFFFF0
	str r1, [r0, #12]

	/* ??????? */
#ifdef CONFIG_CPU_S3C2440
	mov r1, #0xFFFFFFFF
#endif
#ifdef CONFIG_CPU_S3C2410
	mov r1, #0x00FFFFFF
#endif
	str r1, [r0, #0]

#ifdef CONFIG_CPU_S3C2440
	/*
	 * Most of the time HDIVN is not 0, so we must use the
	 * asynchronous bus mode (refer datasheet "Clock and Power Management")
	 */
	mrc p15, 0, r1, c1, c0, 0
	orr r1, r1, #0xc0000000
	mcr p15, 0, r1, c1, c0, 0
#endif

	/* configure UPLL */
	ldr r1, =BOARD_SPECIFIC_UPLL
	str r1, [r0, #8]

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	/* configure MPLL */
	ldr r1, =BOARD_SPECIFIC_MPLL
	str r1, [r0, #4]

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	mov pc, lr

#endif

/**
@page dev_s3c24xx_pll_handling PLL clock handling

To control the speed of your machine the PLLs must be reconfigured after reset.

For example the S3C2410 CPU wakes up after reset at 120MHz main PLL speed,
shared with all other system on chip components. Most of the time this
configuration is to slow for the CPU and to fast for the other components.

PLL reprogramming can be done in the machine specific manner very early when
the CONFIG_S3C_PLL_INIT and CONFIG_MACH_HAS_LOWLEVEL_INIT symbols are
defined. The board must provide a board_init_lowlevel() assembler function in
this case and calling the s3c24x0_pll_init() assembler function.

If the s3c24x0_pll_init() is called a few further symbols must be defined to
setup the correct values for the machine.

Define in the machine specific config.h the following symbols:

- S3C24XX_CLOCK_REFERENCE with the frequency in Hz of your reference crystal.
- BOARD_SPECIFIC_CLKDIVN with the value for the main clock ratio register (CLKDIVN)
- BOARD_SPECIFIC_MPLL with the value for the main PLL setup register
- BOARD_SPECIFIC_UPLL with the value for the USB PLL setup register

@note Valid values for the PLL settings can be found in the CPU manual.

@par Background: PLL frequency calculation for the S3C2410 CPU (both PLLs) and S3C2440 (UPLL only)

@f[
	f_{PLL} = \frac{m * f_{Ref}}{p * 2^s}
@f]

With m = MDIV + 8, p = PDIV + 2 and s = SDIV.

@par Background: PLL frequency calculation for the S3C2440 CPU (MPLL only)

@f[
	f_{PLL} = \frac{2 * m * f_{Ref}}{p * 2^s}
@f]

With m = MDIV + 8, p = PDIV + 2 and s = SDIV.

@note This routine can be used for the S3C2410 and the S3C2440 CPU.

*/

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_S3C_SDRAM_INIT

	.section ".text_bare_init.s3c24x0_sdram_init","ax"

	.globl s3c24x0_sdram_init
s3c24x0_sdram_init:

	adr r0, SDRAMDATA	/* get the current relative address of the table */
	mov r1, #S3C_MEMCTL_BASE
	mov r2, #6		/* we *know* it contains 6 entries */

	ldr r3, [r0], #4	/* write BSWCON first */
	str r3, [r1], #0x1c	/* post add register offset for bank6 */
/*
 * Initializing the SDRAM controller is very simple:
 * Just write some useful values into the SDRAM controller.
 */
0:	ldr r3, [r0], #4
	str r3, [r1], #4
	subs r2, r2, #1
	bne 0b

	mov pc, lr

SDRAMDATA:
	.word BOARD_SPECIFIC_BWSCON
	.word BOARD_SPECIFIC_BANKCON6
	.word BOARD_SPECIFIC_BANKCON7
	.word BOARD_SPECIFIC_REFRESH
	.word BOARD_SPECIFIC_BANKSIZE
	.word BOARD_SPECIFIC_MRSRB6
	.word BOARD_SPECIFIC_MRSRB7

#endif

/**
@page dev_s3c24xx_sdram_handling SDRAM controller initialisation

The SDRAM controller is very simple and its initialisation requires only a
few steps. barebox provides a generic routine to do this step.

Enable CONFIG_S3C_SDRAM_INIT and CONFIG_MACH_HAS_LOWLEVEL_INIT to be able
to call the generic s3c24x0_sdram_init() assembler function from within the
machine specific board_init_lowlevel() assembler function.

To use the s3c24x0_sdram_init() assembler function a few symbols must be
defined to setup correct values for the machine.

Define in the machine specific config.h the following list of symbols:

- BOARD_SPECIFIC_BWSCON with the values for SDRAM banks 6 and 7
- BOARD_SPECIFIC_BANKCON6 with the value for the BANKCON6 register
- BOARD_SPECIFIC_BANKCON7 with the value for the BANKCON7 register
- BOARD_SPECIFIC_REFRESH with the value for the REFRESH register
- BOARD_SPECIFIC_BANKSIZE with the value for the BANKSIZE register
- BOARD_SPECIFIC_MRSRB6 with the value for the MRSRB6 register
- BOARD_SPECIFIC_MRSRB7 with the value for the MRSRB7 register
*/

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_S3C_NAND_BOOT

	.section ".text_bare_init.s3c24x0_nand_boot","ax"

	.globl s3c24x0_nand_boot
s3c24x0_nand_boot:
/*
 * In the case of NOR boot we are running from the same address space.
 * Detect this case to handle it correctly.
 */
	mov r1, #S3C_MEMCTL_BASE
	ldr r3, [r1]
	and r3, r3, #0x6
	cmp r3, #0x0	/* check for NAND case */
	beq 2f
	mov pc, lr	/* NOR case: nothing to do here */

2:	ldr sp, =TEXT_BASE	/* Setup a temporary stack in SDRAM */
/*
 * We still run at a location we are not linked to. But lets still running
 * from the internal SRAM, this may speed up the boot
 */
	push {lr}
	bl nand_boot
	pop {lr}
/*
 * Adjust the return address to the correct address in SDRAM
 */
	ldr r1, =TEXT_BASE
	add lr, lr, r1

	mov pc, lr

#endif

/**
@page dev_s3c24xx_nandboot_handling Booting from NAND

To be able to boot from NAND memory only, enable the S3C24x0 NAND driver. Also
enable CONFIG_S3C_NAND_BOOT and CONFIG_MACH_HAS_LOWLEVEL_INIT to be
able to call the s3c24x0_nand_boot() assembler routine from within the
machine specific board_init_lowlevel() assembler function.

@note This routine assumes an already working SDRAM controller and
an initialized stack pointer.

@note Basicly this routine runs from inside the internal SRAM. After load of
the whole barebox image from the NAND flash memory into the SDRAM it adjusts
the link register to the final SDRAM adress and returns.

@note In the NAND boot mode, ECC is not checked. So, the first x KBytes used
by barebox should have no bit error.

Due to the fact the code to load the whole barebox from NAND must fit into
the first 4kiB of the barebox image, the shrinked NAND driver is very
minimalistic. Setup the NAND access timing is done in a safe manner, what
means: Slowest possible values are used. If you want to increase the speed you
should define the BOARD_DEFAULT_NAND_TIMING to a valid setting into the
NFCONF register and add it to your board specific config.h. Refer S3C24x0's
datasheet for further details. The macro #CALC_NFCONF_TIMING could help to
calculate the register setting in a hardware independent manner.

@note The regular NAND driver uses a platform data structure to define the
NAND access timings.

@note Its still possible to boot this image from NOR memory. If this routine
detects it is running from NOR instead of the internal SRAM it skips any
loading and returns immediately.

*/
