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
 * @brief Common boot sector main routine to be entered by the BIOS
 */
/**
 * @fn void _start(void)
 *
 * @brief Fix segment:offset settings of some buggy BIOSs
 */


	.file "boot_main.S"
	.code16

	.extern real_start

	.section .boot_start, "ax"
	.type _start, @function

	/*
	 * The BIOS loads this code to address 0x00007c00.
	 * The code should be called with CS:IP 0:0x7c00 (hopefully).
	 */
	.globl _start
_start:
	cli		/* we're not safe here! */
	/*
	 * It seems there are implementations in the wild which call this
	 * code with CS:IP 0x07C0:0000 instead. We fix it immediately.
	 */
	ljmp $0, $real_start

	.size _start, .-_start

