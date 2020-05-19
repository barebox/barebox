// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Antony Pavlov <antonynpavlov@gmail.com>

/* This file is part of barebox. */

#define __LOWLEVEL_INIT__

#include <common.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <init.h>
#include <linux/sizes.h>

#define VIRT2REAL_SRAM_BASE 0x82000000
#define VIRT2REAL_SRAM_SIZE SZ_16M

extern char __dtb_virt2real_start[];

void __naked __bare_init barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_virt2real_start + get_runtime_offset();

	barebox_arm_entry(VIRT2REAL_SRAM_BASE, VIRT2REAL_SRAM_SIZE, fdt);
}
