/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Resetcode and exception/interrupt shells for Coldfire V4E
 *
 *  This file contains the common startup code for use on at least Coldfire
 *  V4E cores:
 *  - MCF547x
 *  - MCF548x
 */
#include <config.h>

	.section ".vectors","a"

/*
 * Define some addresses from your board configuration file
 */
	.equ	__MBAR,CFG_MBAR_ADDRESS
	.globl  __MBAR

	.equ	__CORE_SRAM0,CFG_CORE0_SRAM_ADDRESS
	.equ	__CORE_SRAM0_SIZE,CFG_CORE0_SRAM_SIZE

	.equ	__CORE_SRAM1,CFG_CORE1_SRAM_ADDRESS
	.equ	__CORE_SRAM1_SIZE,CFG_CORE1_SRAM_SIZE

 /*
  * Preload stack pointer with end of Core SRAM - useable _after_ you have
  * setup the MBR register in reset code!
  *
  * The upper 4 LW of the Core SRAM are left spare - it can be used as
  * fixed address temporay storage in the code below (ok, well also to
  * fix up stack traces in the debugger)
  *
  * So we have a stack usable for C code, before we even started SDRAM!
  */
	.equ ___SP_INIT,__CORE_SRAM1+__CORE_SRAM1_SIZE-16

/*
 * Vector table for M68k and barebox Link Address
 *
 * On M68k/Coldfire cores all exceptions and interrupts are routed through
 * a vector array. This vector is by default at address 0x0000_0000, but
 * can be moved to any other 1MB aligned address.
 *
 * We take advantage of this to move barebox out of low memory. Some BDM
 * debuggers won't like a moved vector base and might need tweaking to
 * work.
 *
 * Note: Respect alignment restrictions for TEXT_BASE, which must be
 *       1MB aligned.
 */

	.globl _barebox_start
_barebox_start:

