/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix */
/* SPDX-FileCopyrightText: 1999-2008 Free Software Foundation, Inc. */

/* Mostly stolen from the GRUB2 project */

/**
 * @file
 * @brief Do BIOS calls to load or save data from disks
 *
 * @note These functions are running in flat and real mode. Due to some
 * other restrictions these routines must running from an address
 * space below 0x10000
 */

/*
 *   int bios_disk_rw_int13_extensions (int ah, int drive, void *dap)
 *
 *   Call IBM/MS INT13 Extensions (int 13 %ah=AH) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */
	.section .boot.text.bios_disk_rw_int13_extensions, "ax"
	.code32
	.globl bios_disk_rw_int13_extensions
	.type bios_disk_rw_int13_extensions, @function

	.extern prot_to_real
	.extern real_to_prot

bios_disk_rw_int13_extensions:
	pushl %ebp
	pushl %esi

	/* compute the address of disk_address_packet */
	movw %cx, %si
	xorw %cx, %cx
	shrl $4, %ecx	/* save the segment to cx */

	movb %al, %dh
	call prot_to_real	/* enter real mode right now */

	.code16
	movb %dh, %ah
	movw %cx, %ds
	int $0x13		/* do the operation */
	movb %ah, %dl		/* save return value */
	/* back to protected mode */
	DATA32 call real_to_prot

	.code32
	movb %dl, %al	/* return value in %eax */

	popl %esi
	popl %ebp

	ret
