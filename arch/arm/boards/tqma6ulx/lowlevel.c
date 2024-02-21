// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2019 Rouven Czerwinski, Pengutronix
 */
#define pr_fmt(fmt) "tqma6ul: " fmt

#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <firmware.h>
#include <mach/imx/generic.h>
#include <asm/barebox-arm.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/iomux-mx6ul.h>
#include <asm/cache.h>

extern char __dtb_z_imx6ul_mba6ulx_start[];

static void setup_uart(void)
{
	imx6_ungate_all_peripherals();

	/*
	 * Default pad configuration on this board, no explicit config needed
	 */
	imx6_uart_setup((void *)MX6_UART1_BASE_ADDR);
	pbl_set_putc(imx_uart_putc, (void *)MX6_UART1_BASE_ADDR);

	pr_debug("\n");

}

static void noinline start_mba6ulx(void)
{
	setup_uart();

	imx6ul_barebox_entry(__dtb_z_imx6ul_mba6ulx_start);
}

ENTRY_FUNCTION(start_imx6ul_mba6ulx, r0, r1, r2)
{

	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000);

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		imx6_uart_setup_ll();
		putc_ll('>');
	}

	relocate_to_current_adr();
	setup_c();
	barrier();

	start_mba6ulx();
}
