/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
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

#include <mach/debug_ll_ar9344.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <mach/pbl_ll_init_qca4531.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	debug_ll_ar9344_init

	debug_ll_outc '1'

	hornet_mips24k_cp0_setup
	debug_ll_outc '2'

	/* test if we are in the SRAM */
	pbl_blt 0xbd000000 1f t8
	debug_ll_outc '3'
	b skip_flash_test
	nop
1:
	/* test if we are in the flash */
	pbl_blt 0xbf000000 skip_pll_ram_config t8
	debug_ll_outc '4'
skip_flash_test:

	pbl_qca4531_ddr2_550_550_init

	debug_ll_outc '5'
	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable
skip_pll_ram_config:
	debug_ll_outc '6'
	debug_ll_outnl

	mips_nmon

	copy_to_link_location	pbl_start

	.set	pop
	.endm
