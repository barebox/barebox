// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/net/phy/micrel.c
 *
 * Driver for Micrel PHYs
 *
 * Author: David J. Choi
 *
 * Copyright (c) 2010 Micrel, Inc.
 *
 * Support : ksz9021 1000/100/10 phy from Micrel
 *		ks8001, ks8737, ks8721, ks8041, ks8051 100/10 phy
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/mdio.h>
#include <linux/micrel_phy.h>
#include <linux/bitfield.h>

/* Operation Mode Strap Override */
#define MII_KSZPHY_OMSO				0x16
#define KSZPHY_OMSO_B_CAST_OFF			BIT(9)
#define KSZPHY_OMSO_NAND_TREE_ON		BIT(5)
#define KSZPHY_OMSO_RMII_OVERRIDE		BIT(1)
#define KSZPHY_OMSO_MII_OVERRIDE		BIT(0)

/* general PHY control reg in vendor specific block. */
#define	MII_KSZPHY_CTRL				0x1f
/* bitmap of PHY register to set interrupt mode */
#define KSZPHY_CTRL_INT_ACTIVE_HIGH		BIT(9)
#define KSZ9021_CTRL_INT_ACTIVE_HIGH		BIT(14)
#define KS8737_CTRL_INT_ACTIVE_HIGH		BIT(14)
#define KSZPHY_RMII_REF_CLK_SEL			BIT(7)

/* PHY Control 1 */
#define MII_KSZPHY_CTRL_1			0x1e

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

struct kszphy_type {
	u32 led_mode_reg;
	bool has_broadcast_disable;
	bool has_nand_tree_disable;
	bool has_rmii_ref_clk_sel;
};

struct kszphy_priv {
	const struct kszphy_type *type;
	int led_mode;
	bool rmii_ref_clk_sel;
	bool rmii_ref_clk_sel_val;
};

static const struct kszphy_type ksz8001_type = {
	.led_mode_reg		= MII_KSZPHY_CTRL_1,
};

static const struct kszphy_type ksz8021_type = {
	.led_mode_reg		= MII_KSZPHY_CTRL,
	.has_broadcast_disable	= true,
	.has_nand_tree_disable	= true,
	.has_rmii_ref_clk_sel	= true,
};

static const struct kszphy_type ksz8041_type = {
	.led_mode_reg		= MII_KSZPHY_CTRL_1,
};

static const struct kszphy_type ksz8051_type = {
	.led_mode_reg		= MII_KSZPHY_CTRL,
	.has_nand_tree_disable	= true,
};

static const struct kszphy_type ksz8081_type = {
	.led_mode_reg		= MII_KSZPHY_CTRL,
	.has_broadcast_disable	= true,
	.has_nand_tree_disable	= true,
	.has_rmii_ref_clk_sel	= true,
};

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

static int kszphy_rmii_clk_sel(struct phy_device *phydev, bool val)
{
	int ctrl;

	ctrl = phy_read(phydev, MII_KSZPHY_CTRL);
	if (ctrl < 0)
		return ctrl;

	if (val)
		ctrl |= KSZPHY_RMII_REF_CLK_SEL;
	else
		ctrl &= ~KSZPHY_RMII_REF_CLK_SEL;

	return phy_write(phydev, MII_KSZPHY_CTRL, ctrl);
}

/* Handle LED mode, shift = position of first led mode bit, usually 4 or 14 */
static int kszphy_led_mode(struct phy_device *phydev, int reg, int shift)
{
	const struct device *dev = &phydev->dev;
	const struct device_node *of_node = dev->of_node ? : dev->parent->of_node;
	u32 val;

	if (!of_property_read_u32(of_node, "micrel,led-mode", &val)) {
		if (val > 0x03) {
			dev_err(dev, "led-mode 0x%02x out of range\n", val);
			return -1;
		}
		return phy_modify(phydev, reg, 0x03 << shift, val << shift);
	}
	return 0;
}

