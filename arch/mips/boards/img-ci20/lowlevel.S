/*
 * Startup Code for Imagination Creator CI20 board
 *
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
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

#define BOARD_PBL_START start_img_ci20

#include <asm/pbl_macros.h>
#include <mach/debug_ll.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_disable_interrupts

	/* CPU/SoC specific setup ... */
	/* ... absent */

	debug_ll_ns16550_init

	debug_ll_outc '.'
	debug_ll_ns16550_outnl

ENTRY_FUNCTION_END(BOARD_PBL_START, img_ci20)
