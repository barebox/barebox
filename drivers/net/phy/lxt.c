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

static int lxt97x_phy_init(void)
{
	return phy_drivers_register(lxt97x_driver,
				    ARRAY_SIZE(lxt97x_driver));
}
fs_initcall(lxt97x_phy_init);
