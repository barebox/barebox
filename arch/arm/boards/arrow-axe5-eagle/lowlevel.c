// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <io.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <pbl.h>
#include <mach/socfpga/debug_ll.h>
#include <mach/socfpga/init.h>
#include <mach/socfpga/generic.h>
#include <mach/socfpga/mailbox_s10.h>
#include <mach/socfpga/soc64-firewall.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-sdram.h>
#include <mach/socfpga/soc64-system-manager.h>

extern char __dtb_z_socfpga_agilex5_axe5_eagle_start[];

#define AXE5_STACKTOP	(SZ_512K)

static noinline void axe5_eagle_continue(void)
{
	void *fdt;

	agilex5_clk_init();

	socfpga_uart_setup_ll();
	pbl_set_putc(socfpga_uart_putc, (void *) SOCFPGA_UART0_ADDRESS);

	pr_debug("Lowlevel init done\n");

	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x224);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x228);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x23c);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x234);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x248);
	writel(0x14, SOCFPGA_PINMUX_ADDRESS + 0x24c);

	writel(0x410, 0x10c03304);
	writel(0x410, 0x10c03300);
	/*
	 * reset the phy via GPIO10. We currently haven't got enough space
	 * to enable the gpio driver in barebox.
	 */
	writel(0x000, 0x10c03300);
	/* FIXME:  can this be decreased? */
	mdelay(1000);
	writel(0x410, 0x10c03300);

	if (current_el() == 3) {
		agilex5_initialize_security_policies();
		pr_debug("Security policies initialized\n");

		/*
		 * need to set the bank select enable before the
		 * agilex5_ddr_init_full() otherwise the serial doesn't show
		 * anything.
		 */
		if (!IS_ENABLED(CONFIG_DEBUG_LL))
			writel(LCR_BKSE, SOCFPGA_UART0_ADDRESS + LCR);
		agilex5_ddr_init_full();

		socfpga_mailbox_s10_init();
		socfpga_mailbox_s10_qspi_open();

		agilex5_load_and_start_image_via_tfa(SZ_1G);
	}

	fdt = __dtb_z_socfpga_agilex5_axe5_eagle_start;

	barebox_arm_entry(SOCFPGA_AGILEX5_DDR_BASE + SZ_1M, SZ_1G - SZ_1M, fdt);
}

ENTRY_FUNCTION_WITHSTACK(start_socfpga_agilex5_axe5_eagle, AXE5_STACKTOP, r0, r1, r2)
{
	if (current_el() == 3)
		socfpga_agilex5_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	axe5_eagle_continue();
}
