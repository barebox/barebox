/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Startup Code for generic MIPS device tree 2nd stage image
 *
 * Copyright (C) 2024 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <linux/sizes.h>

ENTRY_FUNCTION(start_dt_2nd)

	/* save device tree address in v1 */
	move	v1, a1

	mips_cpu_setup

	copy_to_link_location	start_dt_2nd

	stack_setup

	/* pbl_main_entry() computes fdt_len by itself
	 * if fdt == fdt_end */
	move	a0, v1 /* fdt */
	move	a1, v1 /* fdt_end */
	PTR_LI	a2, SZ_256M /* ram_size */
	PTR_LA	v0, pbl_main_entry
	jal	v0
	 nop

	/* No return */
1:
	b	1b
	 nop
