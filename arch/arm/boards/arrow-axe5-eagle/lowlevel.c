// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <io.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/system.h>
#include <pbl.h>
#include <mach/socfpga/barebox-arm.h>
#include <mach/socfpga/debug_ll.h>
#include <mach/socfpga/init.h>
#include <mach/socfpga/generic.h>
#include <mach/socfpga/soc64-regs.h>

extern char __dtb_z_socfpga_agilex5_axe5_eagle_start[];

static noinline void axe5_eagle_continue(void)
{
	agilex5_clk_init();

	socfpga_uart_setup_ll();
	pbl_set_putc(socfpga_uart_putc, (void *) SOCFPGA_UART0_ADDRESS);

	pr_debug("Lowlevel init done\n");

	agilex5_barebox_entry(__dtb_z_socfpga_agilex5_axe5_eagle_start);
}

ENTRY_FUNCTION_AGILEX5(start_socfpga_agilex5_axe5_eagle)
{
	if (current_el() == 3)
		socfpga_agilex5_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();

	axe5_eagle_continue();
}
