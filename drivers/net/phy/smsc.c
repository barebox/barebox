// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/net/phy/smsc.c
 *
 * Driver for SMSC PHYs
 *
 * Author: Herbert Valerio Riedel
 *
 * Copyright (c) 2006 Herbert Valerio Riedel <hvr@gnu.org>
 *
 * Support added for SMSC LAN8187 and LAN8700 by steve.glendinning@shawell.net
 *
 */

#include <common.h>
#include <init.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/smscphy.h>

static int smsc_phy_ack_interrupt(struct phy_device *phydev)
{
	int rc = phy_read (phydev, MII_LAN83C185_ISF);

	return rc < 0 ? rc : 0;
}

static int smsc_phy_config_init(struct phy_device *phydev)
{
	int rc = phy_read(phydev, MII_LAN83C185_CTRL_STATUS);
	if (rc < 0)
		return rc;

	/* Enable energy detect mode for this SMSC Transceivers */
	rc = phy_write(phydev, MII_LAN83C185_CTRL_STATUS,
		       rc | MII_LAN83C185_EDPWRDOWN);
	if (rc < 0)
		return rc;

	return smsc_phy_ack_interrupt (phydev);
}

static int lan87xx_config_init(struct phy_device *phydev)
{
	/*
	 * Make sure the EDPWRDOWN bit is NOT set. Setting this bit on
	 * LAN8710/LAN8720 PHY causes the PHY to misbehave, likely due
	 * to a bug on the chip.
	 *
	 * When the system is powered on with the network cable being
	 * disconnected all the way until after ifconfig ethX up is
	 * issued for the LAN port with this PHY, connecting the cable
	 * afterwards does not cause LINK change detection, while the
	 * expected behavior is the Link UP being detected.
	 */
	int rc = phy_read(phydev, MII_LAN83C185_CTRL_STATUS);
	if (rc < 0)
		return rc;

	rc &= ~MII_LAN83C185_EDPWRDOWN;

	rc = phy_write(phydev, MII_LAN83C185_CTRL_STATUS, rc);
	if (rc < 0)
		return rc;

	return smsc_phy_ack_interrupt(phydev);
}

static int lan911x_config_init(struct phy_device *phydev)
{
	return smsc_phy_ack_interrupt(phydev);
}

static struct phy_driver smsc_phy_driver[] = {
{
	.phy_id		= 0x0007c0a0, /* OUI=0x00800f, Model#=0x0a */
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "SMSC LAN83C185",

	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),

	/* basic functions */
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.config_init	= smsc_phy_config_init,
}, {
	.phy_id		= 0x0007c0b0, /* OUI=0x00800f, Model#=0x0b */
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "SMSC LAN8187",

	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),

	/* basic functions */
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.config_init	= smsc_phy_config_init,
}, {
	.phy_id		= 0x0007c0c0, /* OUI=0x00800f, Model#=0x0c */
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "SMSC LAN8700",

	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),

	/* basic functions */
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.config_init	= smsc_phy_config_init,
}, {
	.phy_id		= 0x0007c0d0, /* OUI=0x00800f, Model#=0x0d */
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "SMSC LAN911x Internal PHY",

	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),

	/* basic functions */
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.config_init	= lan911x_config_init,
}, {
	.phy_id		= 0x0007c0f0, /* OUI=0x00800f, Model#=0x0f */
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "SMSC LAN8710/LAN8720",

	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),

	/* basic functions */
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.config_init	= lan87xx_config_init,
} };

static int __init smsc_init(void)
{
	return phy_drivers_register(smsc_phy_driver, ARRAY_SIZE(smsc_phy_driver));
}
fs_initcall(smsc_init);
