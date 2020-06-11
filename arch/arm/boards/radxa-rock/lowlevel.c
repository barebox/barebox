// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Beniamino Galvani <b.galvani@gmail.com>

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_rk3188_radxarock_start[];

ENTRY_FUNCTION(start_radxa_rock, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_rk3188_radxarock_start + get_runtime_offset();

	barebox_arm_entry(0x60000000, SZ_2G, fdt);
}
