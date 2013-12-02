/*
 * drivers/net/phy/at803x.c
 *
 * Driver for Atheros 803x PHY
 *
 * Author: Matus Ujhelyi <ujhelyi.m@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/string.h>

#define AT803X_INTR_ENABLE			0x12
#define AT803X_INTR_STATUS			0x13
#define AT803X_WOL_ENABLE			0x01
#define AT803X_DEVICE_ADDR			0x03
#define AT803X_LOC_MAC_ADDR_0_15_OFFSET		0x804C
#define AT803X_LOC_MAC_ADDR_16_31_OFFSET	0x804B
#define AT803X_LOC_MAC_ADDR_32_47_OFFSET	0x804A
#define AT803X_MMD_ACCESS_CONTROL		0x0D
#define AT803X_MMD_ACCESS_CONTROL_DATA		0x0E
#define AT803X_FUNC_DATA			0x4003
#define AT803X_DEBUG_ADDR			0x1D
#define AT803X_DEBUG_DATA			0x1E
#define AT803X_DEBUG_SYSTEM_MODE_CTRL		0x05
#define AT803X_DEBUG_RGMII_TX_CLK_DLY		(1 << 8)

static int at803x_config_init(struct phy_device *phydev)
{
	int val;
	int ret;
	u32 features;

	features = SUPPORTED_TP | SUPPORTED_MII | SUPPORTED_AUI |
		   SUPPORTED_FIBRE | SUPPORTED_BNC;

	val = phy_read(phydev, MII_BMSR);
	if (val < 0)
		return val;

	if (val & BMSR_ANEGCAPABLE)
		features |= SUPPORTED_Autoneg;
	if (val & BMSR_100FULL)
		features |= SUPPORTED_100baseT_Full;
	if (val & BMSR_100HALF)
		features |= SUPPORTED_100baseT_Half;
	if (val & BMSR_10FULL)
		features |= SUPPORTED_10baseT_Full;
	if (val & BMSR_10HALF)
		features |= SUPPORTED_10baseT_Half;

	if (val & BMSR_ESTATEN) {
		val = phy_read(phydev, MII_ESTATUS);
		if (val < 0)
			return val;

		if (val & ESTATUS_1000_TFULL)
			features |= SUPPORTED_1000baseT_Full;
		if (val & ESTATUS_1000_THALF)
			features |= SUPPORTED_1000baseT_Half;
	}

	phydev->supported = features;
	phydev->advertising = features;

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID) {
		ret = phy_write(phydev, AT803X_DEBUG_ADDR,
				AT803X_DEBUG_SYSTEM_MODE_CTRL);
		if (ret)
			return ret;
		ret = phy_write(phydev, AT803X_DEBUG_DATA,
				AT803X_DEBUG_RGMII_TX_CLK_DLY);
		if (ret)
			return ret;
	}

	return 0;
}

static struct phy_driver at803x_driver[] = {
{
	/* ATHEROS 8035 */
	.phy_id		= 0x004dd072,
	.phy_id_mask	= 0xffffffef,
	.drv.name	= "Atheros 8035 ethernet",
	.config_init	= at803x_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
}, {
	/* ATHEROS 8030 */
	.phy_id		= 0x004dd076,
	.phy_id_mask	= 0xffffffef,
	.drv.name	= "Atheros 8030 ethernet",
	.config_init	= at803x_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
}, {
	/* ATHEROS 8031 */
	.phy_id		= 0x004dd074,
	.phy_id_mask	= 0xffffffef,
	.drv.name	= "Atheros 8031 ethernet",
	.config_init	= at803x_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
} };

static int atheros_phy_init(void)
{
	return phy_drivers_register(at803x_driver,
				    ARRAY_SIZE(at803x_driver));
}
fs_initcall(atheros_phy_init);
