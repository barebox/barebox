/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: Juergen Beisert, Pengutronix */

/* This code was inspired by the GRUB2 project. */

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

