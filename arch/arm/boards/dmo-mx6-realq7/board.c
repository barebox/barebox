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

static void mmd_write_reg(struct phy_device *dev, int device, int reg, int val)
{
	phy_write(dev, 0x0d, device);
	phy_write(dev, 0x0e, reg);
	phy_write(dev, 0x0d, (1 << 14) | device);
	phy_write(dev, 0x0e, val);
}

static int ksz9031rn_phy_fixup(struct phy_device *dev)
{
	/*
	 * min rx data delay, max rx/tx clock delay,
	 * min rx/tx control delay
	 */
	mmd_write_reg(dev, 2, 4, 0);
	mmd_write_reg(dev, 2, 5, 0);
	mmd_write_reg(dev, 2, 8, 0x003ff);

	return 0;
}

static int realq7_enet_init(void)
{
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

static int realq7_devices_init(void)
{
	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0.barebox",
		BBU_HANDLER_FLAG_DEFAULT, NULL, 0, 0x00907000);
	imx6_bbu_internal_mmc_register_handler("mmc", "/dev/mmc3.barebox",
		0, NULL, 0, 0x00907000);

	return 0;
}
device_initcall(realq7_devices_init);

static int realq7_env_init(void)
{
	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		if (!IS_ENABLED(CONFIG_MCI_STARTUP)) {
			struct device_d *dev = get_device_by_name("mmc3");
			if (dev)
				device_detect(dev);
		}
		devfs_add_partition("mmc3", 0, SZ_1M, DEVFS_PARTITION_FIXED, "mmc3.barebox");
		devfs_add_partition("mmc3", SZ_1M, SZ_1M, DEVFS_PARTITION_FIXED, "mmc3.bareboxenv");
		default_environment_path = "/dev/mmc3.bareboxenv";
		break;
	default:
	case BOOTSOURCE_SPI:
		devfs_add_partition("m25p0", 0, SZ_256K, DEVFS_PARTITION_FIXED, "m25p0.barebox");
		devfs_add_partition("m25p0", SZ_256K, SZ_256K, DEVFS_PARTITION_FIXED, "m25p0.bareboxenv");
		default_environment_path = "/dev/m25p0.bareboxenv";
		break;
	}

	return 0;
}
late_initcall(realq7_env_init);

static int realq7_console_init(void)
{
	imx6_init_lowlevel();

	return 0;
}
core_initcall(realq7_console_init);
