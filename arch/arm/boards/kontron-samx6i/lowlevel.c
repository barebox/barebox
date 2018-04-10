/*
 * Copyright 2018 (C) Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <debug_ll.h>
#include <common.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx6.h>
#include <mach/esdctl.h>

resource_size_t samx6i_get_size(void);

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	writel(0x4, iomuxbase + 0x016c);

	imx6_ungate_all_peripherals();
	imx6_uart_setup_ll();

	putc_ll('>');
}

static void __noreturn start_imx6_samx6i_common(void *fdt_blob_fixed_offset)
{
	void *fdt;
	resource_size_t size = 0;

	size = samx6i_get_size();

	imx6_cpu_lowlevel_init();
	arm_setup_stack(0x00920000 - 8);

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = fdt_blob_fixed_offset + get_runtime_offset();

	barebox_arm_entry(0x10000000, size, fdt);
}

extern char __dtb_imx6dl_samx6i_start[];
extern char __dtb_imx6q_samx6i_start[];

ENTRY_FUNCTION(start_imx6q_samx6i, r0, r1, r2)
{
	start_imx6_samx6i_common(__dtb_imx6q_samx6i_start);
}

ENTRY_FUNCTION(start_imx6dl_samx6i, r0, r1, r2)
{
	start_imx6_samx6i_common(__dtb_imx6dl_samx6i_start);
}
