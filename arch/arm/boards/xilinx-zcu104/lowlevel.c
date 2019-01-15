// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */

#include <common.h>
#include <debug_ll.h>
#include <asm/barebox-arm.h>

extern char __dtb_zynqmp_zcu104_revA_start[];

void zynqmp_zcu104_start(uint32_t, uint32_t, uint32_t);

void noinline zynqmp_zcu104_start(uint32_t r0, uint32_t r1, uint32_t r2)
{
	/* Assume that the first stage boot loader configured the UART */
	putc_ll('>');

	barebox_arm_entry(0, SZ_2G,
			  __dtb_zynqmp_zcu104_revA_start + global_variable_offset());
}