VECTOR_TABLE:
_VECTOR_TABLE:
INITSP:     .long   ___SP_INIT              /* Initial SP           */
INITPC:     .long   0x410                   /* Initial PC           */
vector02:   .long   _asm_exception_handler  /* Access Error         */
vector03:   .long   _asm_exception_handler  /* Address Error        */
vector04:   .long   _asm_exception_handler  /* Illegal Instruction  */
vector05:   .long   _asm_exception_handler  /* Divide by Zero       */
vector06:   .long   _asm_exception_handler  /* Reserved             */
vector07:   .long   _asm_exception_handler  /* Reserved             */
vector08:   .long   _asm_exception_handler  /* Privilege Violation  */
vector09:   .long   _asm_exception_handler  /* Trace                */
vector0A:   .long   _asm_exception_handler  /* Unimplemented A-Line */
vector0B:   .long   _asm_exception_handler  /* Unimplemented F-Line */
vector0C:   .long   _asm_exception_handler  /* Non-PC Brkpt Debug Int */
vector0D:   .long   _asm_exception_handler  /* PC Brkpt Debug Int   */
vector0E:   .long   _asm_exception_handler  /* Format Error         */
vector0F:   .long   _asm_exception_handler  /* Unitialized Int      */
vector10:   .long   _asm_exception_handler  /* Reserved             */
vector11:   .long   _asm_exception_handler  /* Reserved             */
vector12:   .long   _asm_exception_handler  /* Reserved             */
vector13:   .long   _asm_exception_handler  /* Reserved             */
vector14:   .long   _asm_exception_handler  /* Reserved             */
vector15:   .long   _asm_exception_handler  /* Reserved             */
vector16:   .long   _asm_exception_handler  /* Reserved             */
vector17:   .long   _asm_exception_handler  /* Reserved             */
vector18:   .long   _asm_exception_handler  /* Spurious Interrupt   */
vector19:   .long   _asm_isr_handler        /* Autovector Level 1   */
vector1A:   .long   _asm_isr_handler        /* Autovector Level 2   */
vector1B:   .long   _asm_isr_handler        /* Autovector Level 3   */
vector1C:   .long   _asm_isr_handler        /* Autovector Level 4   */
vector1D:   .long   _asm_isr_handler        /* Autovector Level 5   */
vector1E:   .long   _asm_isr_handler        /* Autovector Level 6   */
vector1F:   .long   _asm_isr_handler        /* Autovector Level 7   */
vector20:   .long   _asm_exception_handler  /* TRAP #0              */
vector21:   .long   _asm_exception_handler  /* TRAP #1              */
vector22:   .long   _asm_exception_handler  /* TRAP #2              */
vector23:   .long   _asm_exception_handler  /* TRAP #3              */
vector24:   .long   _asm_exception_handler  /* TRAP #4              */
vector25:   .long   _asm_exception_handler  /* TRAP #5              */
vector26:   .long   _asm_exception_handler  /* TRAP #6              */
vector27:   .long   _asm_exception_handler  /* TRAP #7              */
vector28:   .long   _asm_exception_handler  /* TRAP #8              */
vector29:   .long   _asm_exception_handler  /* TRAP #9              */
vector2A:   .long   _asm_exception_handler  /* TRAP #10             */
vector2B:   .long   _asm_exception_handler  /* TRAP #11             */
vector2C:   .long   _asm_exception_handler  /* TRAP #12             */
vector2D:   .long   _asm_exception_handler  /* TRAP #13             */
vector2E:   .long   _asm_exception_handler  /* TRAP #14             */
vector2F:   .long   _dbug_sc_handler        /* TRAP #15 - System Call */
vector30:   .long   _asm_exception_handler  /* Reserved             */
vector31:   .long   _asm_exception_handler  /* Reserved             */
vector32:   .long   _asm_exception_handler  /* Reserved             */
vector33:   .long   _asm_exception_handler  /* Reserved             */
vector34:   .long   _asm_exception_handler  /* Reserved             */
vector35:   .long   _asm_exception_handler  /* Reserved             */
vector36:   .long   _asm_exception_handler  /* Reserved             */
vector37:   .long   _asm_exception_handler  /* Reserved             */
vector38:   .long   _asm_exception_handler  /* Reserved             */
vector39:   .long   _asm_exception_handler  /* Reserved             */
vector3A:   .long   _asm_exception_handler  /* Reserved             */
vector3B:   .long   _asm_exception_handler  /* Reserved             */
vector3C:   .long   _asm_exception_handler  /* Reserved             */
vector3D:   .long   _asm_exception_handler  /* Unsupported Instruction */
vector3E:   .long   _asm_exception_handler  /* Reserved             */
vector3F:   .long   _asm_exception_handler  /* Reserved             */
vector40:   .long   _asm_isr_handler        /* User Defined Interrupts  */
vector41:   .long   _asm_isr_handler
vector42:   .long   _asm_isr_handler
vector43:   .long   _asm_isr_handler
vector44:   .long   _asm_isr_handler
vector45:   .long   _asm_isr_handler
vector46:   .long   _asm_isr_handler
vector47:   .long   _asm_isr_handler
vector48:   .long   _asm_isr_handler
vector49:   .long   _asm_isr_handler
vector4A:   .long   _asm_isr_handler
vector4B:   .long   _asm_isr_handler
vector4C:   .long   _asm_isr_handler
vector4D:   .long   _asm_isr_handler
vector4E:   .long   _asm_isr_handler
vector4F:   .long   _asm_isr_handler
vector50:   .long   _asm_isr_handler
vector51:   .long   _asm_isr_handler
vector52:   .long   _asm_isr_handler
vector53:   .long   _asm_isr_handler
vector54:   .long   _asm_isr_handler
vector55:   .long   _asm_isr_handler
vector56:   .long   _asm_isr_handler
vector57:   .long   _asm_isr_handler
vector58:   .long   _asm_isr_handler
vector59:   .long   _asm_isr_handler
vector5A:   .long   _asm_isr_handler
vector5B:   .long   _asm_isr_handler
vector5C:   .long   _asm_isr_handler
vector5D:   .long   _asm_isr_handler
vector5E:   .long   _asm_isr_handler
vector5F:   .long   _asm_isr_handler
vector60:   .long   _asm_isr_handler
vector61:   .long   _asm_isr_handler
vector62:   .long   _asm_isr_handler
vector63:   .long   _asm_isr_handler
vector64:   .long   _asm_isr_handler
vector65:   .long   _asm_isr_handler
vector66:   .long   _asm_isr_handler
vector67:   .long   _asm_isr_handler
vector68:   .long   _asm_isr_handler
vector69:   .long   _asm_isr_handler
vector6A:   .long   _asm_isr_handler
vector6B:   .long   _asm_isr_handler
vector6C:   .long   _asm_isr_handler
vector6D:   .long   _asm_isr_handler
vector6E:   .long   _asm_isr_handler
vector6F:   .long   _asm_isr_handler
vector70:   .long   _asm_isr_handler
vector71:   .long   _asm_isr_handler
vector72:   .long   _asm_isr_handler
vector73:   .long   _asm_isr_handler
vector74:   .long   _asm_isr_handler
vector75:   .long   _asm_isr_handler
vector76:   .long   _asm_isr_handler
vector77:   .long   _asm_isr_handler
vector78:   .long   _asm_isr_handler
vector79:   .long   _asm_isr_handler
vector7A:   .long   _asm_isr_handler
vector7B:   .long   _asm_isr_handler
vector7C:   .long   _asm_isr_handler
vector7D:   .long   _asm_isr_handler
vector7E:   .long   _asm_isr_handler
vector7F:   .long   _asm_isr_handler
vector80:   .long   _asm_isr_handler
vector81:   .long   _asm_isr_handler
vector82:   .long   _asm_isr_handler
vector83:   .long   _asm_isr_handler
vector84:   .long   _asm_isr_handler
vector85:   .long   _asm_isr_handler
vector86:   .long   _asm_isr_handler
vector87:   .long   _asm_isr_handler
vector88:   .long   _asm_isr_handler
vector89:   .long   _asm_isr_handler
vector8A:   .long   _asm_isr_handler
vector8B:   .long   _asm_isr_handler
vector8C:   .long   _asm_isr_handler
vector8D:   .long   _asm_isr_handler
vector8E:   .long   _asm_isr_handler
vector8F:   .long   _asm_isr_handler
vector90:   .long   _asm_isr_handler
vector91:   .long   _asm_isr_handler
vector92:   .long   _asm_isr_handler
vector93:   .long   _asm_isr_handler
vector94:   .long   _asm_isr_handler
vector95:   .long   _asm_isr_handler
vector96:   .long   _asm_isr_handler
vector97:   .long   _asm_isr_handler
vector98:   .long   _asm_isr_handler
vector99:   .long   _asm_isr_handler
vector9A:   .long   _asm_isr_handler
vector9B:   .long   _asm_isr_handler
vector9C:   .long   _asm_isr_handler
vector9D:   .long   _asm_isr_handler
vector9E:   .long   _asm_isr_handler
vector9F:   .long   _asm_isr_handler
vectorA0:   .long   _asm_isr_handler
vectorA1:   .long   _asm_isr_handler
vectorA2:   .long   _asm_isr_handler
vectorA3:   .long   _asm_isr_handler
vectorA4:   .long   _asm_isr_handler
vectorA5:   .long   _asm_isr_handler
vectorA6:   .long   _asm_isr_handler
vectorA7:   .long   _asm_isr_handler
vectorA8:   .long   _asm_isr_handler
vectorA9:   .long   _asm_isr_handler
vectorAA:   .long   _asm_isr_handler
vectorAB:   .long   _asm_isr_handler
vectorAC:   .long   _asm_isr_handler
vectorAD:   .long   _asm_isr_handler
vectorAE:   .long   _asm_isr_handler
vectorAF:   .long   _asm_isr_handler
vectorB0:   .long   _asm_isr_handler
vectorB1:   .long   _asm_isr_handler
vectorB2:   .long   _asm_isr_handler
vectorB3:   .long   _asm_isr_handler
vectorB4:   .long   _asm_isr_handler
vectorB5:   .long   _asm_isr_handler
vectorB6:   .long   _asm_isr_handler
vectorB7:   .long   _asm_isr_handler
vectorB8:   .long   _asm_isr_handler
vectorB9:   .long   _asm_isr_handler
vectorBA:   .long   _asm_isr_handler
vectorBB:   .long   _asm_isr_handler
vectorBC:   .long   _asm_isr_handler
vectorBD:   .long   _asm_isr_handler
vectorBE:   .long   _asm_isr_handler
vectorBF:   .long   _asm_isr_handler
vectorC0:   .long   _asm_isr_handler
vectorC1:   .long   _asm_isr_handler
vectorC2:   .long   _asm_isr_handler
vectorC3:   .long   _asm_isr_handler
vectorC4:   .long   _asm_isr_handler
vectorC5:   .long   _asm_isr_handler
vectorC6:   .long   _asm_isr_handler
vectorC7:   .long   _asm_isr_handler
vectorC8:   .long   _asm_isr_handler
vectorC9:   .long   _asm_isr_handler
vectorCA:   .long   _asm_isr_handler
vectorCB:   .long   _asm_isr_handler
vectorCC:   .long   _asm_isr_handler
vectorCD:   .long   _asm_isr_handler
vectorCE:   .long   _asm_isr_handler
vectorCF:   .long   _asm_isr_handler
vectorD0:   .long   _asm_isr_handler
vectorD1:   .long   _asm_isr_handler
vectorD2:   .long   _asm_isr_handler
vectorD3:   .long   _asm_isr_handler
vectorD4:   .long   _asm_isr_handler
vectorD5:   .long   _asm_isr_handler
vectorD6:   .long   _asm_isr_handler
vectorD7:   .long   _asm_isr_handler
vectorD8:   .long   _asm_isr_handler
vectorD9:   .long   _asm_isr_handler
vectorDA:   .long   _asm_isr_handler
vectorDB:   .long   _asm_isr_handler
vectorDC:   .long   _asm_isr_handler
vectorDD:   .long   _asm_isr_handler
vectorDE:   .long   _asm_isr_handler
vectorDF:   .long   _asm_isr_handler
vectorE0:   .long   _asm_isr_handler
vectorE1:   .long   _asm_isr_handler
vectorE2:   .long   _asm_isr_handler
vectorE3:   .long   _asm_isr_handler
vectorE4:   .long   _asm_isr_handler
vectorE5:   .long   _asm_isr_handler
vectorE6:   .long   _asm_isr_handler
vectorE7:   .long   _asm_isr_handler
vectorE8:   .long   _asm_isr_handler
vectorE9:   .long   _asm_isr_handler
vectorEA:   .long   _asm_isr_handler
vectorEB:   .long   _asm_isr_handler
vectorEC:   .long   _asm_isr_handler
vectorED:   .long   _asm_isr_handler
vectorEE:   .long   _asm_isr_handler
vectorEF:   .long   _asm_isr_handler
vectorF0:   .long   _asm_isr_handler
vectorF1:   .long   _asm_isr_handler
vectorF2:   .long   _asm_isr_handler
vectorF3:   .long   _asm_isr_handler
vectorF4:   .long   _asm_isr_handler
vectorF5:   .long   _asm_isr_handler
vectorF6:   .long   _asm_isr_handler
vectorF7:   .long   _asm_isr_handler
vectorF8:   .long   _asm_isr_handler
vectorF9:   .long   _asm_isr_handler
vectorFA:   .long   _asm_isr_handler
vectorFB:   .long   _asm_isr_handler
vectorFC:   .long   _asm_isr_handler
vectorFD:   .long   _asm_isr_handler
vectorFE:   .long   _asm_isr_handler
vectorFF:   .long   _asm_isr_handler

