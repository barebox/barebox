// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Sascha Hauer, Pengutronix

#include <generated/mach-types.h>
#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <envfs.h>
#include <linux/sizes.h>
#include <init.h>
#include <gpio.h>
#include <of.h>

#include <linux/micrel_phy.h>
#include <mfd/stmpe-i2c.h>

#include <asm/armlinux.h>
#include <asm/io.h>

#include <mach/imx/devices-imx6.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/iomux-mx6.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx6.h>
#include <mach/imx/bbu.h>

#define RQ7_GPIO_ENET_PHYADD2	IMX_GPIO_NR(6, 30)
#define RQ7_GPIO_ENET_MODE0	IMX_GPIO_NR(6, 25)
#define RQ7_GPIO_ENET_MODE1	IMX_GPIO_NR(6, 27)
#define RQ7_GPIO_ENET_MODE2	IMX_GPIO_NR(6, 28)
#define RQ7_GPIO_ENET_MODE3	IMX_GPIO_NR(6, 29)
#define RQ7_GPIO_ENET_EN_CLK125	IMX_GPIO_NR(6, 24)

static iomux_v3_cfg_t tqma6x_pads_gpio[] = {
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
	phy_write_mmd_indirect(dev, 8, 2, 0x003ff);

	return 0;
}

static int tq_mba6x_enet_init(void)
{
	if (!of_machine_is_compatible("tq,mba6x"))
		return 0;

	mxc_iomux_v3_setup_multiple_pads(tqma6x_pads_gpio, ARRAY_SIZE(tqma6x_pads_gpio));
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
fs_initcall(tq_mba6x_enet_init);

static int tqma6x_init(void)
{
	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0.barebox",
		BBU_HANDLER_FLAG_DEFAULT);
	imx6_bbu_internal_mmcboot_register_handler("emmc", "mmc2", 0);

	device_detect_by_name("mmc2"); // eMMC

	return 0;
}

static int tq_mba6x_env_init(void)
{
	if (!of_machine_is_compatible("tq,mba6x"))
		return 0;

	tqma6x_init();

	default_environment_path_set("/dev/mmc2.boot1");

	barebox_set_hostname("mba6x");

	return 0;
}
late_initcall(tq_mba6x_env_init);
