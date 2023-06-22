/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2011, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <asm/pbl_macros.h>

	.set noreorder
	.section ".text_entry"
	.align 4

EXPORT(_start)

	mips_barebox_10h

	/* save dtb pointer */
	move	s0, a0
	move	s1, a1
	move	s2, a2

	/* disable watchpoints */
	mtc0	zero, CP0_WATCHLO
	mtc0	zero, CP0_WATCHHI

	mips_disable_interrupts

	mips64_enable_64bit_addressing

	copy_to_link_location _start

	stack_setup

	/* restore dtb pointer */
	move	a0, s0
	move	a1, s1
	move	a2, s2
	PTR_LA	v0, relocate_code
	jal	v0
	 nop

	/* No return */
__error:
	b __error
	 nop
