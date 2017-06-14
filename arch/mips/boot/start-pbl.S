/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2011, 2012 Antony Pavlov <antonynpavlov@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>

#include <asm/pbl_macros.h>
#include <board/board_pbl_start.h>

	.set noreorder
	.section ".text_head_entry"
	.align 4

EXPORT(pbl_start)

	board_pbl_start

	stack_setup

	la	v0, pbl_main_entry
	jal	v0
	 nop

	/* No return */
__error:
	b	__error
	 nop