static int kszphy_setup_led(struct phy_device *phydev, u32 reg, int val)
{
	const struct device *dev = &phydev->dev;
	int rc, temp, shift;

	switch (reg) {
	case MII_KSZPHY_CTRL_1:
		shift = 14;
		break;
	case MII_KSZPHY_CTRL:
		shift = 4;
		break;
	default:
		return -EINVAL;
	}

	temp = phy_read(phydev, reg);
	if (temp < 0) {
		rc = temp;
		goto out;
	}

	temp &= ~(3 << shift);
	temp |= val << shift;
	rc = phy_write(phydev, reg, temp);
out:
	if (rc < 0)
		dev_err(dev, "failed to set led mode\n");

	return rc;
}

/* Disable PHY address 0 as the broadcast address, so that it can be used as a
 * unique (non-broadcast) address on a shared bus.
 */
static int kszphy_broadcast_disable(struct phy_device *phydev)
{
	const struct device *dev = &phydev->dev;
	int ret;

	ret = phy_read(phydev, MII_KSZPHY_OMSO);
	if (ret < 0)
		goto out;

	ret = phy_write(phydev, MII_KSZPHY_OMSO, ret | KSZPHY_OMSO_B_CAST_OFF);
out:
	if (ret)
		dev_err(dev, "failed to disable broadcast address\n");

	return ret;
}

static int kszphy_nand_tree_disable(struct phy_device *phydev)
{
	const struct device *dev = &phydev->dev;
	int ret;

	ret = phy_read(phydev, MII_KSZPHY_OMSO);
	if (ret < 0)
		goto out;

	if (!(ret & KSZPHY_OMSO_NAND_TREE_ON))
		return 0;

	ret = phy_write(phydev, MII_KSZPHY_OMSO,
			ret & ~KSZPHY_OMSO_NAND_TREE_ON);
out:
	if (ret)
		dev_err(dev, "failed to disable NAND tree mode\n");

	return ret;
}

/* Some config bits need to be set again on resume, handle them here. */
static int kszphy_config_reset(struct phy_device *phydev)
{
	struct kszphy_priv *priv = phydev->priv;
	int ret;

	if (priv->rmii_ref_clk_sel) {
		ret = kszphy_rmii_clk_sel(phydev, priv->rmii_ref_clk_sel_val);
		if (ret) {
			dev_err(&phydev->dev,
				   "failed to set rmii reference clock\n");
			return ret;
		}
	}

	if (priv->led_mode >= 0)
		kszphy_setup_led(phydev, priv->type->led_mode_reg, priv->led_mode);

	return 0;
}

static int kszphy_config_init(struct phy_device *phydev)
{
	struct kszphy_priv *priv = phydev->priv;
	const struct kszphy_type *type;

	if (!priv)
		return 0;

	type = priv->type;

	if (type->has_broadcast_disable)
		kszphy_broadcast_disable(phydev);

	if (type->has_nand_tree_disable)
		kszphy_nand_tree_disable(phydev);

	return kszphy_config_reset(phydev);
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
	const struct device *dev = &phydev->dev;
	const struct device_node *of_node = dev->of_node;
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

	if (!of_node && dev->parent->of_node)
		of_node = dev->parent->of_node;

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

		kszphy_led_mode(phydev, 0x11, 6);
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
#define MII_KSZ9031RN_RX_CTL_M		GENMASK(7, 4)
#define MII_KSZ9031RN_TX_CTL_M		GENMASK(3, 0)

#define MII_KSZ9031RN_RX_DATA_PAD_SKEW	5
#define MII_KSZ9031RN_RXD3		GENMASK(15, 12)
#define MII_KSZ9031RN_RXD2		GENMASK(11, 8)
#define MII_KSZ9031RN_RXD1		GENMASK(7, 4)
#define MII_KSZ9031RN_RXD0		GENMASK(3, 0)

#define MII_KSZ9031RN_TX_DATA_PAD_SKEW	6
#define MII_KSZ9031RN_TXD3		GENMASK(15, 12)
#define MII_KSZ9031RN_TXD2		GENMASK(11, 8)
#define MII_KSZ9031RN_TXD1		GENMASK(7, 4)
#define MII_KSZ9031RN_TXD0		GENMASK(3, 0)

#define MII_KSZ9031RN_CLK_PAD_SKEW	8
#define MII_KSZ9031RN_GTX_CLK		GENMASK(9, 5)
#define MII_KSZ9031RN_RX_CLK		GENMASK(4, 0)

/* KSZ9031 has internal RGMII_IDRX = 1.2ns and RGMII_IDTX = 0ns. To
 * provide different RGMII options we need to configure delay offset
 * for each pad relative to build in delay.
 */
/* keep rx as "No delay adjustment" and set rx_clk to +0.60ns to get delays of
 * 1.80ns
 */
#define RX_ID				0x7
#define RX_CLK_ID			0x19

/* set rx to +0.30ns and rx_clk to -0.90ns to compensate the
 * internal 1.2ns delay.
 */
#define RX_ND				0xc
#define RX_CLK_ND			0x0

/* set tx to -0.42ns and tx_clk to +0.96ns to get 1.38ns delay */
#define TX_ID				0x0
#define TX_CLK_ID			0x1f

/* set tx and tx_clk to "No delay adjustment" to keep 0ns
 * dealy
 */
#define TX_ND				0x7
#define TX_CLK_ND			0xf

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
		newval = phy_read_mmd(phydev, MDIO_MMD_WIS, reg);
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

	phy_write_mmd(phydev, MDIO_MMD_WIS, reg, newval);
	return 0;
}

