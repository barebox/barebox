/*
 * drivers/net/phy/micrel.c
 *
 * Driver for Micrel PHYs
 *
 * Author: David J. Choi
 *
 * Copyright (c) 2010 Micrel, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Support : ksz9021 1000/100/10 phy from Micrel
 *		ks8001, ks8737, ks8721, ks8041, ks8051 100/10 phy
 */

#include <common.h>
#include <init.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>

/* Operation Mode Strap Override */
#define MII_KSZPHY_OMSO				0x16
#define KSZPHY_OMSO_B_CAST_OFF			(1 << 9)
#define KSZPHY_OMSO_RMII_OVERRIDE		(1 << 1)
#define KSZPHY_OMSO_MII_OVERRIDE		(1 << 0)

/* general PHY control reg in vendor specific block. */
#define	MII_KSZPHY_CTRL			0x1F
/* bitmap of PHY register to set interrupt mode */
#define KSZPHY_CTRL_INT_ACTIVE_HIGH		(1 << 9)
#define KSZ9021_CTRL_INT_ACTIVE_HIGH		(1 << 14)
#define KS8737_CTRL_INT_ACTIVE_HIGH		(1 << 14)
#define KSZ8051_RMII_50MHZ_CLK			(1 << 7)

/* Write/read to/from extended registers */
#define MII_KSZPHY_EXTREG                       0x0b
#define KSZPHY_EXTREG_WRITE                     0x8000

#define MII_KSZPHY_EXTREG_WRITE                 0x0c
#define MII_KSZPHY_EXTREG_READ                  0x0d

/* Extended registers */
#define MII_KSZPHY_CLK_CONTROL_PAD_SKEW         0x104
#define MII_KSZPHY_RX_DATA_PAD_SKEW             0x105
#define MII_KSZPHY_TX_DATA_PAD_SKEW             0x106

#define PS_TO_REG				200

static int kszphy_extended_write(struct phy_device *phydev,
				u32 regnum, u16 val)
{
	phy_write(phydev, MII_KSZPHY_EXTREG, KSZPHY_EXTREG_WRITE | regnum);
	return phy_write(phydev, MII_KSZPHY_EXTREG_WRITE, val);
}

static int kszphy_extended_read(struct phy_device *phydev,
				u32 regnum)
{
	phy_write(phydev, MII_KSZPHY_EXTREG, regnum);
	return phy_read(phydev, MII_KSZPHY_EXTREG_READ);
}

static int kszphy_config_init(struct phy_device *phydev)
{
	return 0;
}

static int ksz8021_config_init(struct phy_device *phydev)
{
	const u16 val = KSZPHY_OMSO_B_CAST_OFF | KSZPHY_OMSO_RMII_OVERRIDE;
	phy_write(phydev, MII_KSZPHY_OMSO, val);
	return 0;
}

static int ks8051_config_init(struct phy_device *phydev)
{
	int regval;

	if (phydev->dev_flags & MICREL_PHY_50MHZ_CLK) {
		regval = phy_read(phydev, MII_KSZPHY_CTRL);
		regval |= KSZ8051_RMII_50MHZ_CLK;
		phy_write(phydev, MII_KSZPHY_CTRL, regval);
	}

	return 0;
}

static int ksz9021_load_values_from_of(struct phy_device *phydev,
				       struct device_node *of_node, u16 reg,
				       const char *field[])
{
	int val, regval, i;

	regval = kszphy_extended_read(phydev, reg);

	for (i = 0; i < 4; i++) {
		int shift = i * 4;

		if (of_property_read_u32(of_node, field[i], &val))
			continue;

		regval &= ~(0xf << shift);
		regval |= ((val / PS_TO_REG) & 0xf) << shift;
	}

	return kszphy_extended_write(phydev, reg, regval);
}

