/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2020 Du Huanpeng <u74147@gmail.com>
 */

#define BOARD_PBL_START start_loongson_ls1b

#include <asm/pbl_macros.h>
#include <mach/loongson1.h>
#include <mach/debug_ll.h>
#include <asm/pbl_nmon.h>
#include <linux/sizes.h>

#include <mach/pbl_macros.h>
#include <mach/pbl_ll_init_loongson1.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_barebox_10h

	mips_disable_interrupts

	pbl_blt 0xbf000000 skip_pll_ram_config t0

	pbl_loongson1_pll

	pbl_loongson1_uart_enable
	debug_ll_ns16550_init

	debug_ll_outc '.'
	debug_ll_ns16550_outnl
	debug_ll_outc '1'

	pbl_loongson1_remap
	debug_ll_outc '2'

	pbl_loongson1_ddr2_init
	pbl_probe_mem t0, t1, KSEG1
	bne     t0, t1, .
	nop

	debug_ll_outc '3'

	mips_cache_reset
	dcache_enable
	debug_ll_outc '4'

skip_pll_ram_config:
	debug_ll_outc '5'
	debug_ll_ns16550_outnl

ENTRY_FUNCTION_END(BOARD_PBL_START, loongson_ls1b, SZ_64M)