static int ksz9031_center_flp_timing(struct phy_device *phydev)
{
	/* Center KSZ9031RNX FLP timing at 16ms. */
	phy_write_mmd(phydev, 0, MII_KSZ9031RN_FLP_BURST_TX_HI, 0x0006);
	phy_write_mmd(phydev, 0, MII_KSZ9031RN_FLP_BURST_TX_LO, 0x1a80);

	return genphy_restart_aneg(phydev);
}

static int ksz9031_config_rgmii_delay(struct phy_device *phydev)
{
	u16 rx, tx, rx_clk, tx_clk;

	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		tx = TX_ND;
		tx_clk = TX_CLK_ND;
		rx = RX_ND;
		rx_clk = RX_CLK_ND;
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		tx = TX_ID;
		tx_clk = TX_CLK_ID;
		rx = RX_ID;
		rx_clk = RX_CLK_ID;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		tx = TX_ND;
		tx_clk = TX_CLK_ND;
		rx = RX_ID;
		rx_clk = RX_CLK_ID;
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		tx = TX_ID;
		tx_clk = TX_CLK_ID;
		rx = RX_ND;
		rx_clk = RX_CLK_ND;
		break;
	default:
		return 0;
	}

	phy_write_mmd(phydev, MDIO_MMD_WIS, MII_KSZ9031RN_CONTROL_PAD_SKEW,
		      FIELD_PREP(MII_KSZ9031RN_RX_CTL_M, rx) |
		      FIELD_PREP(MII_KSZ9031RN_TX_CTL_M, tx));

	phy_write_mmd(phydev, MDIO_MMD_WIS, MII_KSZ9031RN_RX_DATA_PAD_SKEW,
		      FIELD_PREP(MII_KSZ9031RN_RXD3, rx) |
		      FIELD_PREP(MII_KSZ9031RN_RXD2, rx) |
		      FIELD_PREP(MII_KSZ9031RN_RXD1, rx) |
		      FIELD_PREP(MII_KSZ9031RN_RXD0, rx));

	phy_write_mmd(phydev, MDIO_MMD_WIS, MII_KSZ9031RN_TX_DATA_PAD_SKEW,
		      FIELD_PREP(MII_KSZ9031RN_TXD3, tx) |
		      FIELD_PREP(MII_KSZ9031RN_TXD2, tx) |
		      FIELD_PREP(MII_KSZ9031RN_TXD1, tx) |
		      FIELD_PREP(MII_KSZ9031RN_TXD0, tx));

	phy_write_mmd(phydev, MDIO_MMD_WIS, MII_KSZ9031RN_CLK_PAD_SKEW,
		      FIELD_PREP(MII_KSZ9031RN_GTX_CLK, tx_clk) |
		      FIELD_PREP(MII_KSZ9031RN_RX_CLK, rx_clk));
	return 0;
}

