// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/net/phy/lxt.c
 *
 * Driver for Intel LXT PHYs
 *
 * base on Andy Fleming's linux lxt.c driver
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>

static struct phy_driver lxt97x_driver[] = {
{
	.phy_id		= 0x001378e0,
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "LXT971",
	.features	= PHY_BASIC_FEATURES,
} };

device_phy_drivers(lxt97x_driver);
