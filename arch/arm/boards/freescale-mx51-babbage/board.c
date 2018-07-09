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

#ifdef CONFIG_ARCH_IMX_XLOAD

static int imx51_babbage_xload_init_pinmux(void)
{
	static const iomux_v3_cfg_t pinmux[] = {
		/* (e)CSPI */
		MX51_PAD_CSPI1_MOSI__ECSPI1_MOSI,
		MX51_PAD_CSPI1_MISO__ECSPI1_MISO,
		MX51_PAD_CSPI1_SCLK__ECSPI1_SCLK,

		/* (e)CSPI chip select lines */
		MX51_PAD_CSPI1_SS1__GPIO4_25,


		/* eSDHC 1 */
		MX51_PAD_SD1_CMD__SD1_CMD,
		MX51_PAD_SD1_CLK__SD1_CLK,
		MX51_PAD_SD1_DATA0__SD1_DATA0,
		MX51_PAD_SD1_DATA1__SD1_DATA1,
		MX51_PAD_SD1_DATA2__SD1_DATA2,
		MX51_PAD_SD1_DATA3__SD1_DATA3,
	};

	mxc_iomux_v3_setup_multiple_pads(ARRAY_AND_SIZE(pinmux));

	return 0;
}
coredevice_initcall(imx51_babbage_xload_init_pinmux);

static int imx51_babbage_xload_init_devices(void)
{
	static int spi0_chipselects[] = {
		IMX_GPIO_NR(4, 25),
	};

	static struct spi_imx_master spi0_pdata = {
		.chipselect = spi0_chipselects,
		.num_chipselect = ARRAY_SIZE(spi0_chipselects),
	};

	static const struct spi_board_info spi0_devices[] = {
		{
			.name		= "mtd_dataflash",
			.chip_select	= 0,
			.max_speed_hz	= 25 * 1000 * 1000,
			.bus_num	= 0,
		},
	};

	imx51_add_mmc0(NULL);

	spi_register_board_info(ARRAY_AND_SIZE(spi0_devices));
	imx51_add_spi0(&spi0_pdata);

	return 0;
}
device_initcall(imx51_babbage_xload_init_devices);

#endif
