/*
 * Copyright (C) 2016 PHYTEC Messtechnik GmbH,
 * Author: Wadim Egorov <w.egorov@phytec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/rk3288-regs.h>
#include <mach/grf_rk3288.h>
#include <mach/hardware.h>
#include <debug_ll.h>

extern char __dtb_rk3288_phycore_som_start[];

ENTRY_FUNCTION(start_rk3288_phycore_som, r0, r1, r2)
{
	void *fdt;
	arm_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		struct rk3288_grf * const grf = (void *)RK3288_GRF_BASE;
		rk_clrsetreg(&grf->gpio4c_iomux,
			GPIO4C1_MASK << GPIO4C1_SHIFT |
			GPIO4C0_MASK << GPIO4C0_SHIFT,
			GPIO4C1_UART0BT_SOUT << GPIO4C1_SHIFT |
			GPIO4C0_UART0BT_SIN << GPIO4C0_SHIFT);
		INIT_LL();
	}

	fdt = __dtb_rk3288_phycore_som_start - get_runtime_offset();

	barebox_arm_entry(0x0, SZ_1G, fdt);
}
