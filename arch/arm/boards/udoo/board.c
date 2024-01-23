// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Raphaël Poggi
// SPDX-FileCopyrightText: 2012 Steffen Trumtrar, Pengutronix

/* based on arch/arm/boards/freescale-mx6-arm2/board.c */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx/imx6-regs.h>
#include <gpio.h>
#include <mach/imx/bbu.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <linux/phy.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/imx/generic.h>
#include <linux/sizes.h>
#include <net.h>
#include <linux/micrel_phy.h>
#include <mach/imx/imx6.h>
#include <mach/imx/devices-imx6.h>
#include <mach/imx/iomux-mx6.h>
#include <spi/spi.h>
#include <mach/imx/spi.h>
#include <mach/imx/usb.h>

static iomux_v3_cfg_t udoo_enet_gpio_pads_1[] = {
	/* RGMII reset */
	MX6Q_PAD_EIM_D23__GPIO_3_23			| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* alimentazione ethernet*/
	MX6Q_PAD_EIM_EB3__GPIO_2_31			| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 32 - 1 - (MODE0) all */
	MX6Q_PAD_RGMII_RD0__GPIO_6_25		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 31 - 1 - (MODE1) all */
	MX6Q_PAD_RGMII_RD1__GPIO_6_27		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 28 - 1 - (MODE2) all */
	MX6Q_PAD_RGMII_RD2__GPIO_6_28		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 27 - 1 - (MODE3) all */
	MX6Q_PAD_RGMII_RD3__GPIO_6_29		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
	MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24	| MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t udoo_enet_gpio_pads_2[] = {
	/* Ethernet */
	MX6Q_PAD_RGMII_RXC__GPIO_6_30,          /* PHYAD */
	MX6Q_PAD_RGMII_RD0__GPIO_6_25,          /* MODE0 */
	MX6Q_PAD_RGMII_RD1__GPIO_6_27,          /* MODE1 */
	MX6Q_PAD_RGMII_RD2__GPIO_6_28,          /* MODE2 */
	MX6Q_PAD_RGMII_RD3__GPIO_6_29,          /* MODE3 */
	MX6Q_PAD_RGMII_RX_CTL__ENET_RGMII_RX_CTL,
};

static int ksz9021rn_phy_fixup(struct phy_device *dev)
{

	phy_write(dev, 0x09, 0x1c00);
	phy_write(dev, 0x4, 0x0000);
	phy_write(dev, 0x5, 0x0000);
	phy_write(dev, 0x6, 0x0000);
	phy_write(dev, 0x8, 0x03ff);

	/* do same as linux kernel */
	/* min rx data delay */
	phy_write(dev, 0x0b, 0x8105);
	phy_write(dev, 0x0c, 0x0000);

	/* max rx/tx clock delay, min rx/tx control delay */
	phy_write(dev, 0x0b, 0x8104);
	phy_write(dev, 0x0c, 0xf0f0);
	phy_write(dev, 0x0b, 0x104);

	return 0;
}

static int udoo_ksz9021rn_setup(void)
{
	if (!of_machine_is_compatible("udoo,imx6qdl-udoo"))
		   return 0;

	mxc_iomux_v3_setup_multiple_pads(udoo_enet_gpio_pads_1,
				  ARRAY_SIZE(udoo_enet_gpio_pads_1));

	gpio_direction_output(IMX_GPIO_NR(2, 31) , 1); /* Power on enet */

	/* MODE strap-in pins: advertise all capabilities */
	gpio_direction_output(IMX_GPIO_NR(6, 24), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 25), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 27), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 28), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 29), 1);

	mdelay(100);

	gpio_free(IMX_GPIO_NR(6, 24));
	gpio_free(IMX_GPIO_NR(6, 25));
	gpio_free(IMX_GPIO_NR(6, 27));
	gpio_free(IMX_GPIO_NR(6, 28));
	gpio_free(IMX_GPIO_NR(6, 29));

	mxc_iomux_v3_setup_multiple_pads(udoo_enet_gpio_pads_2, ARRAY_SIZE(udoo_enet_gpio_pads_2));

	return 0;
}
/*
 * Do this before the fec initializes but after our
 * gpios are available.
 */
fs_initcall(udoo_ksz9021rn_setup);

static void udoo_ehci_init(void)
{
	/* hub reset */
	gpio_direction_output(204, 0);
	udelay(2000);
	gpio_set_value(204, 1);
}

static iomux_v3_cfg_t const wdog_pads[] = {
	MX6Q_PAD_EIM_A24__GPIO_5_4 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6Q_PAD_EIM_D19__EPIT1_EPITO,
};

#define WDT_EN      IMX_GPIO_NR(5, 4)
#define WDT_TRG     IMX_GPIO_NR(3, 19)
static void udoo_wdog_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));
	gpio_direction_output(WDT_TRG, 0);
	gpio_direction_output(WDT_EN, 1);
	gpio_direction_input(WDT_TRG);
}

static void udoo_epit_init(void)
{
	writel(0x0000000, MX6_EPIT1_BASE_ADDR);
	writel(0x142000F, MX6_EPIT1_BASE_ADDR);
	writel(0x30000, MX6_EPIT1_BASE_ADDR + 0x8);
	writel(0x0, MX6_EPIT1_BASE_ADDR + 0xC);
}

static int udoo_devices_init(void)
{
	if (!of_machine_is_compatible("udoo,imx6qdl-udoo"))
		return 0;

	udoo_wdog_init();
	udoo_ehci_init();
	udoo_epit_init();

	armlinux_set_bootparams((void *)0x10000100);
	barebox_set_hostname("udoo");

	return 0;
}
device_initcall(udoo_devices_init);

static int udoo_coredevices_init(void)
{
	if (!of_machine_is_compatible("udoo,imx6qdl-udoo"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
			ksz9021rn_phy_fixup);
	return 0;
}
coredevice_initcall(udoo_coredevices_init);
