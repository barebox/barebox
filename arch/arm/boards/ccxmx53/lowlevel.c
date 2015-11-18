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

#include <common.h>
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

static void __noreturn start_imx53_ccxmx53_common(uint32_t size,
						void *fdt_blob_fixed_offset)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(0xf8020000 - 8);

	fdt = fdt_blob_fixed_offset - get_runtime_offset();
	barebox_arm_entry(0x70000000, size, fdt);
}

#define CCMX53_ENTRY(name, fdt_name, memory_size)	\
	ENTRY_FUNCTION(name, r0, r1, r2)				\
	{								\
		extern char __dtb_##fdt_name##_start[];			\
									\
		IMD_USED(ccxmx53_memsize_##memory_size);		\
									\
		start_imx53_ccxmx53_common(memory_size,  \
					 __dtb_##fdt_name##_start);	\
	}

CCMX53_ENTRY(start_ccxmx53_512mb, imx53_ccxmx53, SZ_512M);
CCMX53_ENTRY(start_ccxmx53_1gib, imx53_ccxmx53, SZ_1G);
