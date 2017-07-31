/*
 * Copyright (C) 2017 Sascha Hauer, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/bbu.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <mach/generic.h>
#include <linux/sizes.h>
#include <asm/psci.h>
#include <io.h>
#include <mach/imx7-regs.h>
#include <serial/imx-uart.h>
#include <asm/secure.h>

static int phycore_mx7_devices_init(void)
{
	if (!of_machine_is_compatible("phytec,imx7d-phycore-som"))
		return 0;

	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc2.boot0.barebox",
					       BBU_HANDLER_FLAG_DEFAULT);

	psci_set_putc(imx_uart_putc, IOMEM(MX7_UART5_BASE_ADDR));

	return 0;
}
device_initcall(phycore_mx7_devices_init);
