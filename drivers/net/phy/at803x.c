// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/net/phy/at803x.c
 *
 * Driver for Atheros 803x PHY
 *
 * Author: Matus Ujhelyi <ujhelyi.m@gmail.com>
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/string.h>
#include <linux/bitfield.h>
#include <linux/mdio.h>
#include <dt-bindings/net/qca-ar803x.h>

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

/* AT803x supports either the XTAL input pad, an internal PLL or the
 * DSP as clock reference for the clock output pad. The XTAL reference
 * is only used for 25 MHz output, all other frequencies need the PLL.
 * The DSP as a clock reference is used in synchronous ethernet
 * applications.
 *
 * By default the PLL is only enabled if there is a link. Otherwise
 * the PHY will go into low power state and disabled the PLL. You can
 * set the PLL_ON bit (see debug register 0x1f) to keep the PLL always
 * enabled.
 */
#define AT803X_MMD7_CLK25M			0x8016
#define AT803X_CLK_OUT_MASK			GENMASK(4, 2)
#define AT803X_CLK_OUT_25MHZ_XTAL		0
#define AT803X_CLK_OUT_25MHZ_DSP		1
#define AT803X_CLK_OUT_50MHZ_PLL		2
#define AT803X_CLK_OUT_50MHZ_DSP		3
#define AT803X_CLK_OUT_62_5MHZ_PLL		4
#define AT803X_CLK_OUT_62_5MHZ_DSP		5
#define AT803X_CLK_OUT_125MHZ_PLL		6
#define AT803X_CLK_OUT_125MHZ_DSP		7

/* The AR8035 has another mask which is compatible with the AR8031/AR8033 mask
 * but doesn't support choosing between XTAL/PLL and DSP.
 */
#define AT8035_CLK_OUT_MASK			GENMASK(4, 3)

#define AT803X_CLK_OUT_STRENGTH_MASK		GENMASK(8, 7)
#define AT803X_CLK_OUT_STRENGTH_FULL		0
#define AT803X_CLK_OUT_STRENGTH_HALF		1
#define AT803X_CLK_OUT_STRENGTH_QUARTER		2

#define ATH9331_PHY_ID 0x004dd041
#define ATH8030_PHY_ID 0x004dd076
#define ATH8031_PHY_ID 0x004dd074
#define ATH8032_PHY_ID 0x004dd023
#define ATH8035_PHY_ID 0x004dd072
#define AT8030_PHY_ID_MASK			0xffffffef

struct at803x_priv {
	u16 clk_25m_reg;
	u16 clk_25m_mask;
};

static bool at803x_match_phy_id(struct phy_device *phydev, u32 phy_id)
{
	struct phy_driver *drv = to_phy_driver(phydev->dev.driver);

	return (phydev->phy_id & drv->phy_id_mask)
		== (phy_id & drv->phy_id_mask);
}

static int at803x_parse_dt(struct phy_device *phydev)
{
	const struct device_d *dev = &phydev->dev;
	const struct device_node *node = dev->device_node;
	struct at803x_priv *priv = phydev->priv;
	unsigned int sel, mask;
	u32 freq, strength;
	int ret;

	ret = of_property_read_u32(node, "qca,clk-out-frequency", &freq);
	if (!ret) {
		mask = AT803X_CLK_OUT_MASK;
		switch (freq) {
		case 25000000:
			sel = AT803X_CLK_OUT_25MHZ_XTAL;
			break;
		case 50000000:
			sel = AT803X_CLK_OUT_50MHZ_PLL;
			break;
		case 62500000:
			sel = AT803X_CLK_OUT_62_5MHZ_PLL;
			break;
		case 125000000:
			sel = AT803X_CLK_OUT_125MHZ_PLL;
			break;
		default:
			dev_err(dev, "invalid qca,clk-out-frequency\n");
			return -EINVAL;
		}

		priv->clk_25m_reg |= FIELD_PREP(mask, sel);
		priv->clk_25m_mask |= mask;

		/* Fixup for the AR8030/AR8035. This chip has another mask and
		 * doesn't support the DSP reference. Eg. the lowest bit of the
		 * mask. The upper two bits select the same frequencies. Mask
		 * the lowest bit here.
		 *
		 * Warning:
		 *   There was no datasheet for the AR8030 available so this is
		 *   just a guess. But the AR8035 is listed as pin compatible
		 *   to the AR8030 so there might be a good chance it works on
		 *   the AR8030 too.
		 */
		if (at803x_match_phy_id(phydev, ATH8030_PHY_ID) ||
		    at803x_match_phy_id(phydev, ATH8035_PHY_ID)) {
			priv->clk_25m_reg &= AT8035_CLK_OUT_MASK;
			priv->clk_25m_mask &= AT8035_CLK_OUT_MASK;
		}
	}

	ret = of_property_read_u32(node, "qca,clk-out-strength", &strength);
	if (!ret) {
		priv->clk_25m_mask |= AT803X_CLK_OUT_STRENGTH_MASK;
		switch (strength) {
		case AR803X_STRENGTH_FULL:
			priv->clk_25m_reg |= AT803X_CLK_OUT_STRENGTH_FULL;
			break;
		case AR803X_STRENGTH_HALF:
			priv->clk_25m_reg |= AT803X_CLK_OUT_STRENGTH_HALF;
			break;
		case AR803X_STRENGTH_QUARTER:
			priv->clk_25m_reg |= AT803X_CLK_OUT_STRENGTH_QUARTER;
			break;
		default:
			dev_err(dev, "invalid qca,clk-out-strength\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int at803x_probe(struct phy_device *phydev)
{
	struct at803x_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	return at803x_parse_dt(phydev);
}

static int at803x_clk_out_config(struct phy_device *phydev)
{
	struct at803x_priv *priv = phydev->priv;
	int val;

	if (!priv->clk_25m_mask)
		return 0;

	val = phy_read_mmd_indirect(phydev, AT803X_MMD7_CLK25M, MDIO_MMD_AN);
	if (val < 0)
		return val;

	val &= ~priv->clk_25m_mask;
	val |= priv->clk_25m_reg;

	phy_write_mmd_indirect(phydev, AT803X_MMD7_CLK25M, MDIO_MMD_AN, val);

	return 0;
}

static int at803x_config_init(struct phy_device *phydev)
{
	int ret;

	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

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

	return at803x_clk_out_config(phydev);
}

static struct phy_driver at803x_driver[] = {
{
	/* ATHEROS 8035 */
	.phy_id		= ATH8035_PHY_ID,
	.phy_id_mask	= AT8030_PHY_ID_MASK,
	.drv.name	= "Atheros 8035 ethernet",
	.probe		= at803x_probe,
	.config_init	= at803x_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
}, {
	/* ATHEROS 8030 */
	.phy_id		= ATH8030_PHY_ID,
	.phy_id_mask	= AT8030_PHY_ID_MASK,
	.drv.name	= "Atheros 8030 ethernet",
	.config_init	= at803x_config_init,
	.probe		= at803x_probe,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
}, {
	/* ATHEROS 8031 */
	.phy_id		= ATH8031_PHY_ID,
	.phy_id_mask	= AT8030_PHY_ID_MASK,
	.drv.name	= "Atheros 8031 ethernet",
	.probe		= at803x_probe,
	.config_init	= at803x_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
} };

device_phy_drivers(at803x_driver);
