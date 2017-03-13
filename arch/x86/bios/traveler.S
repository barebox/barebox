/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
 *
 * Mostly stolen from the GRUB2 project
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
 *
 */

/**
 * @file
 * @brief Switch from the flat mode world into the real mode world and vice versa
 *
 * Note: These functions are *called* and return in a different operating mode
 */

/**
 * @fn void real_to_prot(void)
 * @brief Switch from temp. real mode back to flat mode
 *
 * Called from a 32 bit flat mode segment and returns into a 16 bit segment
 */

/**
 * @fn void prot_to_real(void)
 * @brief Switch from flat mode to real mode
 *
 * Called from a 16 bit real mode segment and returns into a 32 bit segment
 */

#include <asm/modes.h>

	.file "walkyrie.S"

/* keep the current flat mode stack pointer, while playing in real mode */
	.section .boot.data.protstack
	.code32
protstack: .long 4
/* temp. store */
return_addr: .long 4


	.section .boot.text.real_to_prot, "ax"
	.code16
	.globl	real_to_prot
	.type	real_to_prot, @function

/* Note: This routine should not change any other standard registers than eax */
real_to_prot:
	/*
	 * Always disable the interrupts, when returning to flat mode
	 */
	cli

	/* turn on protected mode */
	movl %cr0, %eax
	orl $0x00000001, %eax
	movl %eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	DATA32 ljmp $__BOOT_CS, $return_to_flatmode
	.size real_to_prot, .-real_to_prot

/* ----------------------------------------------------------------------- */
	.section .boot.text.return_to_flatmode, "ax"
	.type return_to_flatmode, @function
	.code32

return_to_flatmode:
	/* reload other segment registers */
	movw $__BOOT_DS, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	/* move the return address from the real mode to the flat mode stack */
	movl (%esp), %eax
	movl %eax, return_addr

	/* setup again the flat mode stack */
	movl protstack, %eax
	movl %eax, %esp
	movl %eax, %ebp

	movl return_addr, %eax
	movl %eax, (%esp)

	/* flag we returned happy here */
	xorl %eax, %eax
	ret
	.size return_to_flatmode, .-return_to_flatmode

/* ------------------------------------------------------------------------ */

/* Note: This routine should not change any other standard registers than eax */

	.section .boot.text.prot_to_real, "ax"
	.globl prot_to_real
	.type prot_to_real, @function
	.extern boot_stack
	.code32

prot_to_real:
	/* save the protected mode stack */
	movl %esp, %eax
	movl %eax, protstack

	/* prepare the real mode stack */
	/* - address to call to the top of this stack */
	movl (%esp), %eax
	movl %eax, boot_stack - 4

	/* - the stack itself */
	movl $boot_stack - 4, %eax
	movl %eax, %esp
	movl %eax, %ebp

	/* prepare segments limits to 16 bit */
	movw $__REAL_DS, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	/* at last, also limit the code segment to 16 bit */
	ljmp $__REAL_CS, $return_to_realmode
	.size prot_to_real, .-prot_to_real

/* ----------------------------------------------------------------------- */

	.section .boot.text.return_to_realmode, "ax"
	.globl return_to_realmode
	.type return_to_realmode, @function
	.code16

return_to_realmode:
	/* disable protected mode */
	movl %cr0, %eax
	andl $(~0x00000001), %eax
	movl %eax, %cr0

	/*
	 * all the protected mode settings are still cached in the CPU.
	 * Refresh them by re-loading all registers in realmode
	 * Start with the CS, continue with the data registers
	 */
	ljmp $0, $enter_realmode

enter_realmode:
	xorl %eax, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	/*
	 * back in plain real mode now, we can play again with the BIOS
	 */

	/* restore interrupts */
	sti

	/* return on realmode stack! */
	DATA32 ret

	.size return_to_realmode, .-return_to_realmode

