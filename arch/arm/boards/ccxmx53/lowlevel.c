// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sascha Hauer <s.hauer@pengutronix.de>

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
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE);

	IMD_USED(ccxmx53_memsize_SZ_512M);

	fdt = __dtb_imx53_ccxmx53_start + get_runtime_offset();

	imx53_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_ccxmx53_1gib, r0, r1, r2)
{
	void *fdt;

	imx5_cpu_lowlevel_init();
	arm_setup_stack(MX53_IRAM_BASE_ADDR + MX53_IRAM_SIZE);

	IMD_USED(ccxmx53_memsize_SZ_1G);

	fdt = __dtb_imx53_ccxmx53_start + get_runtime_offset();

	imx53_barebox_entry(fdt);
}
