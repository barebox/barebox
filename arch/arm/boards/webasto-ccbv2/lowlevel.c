// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2019 Rouven Czerwinski, Pengutronix
 */

#include <common.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <firmware.h>
#include <mach/imx/generic.h>
#include <asm/barebox-arm.h>
#include <mach/imx/esdctl.h>
#include <mach/imx/iomux-mx6ul.h>
#include <asm/cache.h>
#include <tee/optee.h>
#include <mach/imx/tzasc.h>

#include "ccbv2.h"

static void configure_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	imx6_ungate_all_peripherals();

	imx_setup_pad(iomuxbase, MX6_PAD_LCD_DATA16__UART7_DCE_TX);
	imx_setup_pad(iomuxbase, MX6_PAD_LCD_DATA17__UART7_DCE_RX);

	imx6_uart_setup((void *)MX6_UART7_BASE_ADDR);

	putc_ll('>');

}

static void noinline start_ccbv2(unsigned long mem_size, char *fdt)
{
	int tee_size;
	void *tee;

	configure_uart();

	/*
	 * Skip loading barebox when we are chainloaded. We can detect that by detecting
	 * if we can access the TZASC.
	 */
	if (IS_ENABLED(CONFIG_FIRMWARE_TQMA6UL_OPTEE) && imx6_can_access_tzasc()) {
		get_builtin_firmware(ccbv2_optee_bin, &tee, &tee_size);

		imx6ul_start_optee_early(NULL, tee, (void *)OPTEE_OVERLAY_LOCATION, 0x1000);
	}

	imx6ul_barebox_entry(fdt);
}

extern char __dtb_z_imx6ul_webasto_ccbv2_start[];
ENTRY_FUNCTION(start_imx6ul_ccbv2_256m, r0, r1, r2)
{

	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000);

	relocate_to_current_adr();
	setup_c();
	barrier();

	start_ccbv2(SZ_256M, __dtb_z_imx6ul_webasto_ccbv2_start);
}

ENTRY_FUNCTION(start_imx6ul_ccbv2_512m, r0, r1, r2)
{
	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000);

	relocate_to_current_adr();
	setup_c();
	barrier();

	start_ccbv2(SZ_512M, __dtb_z_imx6ul_webasto_ccbv2_start);
}

extern char __dtb_z_imx6ul_webasto_marvel_start[];
ENTRY_FUNCTION(start_imx6ul_marvel, r0, r1, r2)
{
	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000);

	relocate_to_current_adr();
	setup_c();
	barrier();

	start_ccbv2(SZ_512M, __dtb_z_imx6ul_webasto_marvel_start);
}
