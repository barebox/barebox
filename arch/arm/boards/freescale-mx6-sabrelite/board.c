// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2012 Steffen Trumtrar, Pengutronix

/*
 * based on arch/arm/boards/freescale-mx6-arm2/board.c
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx/imx6-regs.h>
#include <gpio.h>
#include <mach/imx/bbu.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <of.h>
#include <deep-probe.h>
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

static iomux_v3_cfg_t sabrelite_enet_gpio_pads[] = {
	/* Ethernet */
	MX6Q_PAD_EIM_D23__GPIO_3_23,		/* RGMII_nRST */
	MX6Q_PAD_RGMII_RXC__GPIO_6_30,		/* PHYAD */
	MX6Q_PAD_RGMII_RD0__GPIO_6_25,		/* MODE0 */
	MX6Q_PAD_RGMII_RD1__GPIO_6_27,		/* MODE1 */
	MX6Q_PAD_RGMII_RD2__GPIO_6_28,		/* MODE2 */
	MX6Q_PAD_RGMII_RD3__GPIO_6_29,		/* MODE3 */
	MX6Q_PAD_RGMII_RX_CTL__GPIO_6_24,
};

static int ksz9021rn_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x09, 0x0f00);

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

static struct gpio fec_gpios[] = {
	{
		.gpio = IMX_GPIO_NR(3, 23),
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "phy-rst",
	}, {
		.gpio = IMX_GPIO_NR(6, 30),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "phy-addr2",
	}, {
		.gpio = IMX_GPIO_NR(1, 23),
		.flags = GPIOF_OUT_INIT_LOW,
		.label = "phy-led-mode",
	}, {
		/* MODE strap-in pins: advertise all capabilities */
		.gpio = IMX_GPIO_NR(6, 25),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "phy-adv1",
	}, {
		.gpio = IMX_GPIO_NR(6, 27),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "phy-adv1",
	}, {
		.gpio = IMX_GPIO_NR(6, 28),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "phy-adv1",
	}, {
		.gpio = IMX_GPIO_NR(6, 29),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "phy-adv1",
	}, {
		/* Enable 125 MHz clock output */
		.gpio = IMX_GPIO_NR(6, 24),
		.flags = GPIOF_OUT_INIT_HIGH,
		.label = "phy-125MHz",
	},
};

static int sabrelite_ksz9021rn_setup(void)
{
	int ret;

	mxc_iomux_v3_setup_multiple_pads(sabrelite_enet_gpio_pads,
			ARRAY_SIZE(sabrelite_enet_gpio_pads));

	ret = gpio_request_array(fec_gpios, ARRAY_SIZE(fec_gpios));
	if (ret) {
		pr_err("Failed to request fec gpios: %s\n", strerror(-ret));
		return ret;
	}

	mdelay(10);

	/* FEC driver picks up the reset gpio later and releases the phy reset */
	gpio_free_array(fec_gpios, ARRAY_SIZE(fec_gpios));

	return 0;
}

static void sabrelite_ehci_init(void)
{
	/* hub reset */
	gpio_direction_output(IMX_GPIO_NR(7, 12), 0);
	udelay(2000);
	gpio_set_value(IMX_GPIO_NR(7, 12), 1);
}

static int sabrelite_probe(struct device *dev)
{
	int ret;

	phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);

	barebox_set_hostname("sabrelite");

	ret = of_devices_ensure_probed_by_property("gpio-controller");
	if (ret)
		return ret;

	ret = sabrelite_ksz9021rn_setup();
	if (ret)
		return ret;

	sabrelite_ehci_init();

	armlinux_set_architecture(3769);

	imx6_bbu_internal_spi_i2c_register_handler("spiflash",
			"/dev/spinor0.barebox", BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
static const struct of_device_id sabrelite_match[] = {
	{
		.compatible = "fsl,imx6q-sabrelite",
	}, {
		.compatible = "fsl,imx6dl-sabrelite",
	},
	{ /* Sentinel */ },
};

static struct driver sabrelite_driver = {
	.name = "physom-imx6",
	.probe = sabrelite_probe,
	.of_compatible = sabrelite_match,
};

postcore_platform_driver(sabrelite_driver);

BAREBOX_DEEP_PROBE_ENABLE(sabrelite_match);
