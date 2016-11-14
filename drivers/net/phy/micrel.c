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
#define KSZPHY_OMSO_B_CAST_OFF			BIT(9)
#define KSZPHY_OMSO_RMII_OVERRIDE		BIT(1)
#define KSZPHY_OMSO_MII_OVERRIDE		BIT(0)

/* general PHY control reg in vendor specific block. */
#define	MII_KSZPHY_CTRL			0x1F
/* bitmap of PHY register to set interrupt mode */
#define KSZPHY_CTRL_INT_ACTIVE_HIGH		BIT(9)
#define KSZ9021_CTRL_INT_ACTIVE_HIGH		BIT(14)
#define KS8737_CTRL_INT_ACTIVE_HIGH		BIT(14)
#define KSZ8051_RMII_50MHZ_CLK			BIT(7)

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
	u16 val;

	val = phy_read(phydev, MII_KSZPHY_OMSO);
	val |= KSZPHY_OMSO_B_CAST_OFF;
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
				       const struct device_node *of_node,
				       u16 reg, const char *field[])
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
	const struct device_d *dev = &phydev->dev;
	const struct device_node *of_node = dev->device_node;
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

#define KSZ9031_PS_TO_REG		60

/* Extended registers */
/* MMD Address 0x0 */
#define MII_KSZ9031RN_FLP_BURST_TX_LO	3
#define MII_KSZ9031RN_FLP_BURST_TX_HI	4

/* MMD Address 0x2 */
#define MII_KSZ9031RN_CONTROL_PAD_SKEW	4
#define MII_KSZ9031RN_RX_DATA_PAD_SKEW	5
#define MII_KSZ9031RN_TX_DATA_PAD_SKEW	6
#define MII_KSZ9031RN_CLK_PAD_SKEW	8

static int ksz9031_of_load_skew_values(struct phy_device *phydev,
					const struct device_node *of_node,
					u16 reg, size_t field_sz,
					const char *field[], u8 numfields)
{
	int val[4] = {-1, -2, -3, -4};
	int matches = 0;
	u16 mask;
	u16 maxval;
	u16 newval;
	int i;

	for (i = 0; i < numfields; i++)
		if (!of_property_read_u32(of_node, field[i], val + i))
			matches++;

	if (!matches)
		return 0;

	if (matches < numfields)
		newval = phy_read_mmd_indirect(phydev, reg, 2);
	else
		newval = 0;

	maxval = (field_sz == 4) ? 0xf : 0x1f;
	for (i = 0; i < numfields; i++)
		if (val[i] != -(i + 1)) {
			mask = 0xffff;
			mask ^= maxval << (field_sz * i);
			newval = (newval & mask) |
				(((val[i] / KSZ9031_PS_TO_REG) & maxval)
				<< (field_sz * i));
		}

	phy_write_mmd_indirect(phydev, reg, 2, newval);
	return 0;
}

static int ksz9031_center_flp_timing(struct phy_device *phydev)
{
	/* Center KSZ9031RNX FLP timing at 16ms. */
	phy_write_mmd_indirect(phydev, MII_KSZ9031RN_FLP_BURST_TX_HI, 0, 0x0006);
	phy_write_mmd_indirect(phydev, MII_KSZ9031RN_FLP_BURST_TX_LO, 0, 0x1a80);

	return genphy_restart_aneg(phydev);
}

static int ksz9031_config_init(struct phy_device *phydev)
{
	const struct device_d *dev = &phydev->dev;
	const struct device_node *of_node = dev->device_node;
	static const char *clk_skews[2] = {"rxc-skew-ps", "txc-skew-ps"};
	static const char *rx_data_skews[4] = {
		"rxd0-skew-ps", "rxd1-skew-ps",
		"rxd2-skew-ps", "rxd3-skew-ps"
	};
	static const char *tx_data_skews[4] = {
		"txd0-skew-ps", "txd1-skew-ps",
		"txd2-skew-ps", "txd3-skew-ps"
	};
	static const char *control_skews[2] = {"txen-skew-ps", "rxdv-skew-ps"};

	if (!of_node && dev->parent->device_node)
		of_node = dev->parent->device_node;

	if (of_node) {
		ksz9031_of_load_skew_values(phydev, of_node,
				MII_KSZ9031RN_CLK_PAD_SKEW, 5,
				clk_skews, 2);

		ksz9031_of_load_skew_values(phydev, of_node,
				MII_KSZ9031RN_CONTROL_PAD_SKEW, 4,
				control_skews, 2);

		ksz9031_of_load_skew_values(phydev, of_node,
				MII_KSZ9031RN_RX_DATA_PAD_SKEW, 4,
				rx_data_skews, 4);

		ksz9031_of_load_skew_values(phydev, of_node,
				MII_KSZ9031RN_TX_DATA_PAD_SKEW, 4,
				tx_data_skews, 4);
	}

	return ksz9031_center_flp_timing(phydev);
}

#define KSZ8873MLL_GLOBAL_CONTROL_4	0x06
#define KSZ8873MLL_GLOBAL_CONTROL_4_DUPLEX	BIT(6)
#define KSZ8873MLL_GLOBAL_CONTROL_4_SPEED	BIT(4)
static int ksz8873mll_read_status(struct phy_device *phydev)
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

static int ksz9031_read_status(struct phy_device *phydev)
{
	int err;
	int regval;

	err = genphy_read_status(phydev);
	if (err)
		return err;

	/* Make sure the PHY is not broken. Read idle error count,
	 * and reset the PHY if it is maxed out.
	 */
	regval = phy_read(phydev, MII_STAT1000);
	if ((regval & 0xff) == 0xff) {
		phy_init_hw(phydev);
		phydev->link = 0;
	}

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
	.drv.name	= "Micrel KSZ8051",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.config_init	= ks8051_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8081,
	.phy_id_mask	= MICREL_PHY_ID_MASK,
	.drv.name	= "Micrel KSZ8081/91",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.config_init	= ksz8021_config_init,
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
	/* I saw the same issue like PHY_ID_KSZ9021 for Asym_Pause */
	.phy_id		= PHY_ID_KSZ9031,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KSZ9031 Gigabit PHY",
	.features	= (PHY_GBIT_FEATURES | SUPPORTED_Pause),
	.config_init	= ksz9031_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= ksz9031_read_status,
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
