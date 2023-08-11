// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 * Copyright (C) 2014 Eric Bénard <eric@eukrea.com>
 * Copyright (C) 2019 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <envfs.h>
#include <mach/imx/bbu.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <deep-probe.h>

static int ar8035_phy_fixup(struct phy_device *dev)
{
	u16 val;

	/* Ar803x phy SmartEEE feature cause link status generates glitch,
	 * which cause ethernet link down/up issue, so disable SmartEEE
	 */
	val = phy_read_mmd(dev, MDIO_MMD_PCS, 0x805d);
	phy_write(dev, MII_MMD_DATA, val & ~(1 << 8));

	val = phy_read_mmd(dev, MDIO_MMD_PCS, 0x4003);
	phy_write(dev, MII_MMD_DATA, val & ~(1 << 8));

	val = phy_read_mmd(dev, MDIO_MMD_PCS, 0x4007);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(dev, MII_MMD_DATA, val);

	return 0;
}

static int marsboard_device_init(struct device *dev)
{
	barebox_set_hostname("marsboard");

	phy_register_fixup_for_uid(0x004dd072, 0xffffffef, ar8035_phy_fixup);

	imx6_bbu_internal_spi_i2c_register_handler("spiflash",
		"/dev/m25p0.barebox", BBU_HANDLER_FLAG_DEFAULT);

	defaultenv_append_directory(defaultenv_mars);

	return 0;
}

static const struct of_device_id marsboard_of_match[] = {
	{ .compatible = "embest,imx6q-marsboard" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(marsboard_of_match);

static struct driver marsboard_driver = {
	.name = "board-mars",
	.probe = marsboard_device_init,
	.of_compatible = marsboard_of_match,
};
postcore_platform_driver(marsboard_driver);
