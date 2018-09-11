/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
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
 *
 *
 */

#define pr_fmt(fmt) "babbage: " fmt

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx51-regs.h>
#include <gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <of.h>
#include <fcntl.h>
#include <mach/bbu.h>
#include <nand.h>
#include <notifier.h>
#include <spi/spi.h>
#include <io.h>
#include <asm/mmu.h>
#include <mach/imx5.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <mach/revision.h>

#define MX51_CCM_CACRR 0x10

#define USBH1_STP	IMX_GPIO_NR(1, 27)
#define USBH1_PHY_RESET IMX_GPIO_NR(2, 5)
#define USBH1_HUB_RESET	IMX_GPIO_NR(1, 7)

static int imx51_babbage_reset_usbh1(void)
{
	void __iomem *iomuxbase = IOMEM(MX51_IOMUXC_BASE_ADDR);

	if (!of_machine_is_compatible("fsl,imx51-babbage"))
		return 0;

	imx_setup_pad(iomuxbase, MX51_PAD_EIM_D21__GPIO2_5);
	imx_setup_pad(iomuxbase, MX51_PAD_GPIO1_7__GPIO1_7);

	gpio_direction_output(USBH1_PHY_RESET, 0);
	gpio_direction_output(USBH1_HUB_RESET, 0);

	mdelay(10);

	gpio_set_value(USBH1_PHY_RESET, 1);
	gpio_set_value(USBH1_HUB_RESET, 1);

	imx_setup_pad(iomuxbase, MX51_PAD_USBH1_STP__GPIO1_27);
	gpio_direction_output(USBH1_STP, 1);

	mdelay(1);

	imx_setup_pad(iomuxbase, MX51_PAD_USBH1_STP__USBH1_STP);

	gpio_free(USBH1_PHY_RESET);

	return 0;
}
console_initcall(imx51_babbage_reset_usbh1);

static int imx51_babbage_init(void)
{
	if (!of_machine_is_compatible("fsl,imx51-babbage"))
		return 0;

	imx51_babbage_power_init();

	barebox_set_hostname("babbage");

	armlinux_set_architecture(MACH_TYPE_MX51_BABBAGE);

	imx51_bbu_internal_mmc_register_handler("mmc", "/dev/mmc0",
		BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
coredevice_initcall(imx51_babbage_init);
