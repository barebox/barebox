/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix */
/* SPDX-FileCopyrightText: 1999-2008 Free Software Foundation, Inc. */

/* Mostly stolen from the GRUB2 project */

/**
 * @file
 * @brief Start the Linux real mode setup code
 *
 * Note: These functions are running in flat and real mode. Due to some
 * other restrictions these routines must running from an address
 * space below 0x10000
 */

/*
 *   void bios_start_linux(unsigned segment)
 *
 */

	.section .boot.text.bios_start_linux, "ax"
	.code32
	.globl bios_start_linux
	.type bios_start_linux, @function

	.extern prot_to_real

bios_start_linux:
	/* 'prot_to_real' eats our eax content */
	movl %eax, %ebx
	addl $0x20, %eax
	movw %ax, setup_seg

	call prot_to_real

	.code16

	cli
	/* all segment registers are using the same segment */
	movw %bx, %ss
	movw %bx, %ds
	movw %bx, %es
	movw %bx, %fs
	movw %bx, %gs

	/* stack for the setup code (end of heap) */
	movw $0x9000, %sp

	/* do an 'ljmp' and never return */
	.byte	0xea
	.word	0
setup_seg:
	.word	0

	.code32
