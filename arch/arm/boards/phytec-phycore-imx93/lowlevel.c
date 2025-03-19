// SPDX-License-Identifier: GPL-2.0

#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <mach/imx/generic.h>
#include <mach/imx/xload.h>
#include <asm/barebox-arm.h>
#include <soc/imx9/ddr.h>
#include <mach/imx/atf.h>
#include <mach/imx/esdctl.h>

extern char __dtb_z_imx93_phyboard_segin_start[];
extern struct dram_timing_info dram_timing;

static noinline void phycore_imx93_continue(void)
{
	void *base = IOMEM(MX9_UART1_BASE_ADDR);
	void *muxbase = IOMEM(MX9_IOMUXC_BASE_ADDR);
	void *fdt;

	/* uart iomux-setup */
	writel(0x0, muxbase + 0x0184); /* TX */
	writel(0x0, muxbase + 0x0180); /* RX */

	imx9_uart_setup(IOMEM(base));
	pbl_set_putc(lpuart32_putc, base + 0x10);

	if (current_el() == 3) {

		/*
		 * Current u-boot master has two dram variants but neither
		 * where found to work here. As a workaround, lpddr4_timing.c
		 * is based on the initial single config version
		 */

		imx93_ddr_init(&dram_timing, DRAM_TYPE_LPDDR4);
		imx93_load_and_start_image_via_tfa();
	}

	fdt = __dtb_z_imx93_phyboard_segin_start;

	imx93_barebox_entry(fdt);

	__builtin_unreachable();
}

ENTRY_FUNCTION(start_imx93_phyboard_segin, r0, r1, r2)
{
	if (current_el() == 3)
		imx93_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	phycore_imx93_continue();
}