/*
 *   Leave some bytes spare here for CW debugger (console IO stuff)
 */
	.rept 4
	.long 0xdeadbeef
	.endr

/** @func reset Startup Code (reset vector)
 *
 * The vector array is mapped to address 0 at reset and SP and PC are
 * fetched from adress 0 and 4.
 *
 * For debugger uploads this image will reside in the middle of RAM, leaving
 * as much memory for other stuff in low memory available, e.g. Linux and
 * an init ramdisk.
 *
 * For real system resets, the boot rom is mapped to all addresses in
 * system, as long as somebody sets up the CS. Now the trick part until
 * relocation to RAM is that we must code at the start of your bootrom
 * - all link addresses are wrong, so we need the reloc.h stuff to find the
 * right address.
 *
 * The following things happen here:
 * * do important init, like SDRAM, only if we don't start from memory!
 * * setup Memory and board specific bits prior to relocation.
 * * Setup stack
 * * relocate barebox to ram
 *
 */
	.globl _start
_start:
	.global reset
reset:
	/* Mask all IRQs */
	move.w  #0x2700,%sr

	/* Initialize MBAR - keep D0/D1 registers */
	move.l  #__MBAR,%d2
	movec   %d2,%MBAR
	nop

	/* Initialize RAMBAR0 - locate it on the data bus */
	move.l  #__CORE_SRAM0,%d2
	add.l   #0x21,%d2
	movec   %d2,%RAMBAR0
	nop

	/* Initialize RAMBAR1 - locate it on the data bus */
	move.l  #__CORE_SRAM1,%d2
	add.l   #0x21,%d2
	movec   %d2,%RAMBAR1
	nop

	/* Point Stack Pointer into Core SRAM temporarily */
	move.l  #___SP_INIT,%d2
	move.l  %d2,%sp
	nop

	/* Invalidate the data, instruction, and branch caches */
	/* Turn on the branch cache */
	move.l  #0x010C0100,%d2
	movec   %d2,%cacr
	nop

	/* Prepare stack top */
	clr.l	%sp@(0)
	move.l	%d0,%sp@(4)
	move.l	%d1,%sp@(8)
	clr.l	%sp@(12)

	/*
	 * This call is intended to give all developers a chance to use a
	 * standard reset vector file, but also do some special things
	 * required only on their specific CPU.
	 */
