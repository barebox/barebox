/*
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation.
 *
 */

#include <generated/mach-types.h>
#include <environment.h>
#include <bootsource.h>
#include <partition.h>
#include <common.h>
#include <envfs.h>
#include <sizes.h>
#include <init.h>
#include <gpio.h>
#include <fec.h>

#include <linux/micrel_phy.h>
#include <mfd/stmpe-i2c.h>

#include <asm/armlinux.h>
#include <asm/io.h>

#include <mach/devices-imx6.h>
#include <mach/imx6-regs.h>
#include <mach/iomux-mx6.h>
#include <mach/imx6-mmdc.h>
#include <mach/generic.h>
#include <mach/imx6.h>
#include <mach/bbu.h>

#define RQ7_GPIO_ENET_PHYADD2	IMX_GPIO_NR(6, 30)
#define RQ7_GPIO_ENET_MODE0	IMX_GPIO_NR(6, 25)
#define RQ7_GPIO_ENET_MODE1	IMX_GPIO_NR(6, 27)
#define RQ7_GPIO_ENET_MODE2	IMX_GPIO_NR(6, 28)
#define RQ7_GPIO_ENET_MODE3	IMX_GPIO_NR(6, 29)
#define RQ7_GPIO_ENET_EN_CLK125	IMX_GPIO_NR(6, 24)

static iomux_v3_cfg_t realq7_pads_gpio[] = {
	MX6Q_PAD_RGMII_RXC__GPIO_6_30,
	MX6Q_PAD_RGMII_RD0__GPIO_6_25,
	MX6Q_PAD_RGMII_RD1__GPIO_6_27,
	MX6Q_PAD_RGMII_RD2__GPIO_6_28,
	MX6Q_PAD_RGMII_RD3__GPIO_6_29,
	MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24,
};

static int ksz9031rn_phy_fixup(struct phy_device *dev)
{
	/*
	 * min rx data delay, max rx/tx clock delay,
	 * min rx/tx control delay
	 */
	phy_write_mmd_indirect(dev, 4, 2, 0);
	phy_write_mmd_indirect(dev, 5, 2, 0);
	phy_write_mmd_indirect(dev, 8, 2, 0x03ff);

	return 0;
}

static int realq7_enet_init(void)
{
	if (!of_machine_is_compatible("dmo,imx6q-realq7"))
		return 0;

	mxc_iomux_v3_setup_multiple_pads(realq7_pads_gpio, ARRAY_SIZE(realq7_pads_gpio));
	gpio_direction_output(RQ7_GPIO_ENET_PHYADD2, 0);
	gpio_direction_output(RQ7_GPIO_ENET_MODE0, 1);
	gpio_direction_output(RQ7_GPIO_ENET_MODE1, 1);
	gpio_direction_output(RQ7_GPIO_ENET_MODE2, 1);
	gpio_direction_output(RQ7_GPIO_ENET_MODE3, 1);
	gpio_direction_output(RQ7_GPIO_ENET_EN_CLK125, 1);

	gpio_direction_output(25, 0);
	mdelay(50);

	gpio_direction_output(25, 1);
	mdelay(50);

	phy_register_fixup_for_uid(PHY_ID_KSZ9031, MICREL_PHY_ID_MASK,
					   ksz9031rn_phy_fixup);

	return 0;
}
fs_initcall(realq7_enet_init);

static int realq7_env_init(void)
{
	if (!of_machine_is_compatible("dmo,imx6q-realq7"))
		return 0;

	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0.barebox",
		BBU_HANDLER_FLAG_DEFAULT, NULL, 0, 0x00907000);
	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc3.barebox",
		0, NULL, 0, 0x00907000);
	return 0;
}
late_initcall(realq7_env_init);

static int realq7_console_init(void)
{
	if (!of_machine_is_compatible("dmo,imx6q-realq7"))
		return 0;

	barebox_set_hostname("eDM-QMX6");

	imx6_init_lowlevel();

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		of_device_enable_path("/chosen/environment-emmc");
		break;
	default:
	case BOOTSOURCE_SPI:
		of_device_enable_path("/chosen/environment-spi");
		break;
	}

	return 0;
}
postcore_initcall(realq7_console_init);
