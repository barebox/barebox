/*
 * Copyright (C) 2013, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
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

#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

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
	debug_ll_ar9331_init
	mips_nmon

	/*
	 * It is amazing but we have to enable MDIO on GPIO
	 * to use GPIO27 for LED1.
	 */
	pbl_ar9331_mdio_gpio_enable

	copy_to_link_location	pbl_start

	.set	pop
	.endm
