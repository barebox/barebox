// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2019 Rouven Czerwinski, Pengutronix
 */

#include <common.h>
#include <debug_ll.h>
#include <firmware.h>
#include <mach/generic.h>
#include <asm/barebox-arm.h>
#include <mach/esdctl.h>
#include <mach/iomux-mx6ul.h>
#include <asm/cache.h>
#include <tee/optee.h>

#include "ccbv2.h"

extern char __dtb_z_imx6ul_webasto_ccbv2_start[];

static void configure_uart(void)
{
	void __iomem *iomuxbase = (void *)MX6_IOMUXC_BASE_ADDR;

	imx6_ungate_all_peripherals();

	imx_setup_pad(iomuxbase, MX6_PAD_LCD_DATA16__UART7_DCE_TX);
	imx_setup_pad(iomuxbase, MX6_PAD_LCD_DATA17__UART7_DCE_RX);

	imx6_uart_setup((void *)MX6_UART7_BASE_ADDR);

	putc_ll('>');

}

static void noinline start_ccbv2(u32 r0)
{
	int tee_size;
	void *tee;

	/* Enable normal/secure r/w for TZC380 region0 */
	writel(0xf0000000, 0x021D0108);

	configure_uart();

	/*
	 * Chainloading barebox will pass a device tree within the RAM in r0,
	 * skip OP-TEE early loading in this case
	 */
	if(IS_ENABLED(CONFIG_FIRMWARE_CCBV2_OPTEE)
	   && !(r0 > MX6_MMDC_P0_BASE_ADDR
	        &&  r0 < MX6_MMDC_P0_BASE_ADDR + SZ_256M)) {
		get_builtin_firmware(ccbv2_optee_bin, &tee, &tee_size);

		memset((void *)OPTEE_OVERLAY_LOCATION, 0, 0x1000);

		start_optee_early(NULL, tee);
	}

	imx6ul_barebox_entry(__dtb_z_imx6ul_webasto_ccbv2_start);
}

ENTRY_FUNCTION(start_imx6ul_ccbv2, r0, r1, r2)
{

	imx6ul_cpu_lowlevel_init();

	arm_setup_stack(0x00910000);

	relocate_to_current_adr();
	setup_c();
	barrier();

	start_ccbv2(r0);
}
