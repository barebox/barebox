// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sascha Hauer <s.hauer@pengutronix.de>

#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx/imx6.h>

extern char __dtb_imx6q_mba6x_start[];
extern char __dtb_imx6dl_mba6x_start[];

ENTRY_FUNCTION_WITHSTACK(start_imx6q_mba6x, 0x00920000, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x2, 0x020e0338);
		imx6_uart_setup_ll();
		putc_ll('a');
	}

	arm_early_mmu_cache_invalidate();

	fdt = __dtb_imx6q_mba6x_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

ENTRY_FUNCTION_WITHSTACK(start_imx6dl_mba6x, 0x00920000, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x2, 0x020e035c);
		imx6_uart_setup_ll();
		putc_ll('a');
	}

	arm_early_mmu_cache_invalidate();

	fdt = __dtb_imx6dl_mba6x_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_512M, fdt);
}