static int ksz9031_config_init(struct phy_device *phydev)
{
	const struct device *dev = &phydev->dev;
	const struct device_node *of_node = dev->of_node;
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
	int ret;

	if (!of_node && dev->parent->of_node)
		of_node = dev->parent->of_node;

	if (of_node) {
		if (phy_interface_is_rgmii(phydev)) {
			ret = ksz9031_config_rgmii_delay(phydev);
			if (ret < 0)
				return ret;
		}

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

		/* Silicon Errata Sheet (DS80000691D or DS80000692D):
		 * When the device links in the 1000BASE-T slave mode only,
		 * the optional 125MHz reference output clock (CLK125_NDO)
		 * has wide duty cycle variation.
		 *
		 * The optional CLK125_NDO clock does not meet the RGMII
		 * 45/55 percent (min/max) duty cycle requirement and therefore
		 * cannot be used directly by the MAC side for clocking
		 * applications that have setup/hold time requirements on
		 * rising and falling clock edges.
		 *
		 * Workaround:
		 * Force the phy to be the master to receive a stable clock
		 * which meets the duty cycle requirement.
		 */
		if (of_property_read_bool(of_node, "micrel,force-master")) {
			ret = phy_read(phydev, MII_CTRL1000);
			if (ret < 0)
				goto err_force_master;

			/* enable master mode, config & prefer master */
			ret |= CTL1000_ENABLE_MASTER | CTL1000_AS_MASTER;
			ret = phy_write(phydev, MII_CTRL1000, ret);
			if (ret < 0)
				goto err_force_master;
		}
	}

	return ksz9031_center_flp_timing(phydev);

err_force_master:
	dev_err(dev, "failed to force the phy to master mode\n");
	return ret;
}

#define KSZ9131_SKEW_5BIT_MAX	2400
#define KSZ9131_SKEW_4BIT_MAX	800
#define KSZ9131_OFFSET		700
#define KSZ9131_STEP		100

static int ksz9131_of_load_skew_values(struct phy_device *phydev,
				       const struct device_node *of_node,
				       u16 reg, size_t field_sz,
				       const char *field[], u8 numfields)
{
	int val[4] = {-(1 + KSZ9131_OFFSET), -(2 + KSZ9131_OFFSET),
		      -(3 + KSZ9131_OFFSET), -(4 + KSZ9131_OFFSET)};
	int skewval, skewmax = 0;
	int matches = 0;
	u16 maxval;
	u16 newval;
	u16 mask;
	int i;

	/* psec properties in dts should mean x pico seconds */
	if (field_sz == 5)
		skewmax = KSZ9131_SKEW_5BIT_MAX;
	else
		skewmax = KSZ9131_SKEW_4BIT_MAX;

	for (i = 0; i < numfields; i++)
		if (!of_property_read_s32(of_node, field[i], &skewval)) {
			if (skewval < -KSZ9131_OFFSET)
				skewval = -KSZ9131_OFFSET;
			else if (skewval > skewmax)
				skewval = skewmax;

			val[i] = skewval + KSZ9131_OFFSET;
			matches++;
		}

	if (!matches)
		return 0;

	if (matches < numfields)
		newval = phy_read_mmd(phydev, 2, reg);
	else
		newval = 0;

	maxval = (field_sz == 4) ? 0xf : 0x1f;
	for (i = 0; i < numfields; i++)
		if (val[i] != -(i + 1 + KSZ9131_OFFSET)) {
			mask = 0xffff;
			mask ^= maxval << (field_sz * i);
			newval = (newval & mask) |
				(((val[i] / KSZ9131_STEP) & maxval)
					<< (field_sz * i));
		}

	return phy_write_mmd(phydev, 2, reg, newval);
}

#define KSZ9131RN_MMD_COMMON_CTRL_REG	2
#define KSZ9131RN_RXC_DLL_CTRL		76
#define KSZ9131RN_TXC_DLL_CTRL		77
#define KSZ9131RN_DLL_CTRL_BYPASS	BIT_MASK(12)
#define KSZ9131RN_DLL_ENABLE_DELAY	0
#define KSZ9131RN_DLL_DISABLE_DELAY	BIT(12)

