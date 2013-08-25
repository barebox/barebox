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

static inline void setup_uart(void)
{
	/* Enable UART for lowlevel debugging purposes */
	writel(0x00000000, 0x021e8080);
	writel(0x00004027, 0x021e8084);
	writel(0x00000704, 0x021e8088);
	writel(0x00000a81, 0x021e8090);
	writel(0x0000002b, 0x021e809c);
	writel(0x00013880, 0x021e80b0);
	writel(0x0000047f, 0x021e80a4);
	writel(0x0000c34f, 0x021e80a8);
	writel(0x00000001, 0x021e8080);
}

extern char __dtb_imx6q_mba6x_start[];
extern char __dtb_imx6dl_mba6x_start[];

ENTRY_FUNCTION(start_imx6q_mba6x)(void)
{
	uint32_t fdt;

	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x2, 0x020e0338);
		setup_uart();
		putc_ll('a');
	}

	arm_early_mmu_cache_invalidate();

	fdt = (uint32_t)__dtb_imx6q_mba6x_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

ENTRY_FUNCTION(start_imx6dl_mba6x)(void)
{
	uint32_t fdt;

	__barebox_arm_head();

	arm_cpu_lowlevel_init();

	arm_setup_stack(0x00920000 - 8);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x2, 0x020e035c);
		setup_uart();
		putc_ll('a');
	}

	arm_early_mmu_cache_invalidate();

	fdt = (uint32_t)__dtb_imx6dl_mba6x_start - get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_512M, fdt);
}
