/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
 *
 * This code was inspired by the GRUB2 project.
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
 * @brief Query the memory layout information from the BIOS
 *
 * Note: This function is running in flat and real mode. Due to some
 * other restrictions it must running from an address space below 0x10000
 */

/**
 * @fn unsigned short bios_get_memsize(void)
 * @brief Does a BIOS call "INT 15H, AH=88H" to get extended memory size
 * @return Extended memory size in KB
 *
 * @note This call is limited to 64 MiB. So, if the system provides more than
 * 64 MiB of memory, still 64 MiB are reported.
 *
 */


	.section .boot.text.bios_get_memsize, "ax"
	.code32
	.globl bios_get_memsize
	.type bios_get_memsize, @function

	.extern prot_to_real

bios_get_memsize:

	pushl %ebp

	call prot_to_real	/* enter real mode */
	.code16

	movb $0x88, %ah
	int $0x15

	movw %ax, %dx

	DATA32	call real_to_prot

	.code32

	movw %dx, %ax

	popl %ebp
	ret

	.size bios_get_memsize, .-bios_get_memsize