static int ksz9131_config_rgmii_delay(struct phy_device *phydev)
{
	u16 rxcdll_val, txcdll_val;
	int ret;

	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		rxcdll_val = KSZ9131RN_DLL_DISABLE_DELAY;
		txcdll_val = KSZ9131RN_DLL_DISABLE_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		rxcdll_val = KSZ9131RN_DLL_ENABLE_DELAY;
		txcdll_val = KSZ9131RN_DLL_ENABLE_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		rxcdll_val = KSZ9131RN_DLL_ENABLE_DELAY;
		txcdll_val = KSZ9131RN_DLL_DISABLE_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		rxcdll_val = KSZ9131RN_DLL_DISABLE_DELAY;
		txcdll_val = KSZ9131RN_DLL_ENABLE_DELAY;
		break;
	default:
		return 0;
	}

	ret = phy_modify_mmd(phydev, KSZ9131RN_MMD_COMMON_CTRL_REG,
			     KSZ9131RN_RXC_DLL_CTRL, KSZ9131RN_DLL_CTRL_BYPASS,
			     rxcdll_val);
	if (ret < 0)
		return ret;

	return phy_modify_mmd(phydev, KSZ9131RN_MMD_COMMON_CTRL_REG,
			      KSZ9131RN_TXC_DLL_CTRL, KSZ9131RN_DLL_CTRL_BYPASS,
			      txcdll_val);
}

/* Silicon Errata DS80000693B
 *
 * When LEDs are configured in Individual Mode, LED1 is ON in a no-link
 * condition. Workaround is to set register 0x1e, bit 9, this way LED1 behaves
 * according to the datasheet (off if there is no link).
 */
static int ksz9131_led_errata(struct phy_device *phydev)
{
	int reg;

	reg = phy_read_mmd(phydev, 2, 0);
	if (reg < 0)
		return reg;

	if (!(reg & BIT(4)))
		return 0;

	return phy_set_bits(phydev, 0x1e, BIT(9));
}

static int ksz9131_config_init(struct phy_device *phydev)
{
	const struct device *dev = &phydev->dev;
	const struct device_node *of_node = dev->of_node;
	static const char *clk_skews[2] = {"rxc-skew-psec", "txc-skew-psec"};
	static const char *rx_data_skews[4] = {
		"rxd0-skew-psec", "rxd1-skew-psec",
		"rxd2-skew-psec", "rxd3-skew-psec"
	};
	static const char *tx_data_skews[4] = {
		"txd0-skew-psec", "txd1-skew-psec",
		"txd2-skew-psec", "txd3-skew-psec"
	};
	static const char *control_skews[2] = {"txen-skew-psec", "rxdv-skew-psec"};
	int ret;

	if (!of_node && dev->parent->of_node)
		of_node = dev->parent->of_node;

	if (!of_node)
		return 0;

	if (phy_interface_is_rgmii(phydev)) {
		ret = ksz9131_config_rgmii_delay(phydev);
		if (ret < 0)
			return ret;
	}

	ret = ksz9131_of_load_skew_values(phydev, of_node,
					  MII_KSZ9031RN_CLK_PAD_SKEW, 5,
					  clk_skews, 2);
	if (ret < 0)
		return ret;

	ret = ksz9131_of_load_skew_values(phydev, of_node,
					  MII_KSZ9031RN_CONTROL_PAD_SKEW, 4,
					  control_skews, 2);
	if (ret < 0)
		return ret;

	ret = ksz9131_of_load_skew_values(phydev, of_node,
					  MII_KSZ9031RN_RX_DATA_PAD_SKEW, 4,
					  rx_data_skews, 4);
	if (ret < 0)
		return ret;

	ret = ksz9131_of_load_skew_values(phydev, of_node,
					  MII_KSZ9031RN_TX_DATA_PAD_SKEW, 4,
					  tx_data_skews, 4);
	if (ret < 0)
		return ret;

	ret = ksz9131_led_errata(phydev);
	if (ret < 0)
		return ret;

	return 0;
}

