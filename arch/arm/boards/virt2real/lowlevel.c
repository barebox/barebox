/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

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
