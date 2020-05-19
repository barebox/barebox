// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2016 PHYTEC Messtechnik GmbH

/* Author: Wadim Egorov <w.egorov@phytec.de> */

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
		rk_clrsetreg(&grf->gpio7ch_iomux,
			     GPIO7C7_MASK << GPIO7C7_SHIFT |
			     GPIO7C6_MASK << GPIO7C6_SHIFT,
			     GPIO7C7_UART2DBG_SOUT << GPIO7C7_SHIFT |
			     GPIO7C6_UART2DBG_SIN << GPIO7C6_SHIFT);
		INIT_LL();
	}
	fdt = __dtb_rk3288_phycore_som_start + get_runtime_offset();

	barebox_arm_entry(0x0, SZ_1G, fdt);
}