#define KSZ8873MLL_GLOBAL_CONTROL_4	0x06
#define KSZ8873MLL_GLOBAL_CONTROL_4_DUPLEX	BIT(6)
#define KSZ8873MLL_GLOBAL_CONTROL_4_SPEED	BIT(4)
static int ksz8873mll_read_status(struct phy_device *phydev)
{
	int regval;

	/* dummy read */
	(void)phy_read(phydev, KSZ8873MLL_GLOBAL_CONTROL_4);

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
		phy_wait_aneg_done(phydev);
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

static int kszphy_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->dev;
	struct device_node *np = dev->of_node;
	struct phy_driver *drv = to_phy_driver(dev->driver);
	const struct kszphy_type *type = drv->driver_data;
	struct kszphy_priv *priv;
	struct clk *clk;
	int ret;

	priv = xzalloc(sizeof(*priv));

	phydev->priv = priv;

	priv->type = type;

	if (type->led_mode_reg) {
		ret = of_property_read_u32(np, "micrel,led-mode",
				&priv->led_mode);
		if (ret)
			priv->led_mode = -1;

		if (priv->led_mode > 3) {
			dev_err(dev, "invalid led mode: 0x%02x\n",
				priv->led_mode);
			priv->led_mode = -1;
		}
	} else {
		priv->led_mode = -1;
	}

	clk = clk_get(dev, "rmii-ref");
	/* NOTE: clk may be NULL if building without CONFIG_HAVE_CLK */
	if (!IS_ERR_OR_NULL(clk)) {
		unsigned long rate = clk_get_rate(clk);
		bool rmii_ref_clk_sel_25_mhz;

		priv->rmii_ref_clk_sel = type->has_rmii_ref_clk_sel;
		rmii_ref_clk_sel_25_mhz = of_property_read_bool(np,
				"micrel,rmii-reference-clock-select-25-mhz");

		if (rate > 24500000 && rate < 25500000) {
			priv->rmii_ref_clk_sel_val = rmii_ref_clk_sel_25_mhz;
		} else if (rate > 49500000 && rate < 50500000) {
			priv->rmii_ref_clk_sel_val = !rmii_ref_clk_sel_25_mhz;
		} else {
			dev_err(dev, "Clock rate out of range: %ld\n", rate);
			return -EINVAL;
		}
	}

	return 0;
}

static struct phy_driver ksphy_driver[] = {
{
	.phy_id		= PHY_ID_KS8737,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KS8737",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8021,
	.phy_id_mask	= 0x00ffffff,
	.drv.name	= "Micrel KSZ8021",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause |
			   SUPPORTED_Asym_Pause),
	.driver_data	= &ksz8021_type,
	.probe		= kszphy_probe,
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8031,
	.phy_id_mask	= 0x00ffffff,
	.drv.name	= "Micrel KSZ8031",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause |
			   SUPPORTED_Asym_Pause),
	.driver_data	= &ksz8021_type,
	.probe		= kszphy_probe,
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8041,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KSZ8041",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.driver_data	= &ksz8041_type,
	.probe		= kszphy_probe,
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8051,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Micrel KSZ8051",
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.driver_data	= &ksz8051_type,
	.probe		= kszphy_probe,
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8081,
	.phy_id_mask	= MICREL_PHY_ID_MASK,
	.drv.name	= "Micrel KSZ8081/91",
	.driver_data	= &ksz8081_type,
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.probe		= kszphy_probe,
	.config_init	= kszphy_config_init,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
}, {
	.phy_id		= PHY_ID_KSZ8001,
	.phy_id_mask	= 0x00ffffff,
	.drv.name	= "Micrel KSZ8001 or KS8721",
	.driver_data	= &ksz8001_type,
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.probe		= kszphy_probe,
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
	.phy_id		= PHY_ID_KSZ9131,
	.phy_id_mask	= 0x00fffff0,
	.drv.name	= "Microchip KSZ9131 Gigabit PHY",
	.features	= (PHY_GBIT_FEATURES | SUPPORTED_Pause),
	.config_init	= ksz9131_config_init,
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

device_phy_drivers(ksphy_driver);
