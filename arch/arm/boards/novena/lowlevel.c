// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 John Watts <contact@jookia.org>

#include <asm/barebox-arm.h>
#include <common.h>
#include <debug_ll.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <mach/iomux-mx6.h>
#include <mach/xload.h>
#include <soc/fsl/fsl_udc.h>

#define STACK_TOP (MX6_OCRAM_BASE_ADDR + MX6_OCRAM_MAX_SIZE)

extern char __dtb_z_imx6q_novena_start[];

static bool running_from_ram(void)
{
	return (get_pc() >= MX6_MMDC_PORT01_BASE_ADDR);
}

static void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *uart2base = IOMEM(MX6_UART2_BASE_ADDR);

	/* NOTE: RX is needed for TX to work on this board */
	imx_setup_pad(iomuxbase, MX6Q_PAD_EIM_D26__UART2_RXD);
	imx_setup_pad(iomuxbase, MX6Q_PAD_EIM_D27__UART2_TXD);

	imx6_uart_setup(uart2base);
	pbl_set_putc(imx_uart_putc, uart2base);

	pr_debug(">");
}

static void load_barebox(void)
{
	enum bootsource bootsrc;
	int bootinstance;

	imx6_get_boot_source(&bootsrc, &bootinstance);

	if (bootsrc == BOOTSOURCE_SERIAL)
		imx6_barebox_start_usb(IOMEM(MX6_MMDC_PORT01_BASE_ADDR));
	else if (bootsrc == BOOTSOURCE_MMC)
		imx6_esdhc_start_image(bootinstance);

	pr_err("Unsupported boot source %i instance %i\n",
	        bootsrc, bootinstance);
	hang();
}

ENTRY_FUNCTION_WITHSTACK(start_imx6q_novena, STACK_TOP, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();

	imx6_ungate_all_peripherals();
	setup_uart();

	if (!running_from_ram())
		load_barebox();
	else
		imx6q_barebox_entry(__dtb_z_imx6q_novena_start);
}
