/*
 * Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <debug_ll.h>
#include <common.h>
#include <sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx6-mmdc.h>
#include <mach/imx6.h>

extern char __dtb_imx6q_phytec_pbab01_start[];

ENTRY_FUNCTION(start_phytec_pbab01_1gib)(void)
{
	uint32_t fdt;

	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	fdt = (uint32_t)__dtb_imx6q_phytec_pbab01_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

ENTRY_FUNCTION(start_phytec_pbab01_2gib)(void)
{
	uint32_t fdt;

	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	fdt = (uint32_t)__dtb_imx6q_phytec_pbab01_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_2G, fdt);
}
