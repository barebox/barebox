/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_black_swift

#include <mach/debug_ll.h>
#include <asm/pbl_macros.h>
#include <asm/asm.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_barebox_10h

	pbl_ar9331_wmac_enable

	hornet_mips24k_cp0_setup

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	hornet_1_1_war

	pbl_ar9331_pll
	pbl_ar9331_ddr2_config

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

skip_pll_ram_config:
	pbl_ar9331_uart_enable
	debug_ll_ath79_init

	/*
	 * It is amazing but we have to enable MDIO on GPIO
	 * to use GPIO27 for LED1.
	 */
	pbl_ar9331_mdio_gpio_enable

ENTRY_FUNCTION_END(BOARD_PBL_START, black_swift)
