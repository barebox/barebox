// SPDX-License-Identifier: GPL-2.0-only

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
#include <mach/imx/iomux-mx6.h>
#include <mach/imx/esdctl.h>

extern char __dtb_imx6s_riotboard_start[];

static noinline void continue_imx6s_riotboard(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	writel(0x4, iomuxbase + 0x016c);

	imx6_ungate_all_peripherals();
	// if uart ist not set-up, then OP-TEE will fail if debugging is enabled.
	imx6_uart_setup(IOMEM(MX6_UART2_BASE_ADDR));

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		pbl_set_putc(imx_uart_putc, IOMEM(MX6_UART2_BASE_ADDR));
		putc_ll('>');
	}

	imx6q_barebox_entry(__dtb_imx6s_riotboard_start);
}

ENTRY_FUNCTION(start_imx6s_riotboard, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	continue_imx6s_riotboard();
}
