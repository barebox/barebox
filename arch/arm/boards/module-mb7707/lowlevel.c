// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Antony Pavlov <antonynpavlov@gmail.com>

/* This file is part of barebox. */

#define __LOWLEVEL_INIT__

#include <common.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <init.h>
#include <linux/sizes.h>

#define MB7707_SRAM_BASE 0x40000000
#define MB7707_SRAM_SIZE SZ_128M

extern char __dtb_module_mb7707_start[];

void __naked __bare_init barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_module_mb7707_start + get_runtime_offset();

	barebox_arm_entry(MB7707_SRAM_BASE, MB7707_SRAM_SIZE, fdt);
}
