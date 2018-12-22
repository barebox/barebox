/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_8devices_lima

#include <mach/debug_ll.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <mach/pbl_ll_init_qca4531.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_barebox_10h

	debug_ll_ath79_init

	hornet_mips24k_cp0_setup

	/* test if we are in the SRAM */
	pbl_blt 0xbd000000 1f t8
	b skip_flash_test
	nop
1:
	/* test if we are in the flash */
	pbl_blt 0xbf000000 skip_pll_ram_config t8
skip_flash_test:

	pbl_qca4531_ddr2_550_550_init

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable
skip_pll_ram_config:

ENTRY_FUNCTION_END(BOARD_PBL_START, qca4531_8devices_lima)
