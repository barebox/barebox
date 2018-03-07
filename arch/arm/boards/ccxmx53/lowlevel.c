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
#include <linux/sizes.h>
#include <io.h>
#include <mach/imx53-regs.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <image-metadata.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>

BAREBOX_IMD_TAG_STRING(ccxmx53_memsize_SZ_512M, IMD_TYPE_PARAMETER, "memsize=512", 0);
BAREBOX_IMD_TAG_STRING(ccxmx53_memsize_SZ_1G, IMD_TYPE_PARAMETER, "memsize=1024", 0);

extern char __dtb_imx53_ccxmx53_start[];

ENTRY_FUNCTION(start_ccxmx53_512mb, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	IMD_USED(ccxmx53_memsize_SZ_512M);

	fdt = __dtb_imx53_ccxmx53_start + get_runtime_offset();

	imx53_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_ccxmx53_1gib, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE - 8);

	IMD_USED(ccxmx53_memsize_SZ_1G);

	fdt = __dtb_imx53_ccxmx53_start + get_runtime_offset();

	imx53_barebox_entry(fdt);
}
