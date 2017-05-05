/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2011, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/pbl_macros.h>

	.set noreorder
	.section ".text_entry"
	.align 4

EXPORT(_start)

	mips_barebox_10h

	/* disable watchpoints */
	mtc0	zero, CP0_WATCHLO
	mtc0	zero, CP0_WATCHHI

	mips_disable_interrupts

	copy_to_link_location _start

	stack_setup

	la	v0, main_entry
	jal	v0
	 nop

	/* No return */
__error:
	b __error
	 nop