#ifdef CONFIG_ARCH_HAS_LOWLEVEL_INIT
	bsr.l	arch_init_lowlevel
	nop
#endif
	/*
	 * If the code vector table is not at TEXT_BASE and so this code
	 * as well, jump to the address mirror at FLASH ROM start address
	 *
	 * So load your image to TEXT_BASE for debugging or flash a binary
	 * image to your bootflash - code below will take proper action.
	 */
	lea.l	%pc@(VECTOR_TABLE),%a0
	move.l  #TEXT_BASE,%a1
	cmp.l   %a0,%a1
	beq.s	saveland

	/*
	 * Execution is not at TEXT_BASE. We assume entry to this code by
	 * a hardware reset and change execution to address of _FLASH_ rom.
	 */
	lea.l   %pc@(saveland),%a0       // Effective ! Address of label below
	move.l  %a0,%d0
	and.l	#0x00ffffff,%d0          // Cut away address high byte
	move.l	#CFG_FLASH_ADDRESS,%d1   // Get flash address
	and.l	#0xff000000,%d1          //   and just take base for CS0
	or.l	%d1,%d0                  // Compose new address
	move.l	%d0,%a0
	jmp	%a0@                     // Jump to flash rom address!
	nop

	/* We now either in SDRAM or FLASH START addresses, save to
	   change chip selects */
