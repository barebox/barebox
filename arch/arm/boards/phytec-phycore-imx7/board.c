// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017 Sascha Hauer, Pengutronix

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx/bbu.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <mach/imx/generic.h>
#include <linux/sizes.h>
#include <asm/psci.h>
#include <io.h>
#include <mach/imx/imx7-regs.h>
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
