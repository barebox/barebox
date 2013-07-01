/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <mach/ar2312_regs.h>

#include <mach/debug_ll.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	pbl_ar2312_pll

	pbl_ar2312_rst_uart0
	debug_ll_ns16550_init

	debug_ll_ns16550_outc 'a'
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
	debug_ll_ns16550_outc '#'
	debug_ll_ns16550_outnl

1:
	b	1b /* dead end */
	 nop

sdram_configured:
	debug_ll_ns16550_outc 'b'
	debug_ll_ns16550_outnl

	copy_to_link_location	pbl_start

	.set	pop
	.endm
