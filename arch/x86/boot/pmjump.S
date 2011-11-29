/* ----------------------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *
 *   This file is part of the Linux kernel, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

/**
 * @file
 * @brief The actual transition into protected mode
 *
 * Note: This function is running in flat and real mode. Due to some
 * other restrictions it must running from an address space below 0x10000
 */

/**
 * @fn void protected_mode_jump(void)
 * @brief Switches the first time from real mode to flat mode
 */

#include <asm/modes.h>
#include "boot.h"

	.file "pmjump.S"
	.code16
	.section .boot.text.protected_mode_jump, "ax"

	.globl	protected_mode_jump
	.type	protected_mode_jump, @function

protected_mode_jump:
	jmp 1f			/* Short jump to serialize on 386/486 */
1:

	movw $__BOOT_DS, %cx
	movw $__BOOT_TSS, %di

	movl %cr0, %edx
	orb $X86_CR0_PE, %dl	/* enable protected mode */
	movl %edx, %cr0

	/* Transition to 32-bit flat mode */
	data32  ljmp $__BOOT_CS, $in_pm32
	.size protected_mode_jump, .-protected_mode_jump

/* ------------------------------------------------------------------------ */

	.section ".text.in_pm32","ax"
	.code32

	.extern uboot_entry
	.extern __bss_stop

	.type	in_pm32, @function
in_pm32:
	# Set up data segments for flat 32-bit mode
	movl %ecx, %ds
	movl %ecx, %es
	movl %ecx, %fs
	movl %ecx, %gs
	movl %ecx, %ss
/*
 * Our flat mode code uses its own stack area behind the bss. With this we
 * are still able to return to real mode temporarely
 */
	movl $__bss_stop + 32768, %esp

	# Set up TR to make Intel VT happy
	ltr %di

	# Clear registers to allow for future extensions to the
	# 32-bit boot protocol
	xorl %ecx, %ecx
	xorl %edx, %edx
	xorl %ebx, %ebx
	xorl %ebp, %ebp
	xorl %edi, %edi

	# Set up LDTR to make Intel VT happy
	lldt %cx

	jmp uboot_entry

	.size in_pm32, .-in_pm32

