/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for Ritmix RZX-50 board
 *
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 */

#define BOARD_PBL_START start_ritmix_rzx50

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

ENTRY_FUNCTION_END(BOARD_PBL_START, rzx50)
