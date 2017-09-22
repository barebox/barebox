/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx6-regs.h>
#include <io.h>
#include <debug_ll.h>
#include <mach/esdctl.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <image-metadata.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *uart = IOMEM(MX6_UART1_BASE_ADDR);

	imx6_ungate_all_peripherals();

	writel(0x0, iomuxbase + 0x84);
	writel(0x1b0b1, iomuxbase + 0x0310);

	writel(0x0, iomuxbase + 0x88);
	writel(0x1b0b0, iomuxbase + 0x0314);

	writel(0x3, iomuxbase + 0x624);

	imx6_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

extern char __dtb_imx6ull_14x14_evk_start[];

static noinline void nxp_imx6_ull(void)
{
	imx6ul_barebox_entry(__dtb_imx6ull_14x14_evk_start);
}

ENTRY_FUNCTION(start_nxp_imx6ull_evk, r0, r1, r2)
{

	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000 - 8);

	arm_early_mmu_cache_invalidate();

	relocate_to_current_adr();
	setup_c();

	setup_uart();

	/* disable all watchdog powerdown counters */
	writew(0x0, IOMEM(MX6_WDOG1_BASE_ADDR + 0x8));
	writew(0x0, IOMEM(MX6_WDOG2_BASE_ADDR + 0x8));
	writew(0x0, IOMEM(MX6ULL_WDOG3_BASE_ADDR + 0x8));

	nxp_imx6_ull();
}
