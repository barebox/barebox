/*
 * drivers/net/phy/lxt.c
 *
 * Driver for Intel LXT PHYs
 *
 * base on Andy Fleming's linux lxt.c driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
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
