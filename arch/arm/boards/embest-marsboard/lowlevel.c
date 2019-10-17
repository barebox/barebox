// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum - Pengutronix
 */

#include <common.h>
#include <io.h>
#include <asm/barebox-arm.h>
#include <mach/imx6.h>
#include <mach/esdctl.h>
#include <mach/iomux-mx6.h>
#include <debug_ll.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	imx6_ungate_all_peripherals();

	imx_setup_pad(iomuxbase, MX6Q_PAD_EIM_D26__UART2_TXD);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_z_imx6q_marsboard_start[];

ENTRY_FUNCTION(start_imx6q_marsboard, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_z_imx6q_marsboard_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}
