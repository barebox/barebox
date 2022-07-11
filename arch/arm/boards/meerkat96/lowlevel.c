// SPDX-License-Identifier: GPL-2.0-only

#include <debug_ll.h>
#include <io.h>
#include <linux/sizes.h>
#include <mach/debug_ll.h>
#include <mach/iomux-mx7.h>
#include <mach/imx7-ccm-regs.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/cache.h>
extern char __dtb_z_imx7d_meerkat96_start[];

static void setup_uart(void)
{
	imx7_early_setup_uart_clock();
	imx7_setup_pad(MX7D_PAD_SD1_WP__UART6_DCE_TX);
	imx7_uart_setup_ll();
	putc_ll('>');
}

ENTRY_FUNCTION_WITHSTACK(start_imx7d_meerkat96, 0, r0, r1, r2)
{
	void *fdt;

	imx7_cpu_lowlevel_init();

	setup_uart();

	fdt = __dtb_z_imx7d_meerkat96_start + get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_512M, fdt);
}