static int ksz9021_config_init(struct phy_device *phydev)
{
	struct device_d *dev = &phydev->dev;
	struct device_node *of_node = dev->device_node;
	const char *clk_pad_skew_names[] = {
		"txen-skew-ps", "txc-skew-ps",
		"rxdv-skew-ps", "rxc-skew-ps"
	};
	const char *rx_pad_skew_names[] = {
		"rxd0-skew-ps", "rxd1-skew-ps",
		"rxd2-skew-ps", "rxd3-skew-ps"
	};
	const char *tx_pad_skew_names[] = {
		"txd0-skew-ps", "txd1-skew-ps",
		"txd2-skew-ps", "txd3-skew-ps"
	};

	if (!of_node && dev->parent->device_node)
		of_node = dev->parent->device_node;

	if (of_node) {
		ksz9021_load_values_from_of(phydev, of_node,
				    MII_KSZPHY_CLK_CONTROL_PAD_SKEW,
				    clk_pad_skew_names);
		ksz9021_load_values_from_of(phydev, of_node,
				    MII_KSZPHY_RX_DATA_PAD_SKEW,
				    rx_pad_skew_names);
		ksz9021_load_values_from_of(phydev, of_node,
				    MII_KSZPHY_TX_DATA_PAD_SKEW,
				    tx_pad_skew_names);
	}

	return 0;
}

#define KSZ8873MLL_GLOBAL_CONTROL_4	0x06
#define KSZ8873MLL_GLOBAL_CONTROL_4_DUPLEX	(1 << 6)
#define KSZ8873MLL_GLOBAL_CONTROL_4_SPEED	(1 << 4)
int ksz8873mll_read_status(struct phy_device *phydev)
{
	int regval;

	/* dummy read */
	regval = phy_read(phydev, KSZ8873MLL_GLOBAL_CONTROL_4);

	regval = phy_read(phydev, KSZ8873MLL_GLOBAL_CONTROL_4);

	if (regval & KSZ8873MLL_GLOBAL_CONTROL_4_DUPLEX)
		phydev->duplex = DUPLEX_HALF;
	else
		phydev->duplex = DUPLEX_FULL;

	if (regval & KSZ8873MLL_GLOBAL_CONTROL_4_SPEED)
		phydev->speed = SPEED_10;
	else
		phydev->speed = SPEED_100;

	phydev->link = 1;
	phydev->pause = phydev->asym_pause = 0;

	return 0;
}

static int ksz8873mll_config_aneg(struct phy_device *phydev)
{
	return 0;
}

static int ksz8873mll_config_init(struct phy_device *phydev)
{
	phydev->autoneg = AUTONEG_DISABLE;
	phydev->link = 1;

	return 0;
}

static struct phy_driver ksphy_driver[] = {
{
	.phy_id		= PHY_ID_KS8737,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KS8737",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8021,
	.phy_id_mask	= 0x00ffffff,
	.drv.name	= "Micrel KSZ8021",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause |
			   SUPPORTED_Asym_Pause),
	.config_init	= ksz8021_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8031,
	.phy_id_mask	= 0x00ffffff,
	.drv.name	= "Micrel KSZ8031",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause |
			   SUPPORTED_Asym_Pause),
	.config_init	= ksz8021_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8041,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KSZ8041",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8051,
	.phy_id_mask	= 0x00fffff0,
	.drv.name		= "Micrel KSZ8051",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.config_init	= ks8051_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8001,
	.drv.name	= "Micrel KSZ8001 or KS8721",
	.phy_id_mask	= 0x00ffffff,
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	/*
	 * Due to a hw bug do not enable the Asym_Pause.
	 * Otherwise if you set the bit 11 in 4h you will have to unplug
	 * and replug the cable to make the phy work.
	 */
	.phy_id		= PHY_ID_KSZ9021,
	.phy_id_mask	= 0x000ffffe,
	.drv.name	= "Micrel KSZ9021 Gigabit PHY",
	.features	= (PHY_GBIT_FEATURES | SUPPORTED_Pause),
	.config_init	= ksz9021_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ9031,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KSZ9031 Gigabit PHY",
	.features	= (PHY_GBIT_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8873MLL,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KSZ8873MLL Switch",
	.features	= (SUPPORTED_Pause | SUPPORTED_Asym_Pause),
	.config_init	= ksz8873mll_config_init,
	.config_aneg	= ksz8873mll_config_aneg,
	.read_status	= ksz8873mll_read_status,
} };

static int ksphy_init(void)
{
	return phy_drivers_register(ksphy_driver,
		ARRAY_SIZE(ksphy_driver));
}
fs_initcall(ksphy_init);
