// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 * Copyright (C) 2014 Eric Bénard <eric@eukrea.com>
 * Copyright (C) 2019 Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <envfs.h>
#include <mach/bbu.h>
#include <linux/phy.h>

static int ar8035_phy_fixup(struct phy_device *dev)
{
	u16 val;

	/* Ar803x phy SmartEEE feature cause link status generates glitch,
	 * which cause ethernet link down/up issue, so disable SmartEEE
	 */
	phy_write(dev, 0xd, 0x3);
	phy_write(dev, 0xe, 0x805d);
	phy_write(dev, 0xd, 0x4003);

	val = phy_read(dev, 0xe);
	phy_write(dev, 0xe, val & ~(1 << 8));

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(dev, 0xd, 0x7);
	phy_write(dev, 0xe, 0x8016);
	phy_write(dev, 0xd, 0x4007);

	val = phy_read(dev, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(dev, 0xe, val);

	/* introduce tx clock delay */
	phy_write(dev, 0x1d, 0x5);
	val = phy_read(dev, 0x1e);
	val |= 0x0100;
	phy_write(dev, 0x1e, val);

	return 0;
}

static int marsboard_device_init(void)
{
	if (!of_machine_is_compatible("embest,imx6q-marsboard"))
		return 0;

	barebox_set_hostname("marsboard");

	phy_register_fixup_for_uid(0x004dd072, 0xffffffef, ar8035_phy_fixup);

	imx6_bbu_internal_spi_i2c_register_handler("spiflash",
		"/dev/m25p0.barebox", BBU_HANDLER_FLAG_DEFAULT);

	defaultenv_append_directory(defaultenv_mars);

	return 0;
}
device_initcall(marsboard_device_init);
