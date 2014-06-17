/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
 *
 * Mostly stolen from the GRUB2 project
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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
