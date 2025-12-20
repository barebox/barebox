// SPDX-License-Identifier: GPL-2.0-only

#include <mach/imx/debug_ll.h>
#include <common.h>
#include <io.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/imx6.h>

extern char __dtb_imx6sl_kindle_wp63gw_start[];
extern char __dtb_imx6sl_kindle_dp75sdi_start[];
extern char __dtb_imx6sl_kindle_nm460gz_start[];

ENTRY_FUNCTION(start_imx6sl_kindle_wp63gw, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x4, 0x020e016c);
		imx6_uart_setup_ll();
	}

	fdt = __dtb_imx6sl_kindle_wp63gw_start + get_runtime_offset();
	imx6ul_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6sl_kindle6_dp75sdi, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x4, 0x020e016c);
		imx6_uart_setup_ll();
	}

	fdt = __dtb_imx6sl_kindle_dp75sdi_start + get_runtime_offset();
	imx6ul_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6sl_kindle7_dp75sdi, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x4, 0x020e016c);
		imx6_uart_setup_ll();
	}

	fdt = __dtb_imx6sl_kindle_dp75sdi_start + get_runtime_offset();
	imx6ul_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6sl_kindle_nm460gz, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	arm_setup_stack(0x00920000);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		writel(0x4, 0x020e016c);
		imx6_uart_setup_ll();
	}

	fdt = __dtb_imx6sl_kindle_nm460gz_start + get_runtime_offset();
	imx6ul_barebox_entry(fdt);
}
