/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_netgear_wg102

#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <mach/ar2312_regs.h>

#include <mach/debug_ll.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_barebox_10h

	mips_disable_interrupts

	pbl_ar2312_pll

	pbl_ar2312_rst_uart0
	debug_ll_ns16550_init

	debug_ll_outc 'a'
	debug_ll_ns16550_outnl

	/* check if SDRAM is already configured,
	 * if yes, we are probably starting
	 * as second stage loader and can skip configuration */
	la	t0, KSEG1 | AR2312_MEM_CFG1
	lw	t1, 0(t0)
	and	t0, t1, MEM_CFG1_E0
	beq	zero, t0, 1f
	 nop

	pbl_probe_mem t0, t1, KSEG1
	beq t0, t1, sdram_configured
	 nop

1:
	/* start SDRAM configuration */
	pbl_ar2312_x16_sdram

	/* check one more time. if some thing wrong,
	 * we don't need to continue */
	pbl_probe_mem t0, t1, KSEG1
	beq t0, t1, sdram_configured
	 nop
	debug_ll_outc '#'
	debug_ll_ns16550_outnl

1:
	b	1b /* dead end */
	 nop

sdram_configured:
	debug_ll_outc 'b'
	debug_ll_ns16550_outnl

	copy_to_link_location	BOARD_PBL_START;
	stack_setup;

	lw	a0, 0;
	la	v0, pbl_main_entry;
	jal	v0;
	 nop;

	/* No return */
__error:
	b	__error;
	 nop;