saveland:
	nop

	/*
	 * Before relocating, we have to setup RAM timing
	 * because memory timing is board-dependend, you will
	 * find a lowlevel_init.[c|S] in your board directory.
	 *
	 * Do not jump/call other barebox code here!
	 */
#ifdef CONFIG_MACH_DO_LOWLEVEL_INIT
	bsr.l	board_init_lowlevel
	nop
#endif

	/*
	 * relocate barebox Code to RAM (including copy of vectors)
	 */
relocate:
	lea.l  	%pc@(VECTOR_TABLE),%a0
	move.l  #TEXT_BASE,%a1
	move.l  #__bss_start,%a3
	cmp.l	%a0,%a1
	beq.s   skip_relocate

	/*
	 * Calculate number of long words, and copy them to RAM
	 */
	move.l  %a3,%d2
	sub.l   %a1,%d2
	asr.l	#2,%d2
copy_loop:
	move.l  %a0@+,%a1@+
	subq.l	#1,%d2
	bne.s	copy_loop

skip_relocate:

	/* Clear BSS segment in RAM */
clear_bss:
	move.l	#__bss_end,%a4
	moveq.l #0,%d2
clear_loop:
	move.l  %d2,%a3@+
	cmp.l   %a4,%a3
	ble.s    clear_loop

	/*
	 * Relocate Vectors to memory start (address 0)
	 *
	 * NOTE: It could be at other places, but debuggers expect
	 *       this table to be at address 0.
	 */
#ifdef CONFIG_COPY_LOWMEM_VECTORS
reloc_vectors:
	lea.l	%pc@(VECTOR_TABLE),%a0
	move.l  #0,%a1
	cmp.l   %a0,%a1
	beq.s	skip_copy_vectors

	move.l  #0x100,%d2
