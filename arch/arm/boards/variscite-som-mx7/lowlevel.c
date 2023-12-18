// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2022 Roland Hieber, Pengutronix <rhi@pengutronix.de>

#include <io.h>
#include <common.h>
#include <console.h>
#include <debug_ll.h>

#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>

#include <linux/sizes.h>

#include <mach/imx/esdctl.h>
#include <mach/imx/generic.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/iomux-mx7.h>
#include <mach/imx/imx7-ccm-regs.h>

static inline void setup_uart(void)
{
	imx7_early_setup_uart_clock(1);

	imx7_setup_pad(MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX);

	imx7_uart_setup_ll();

	putc_ll('>');
}

ENTRY_FUNCTION_WITHSTACK(start_gome_e143_01, 0, r0, r1, r2)
{
	extern char __dtb_imx7d_gome_e143_01_start[];

	imx7_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	relocate_to_current_adr();
	setup_c();

	imx7d_barebox_entry(__dtb_imx7d_gome_e143_01_start + get_runtime_offset());
}
