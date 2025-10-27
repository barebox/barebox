// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/generic.h>
#include <mach/imx/xload.h>
#include <asm/barebox-arm.h>
#include <soc/imx9/ddr.h>
#include <mach/imx/atf.h>
#include <mach/imx/xload.h>
#include <mach/imx/esdctl.h>

extern char __dtb_z_imx93_frdm_start[];
extern struct dram_timing_info frdm_imx93_dram_timing;

static noinline void frdm_imx93_continue(void)
{
	void __iomem *base = IOMEM(MX9_UART1_BASE_ADDR);
	void __iomem *muxbase = IOMEM(MX9_IOMUXC_BASE_ADDR);

	/* configure LPUART1 TX pin */
	writel(0x0, muxbase + 0x184);

	imx9_uart_setup(base);
	pbl_set_putc(lpuart32_putc, base + 0x10);
	putchar('>');

	if (current_el() == 3) {
		imx93_ddr_init(&frdm_imx93_dram_timing, DRAM_TYPE_LPDDR4);
		imx93_load_and_start_image_via_tfa();
	}

	imx93_barebox_entry(__dtb_z_imx93_frdm_start);
}

ENTRY_FUNCTION(start_imx93_nxp_mx93_frdm, r0, r1, r2)
{
	if (current_el() == 3)
		imx93_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	frdm_imx93_continue();
}