copy_loop_vectors:
	move.l	%a0@+,%a1@+
	subq.l	#1,%d2
	bne.s   copy_loop_vectors
skip_copy_vectors:
#endif

#ifndef CONFIG_USE_LOWMEM_VECTORS
	move.l	#TEXT_BASE,%d0
	movec	%d0,%vbr
	nop
#endif

#ifdef CONFIG_MACH_DO_LOWLEVEL_INIT
	/*
	 * Call other half of initcode in relocated code
	 *
	 * You allowed to call other barebox code from here
	 */
	jsr.l	board_init_highlevel
	nop
#endif
	/*
	 * Now jump to real link address and barebox entry point
	 */
	nop
	jmp.l	start_barebox
	nop
	nop

/*
 * Interrupt handling
 */

/*
 * IRQ stack frame.
 */
#define S_FRAME_SIZE	148

#define S_SP            S_A7
#define S_SR		144
#define S_PC		140

#define S_FPIAR		136
#define S_FPSR		132
#define S_FPCR		128

#define S_FP7		120
#define S_FP6		112
#define S_FP5		104
#define S_FP4		96
#define S_FP3		88
#define S_FP2		80
#define S_FP1		72
#define S_FP0		64

#define S_A7		60
#define S_A6		56
#define S_A5		52
#define S_A4		48
#define S_A3		44
#define S_A2		40
#define S_A1		36
#define S_A0		32

#define S_D7		28
#define S_D6		24
#define S_D5		20
#define S_D4		16
#define S_D3		12
#define S_D2		8
#define S_D1		4
#define S_D0		0


/*
 * exception handlers
 */
#ifdef CONFIG_USE_IRQ
	.global _dbug_sc_handler
_dbug_sc_handler:
	.global _asm_exception_handler
_asm_exception_handler:
	move.w	#0x2700,%sr	/* Disable IRQs */

	move.l  %sp,___SP_INIT   /* Remember on top of stack */
	move.l  #___SP_INIT,%sp  /* Set stack to known area */

	move.l  %a0,%sp@-
	lea	_asm_context,%a0

	movem.l %d0-%d7/%a0-%a7,%a0@

	fmovem  %fp0-%fp7,%a0@(S_FP0)
	fmove.l %fpcr,%a0@(S_FPCR)
	fmove.l %fpsr,%a0@(S_FPSR)
	fmove.l %fpiar,%a0@(S_FPIAR)

	move.l	%sp@+,%a0@(S_A0)
	move.l  %sp@,%a1
	move.l  %a1,%a0@(S_SP)
	move.l  %a1@(4),%a0@(S_PC)
	move.w  %a1@(2),%a0@(S_SR)

	jsr	cpu_cache_flush
	nop

	move.l  %a1,%sp@-
	jsr     mcf_execute_exception_handler


	lea	_asm_context,%a0
	move.l	%a0@(S_SP),%sp

	move.l  %a0@(S_D1),%d1
	move.l  %a0@(S_D0),%d0
	move.l  %a0@(S_A1),%a1
	move.l  %a0@(S_A0),%a0

	rte
	nop
	nop

	.global _asm_isr_handler
_asm_isr_handler:
	link	%a6,#-16
	movem.l %d0-%d1/%a0-%a1,%sp@

	move.w	%a6@(4),%d0
	lsr.l	#2,%d0
	andi.l  #0x0000FF,%d0
	move.l  %d0,%sp@-
	move.l	#0,%a0
	move.l  %a0,%sp@-
	jsr	mcf_execute_irq_handler
	lea	%sp@(8),%sp
	cmpi.l  #1,%d0
	beq	handled

nothandled:
	movem.l	%sp@,%d0-%d1/%a0-%a1
	unlk	%a6
	jmp     _asm_exception_handler
	nop

handled:
	movem.l	%sp@,%d0-%d1/%a0-%a1
	unlk    %a6
	rte
	nop
	nop

#else

	.global _dbug_sc_handler
_dbug_sc_handler:
	.global _asm_exception_handler
_asm_exception_handler:
	nop
	// FIXME - do something useful here
	rte

	.global _asm_isr_handler
_asm_isr_handler:
	nop
	// FIXME - do something useful here
	rte

#endif

	.data
_asm_context:
	.space  S_FRAME_SIZE,0x55


	.end
