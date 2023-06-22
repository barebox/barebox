// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for the Texas Instruments DP83867 PHY
 *
 * Copyright (C) 2015 Texas Instruments Inc.
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>

#include <dt-bindings/net/ti-dp83867.h>
#include <linux/mdio.h>

#define DP83867_PHY_ID		0x2000a231
#define DP83867_DEVADDR		MDIO_MMD_VEND2

#define MII_DP83867_PHYCTRL	0x10
#define MII_DP83867_PHYSTS	0x11
#define MII_DP83867_MICR	0x12
#define MII_DP83867_ISR		0x13
#define MII_DP83867_CFG2	0x14
#define MII_DP83867_BISCR	0x16
#define DP83867_CTRL		0x1f
#define DP83867_CFG3		0x1e

/* Extended Registers */
#define DP83867_CFG4		0x0031
#define DP83867_RGMIICTL	0x0032
#define DP83867_STRAP_STS1	0x006E
#define DP83867_RGMIIDCTL	0x0086
#define DP83867_IO_MUX_CFG	0x0170

#define DP83867_SW_RESET	BIT(15)
#define DP83867_SW_RESTART	BIT(14)

/* MICR Interrupt bits */
#define MII_DP83867_MICR_AN_ERR_INT_EN		BIT(15)
#define MII_DP83867_MICR_SPEED_CHNG_INT_EN	BIT(14)
#define MII_DP83867_MICR_DUP_MODE_CHNG_INT_EN	BIT(13)
#define MII_DP83867_MICR_PAGE_RXD_INT_EN	BIT(12)
#define MII_DP83867_MICR_AUTONEG_COMP_INT_EN	BIT(11)
#define MII_DP83867_MICR_LINK_STS_CHNG_INT_EN	BIT(10)
#define MII_DP83867_MICR_FALSE_CARRIER_INT_EN	BIT(8)
#define MII_DP83867_MICR_SLEEP_MODE_CHNG_INT_EN	BIT(4)
#define MII_DP83867_MICR_WOL_INT_EN		BIT(3)
#define MII_DP83867_MICR_XGMII_ERR_INT_EN	BIT(2)
#define MII_DP83867_MICR_POL_CHNG_INT_EN	BIT(1)
#define MII_DP83867_MICR_JABBER_INT_EN		BIT(0)

/* RGMIICTL bits */
#define DP83867_RGMII_TX_CLK_DELAY_EN		BIT(1)
#define DP83867_RGMII_RX_CLK_DELAY_EN		BIT(0)

/* STRAP_STS1 bits */
#define DP83867_STRAP_STS1_RESERVED		BIT(11)

/* PHY CTRL bits */
#define DP83867_PHYCR_FIFO_DEPTH_SHIFT		14
#define DP83867_PHYCR_FIFO_DEPTH_MASK		(3 << 14)
#define DP83867_MDI_CROSSOVER			5
#define DP83867_MDI_CROSSOVER_AUTO		0b10
#define DP83867_MDI_CROSSOVER_MDIX		0b01
#define DP83867_PHYCTRL_SGMIIEN			0x0800
#define DP83867_PHYCTRL_RXFIFO_SHIFT		12
#define DP83867_PHYCTRL_TXFIFO_SHIFT		14
#define DP83867_PHYCR_RESERVED_MASK		BIT(11)

/* PHY STS bits */
#define DP83867_PHYSTS_SPEED_1000		BIT(15)
#define DP83867_PHYSTS_SPEED_100		BIT(14)
#define DP83867_PHYSTS_DUPLEX_FULL		BIT(13)

/* RGMIIDCTL bits */
#define DP83867_RGMII_TX_CLK_DELAY_SHIFT	4

/* CFG2 bits */
#define DP83867_DOWNSHIFT_EN			(BIT(8) | BIT(9))
#define DP83867_DOWNSHIFT_ATTEMPT_MASK		(BIT(10) | BIT(11))
#define DP83867_DOWNSHIFT_1_COUNT_VAL		0
#define DP83867_DOWNSHIFT_2_COUNT_VAL		1
#define DP83867_DOWNSHIFT_4_COUNT_VAL		2
#define DP83867_DOWNSHIFT_8_COUNT_VAL		3

/* CFG4 bits */
#define DP83867_CFG4_SGMII_AUTONEG_TIMER_MASK	0x60
#define DP83867_CFG4_SGMII_AUTONEG_TIMER_16MS	0x00
#define DP83867_CFG4_SGMII_AUTONEG_TIMER_2US	0x20
#define DP83867_CFG4_SGMII_AUTONEG_TIMER_800US	0x40
#define DP83867_CFG4_SGMII_AUTONEG_TIMER_11MS	0x60
#define DP83867_CFG4_RESVDBIT7			BIT(7)
#define DP83867_CFG4_RESVDBIT8			BIT(8)

/* IO_MUX_CFG bits */
#define DP83867_IO_MUX_CFG_IO_IMPEDANCE_CTRL	0x1f

#define DP83867_IO_MUX_CFG_IO_IMPEDANCE_MAX	0x0
#define DP83867_IO_MUX_CFG_IO_IMPEDANCE_MIN	0x1f
#define DP83867_IO_MUX_CFG_CLK_O_DISABLE	BIT(6)
#define DP83867_IO_MUX_CFG_CLK_O_SEL_MASK	(0x1f << 8)
#define DP83867_IO_MUX_CFG_CLK_O_SEL_SHIFT	8

/* CFG4 bits */
#define DP83867_CFG4_PORT_MIRROR_EN		BIT(0)

enum {
	DP83867_PORT_MIRROING_KEEP,
	DP83867_PORT_MIRROING_EN,
	DP83867_PORT_MIRROING_DIS,
};

struct dp83867_private {
	int rx_id_delay;
	int tx_id_delay;
	int fifo_depth;
	int io_impedance;
	int port_mirroring;
	bool rxctrl_strap_quirk;
	bool set_clk_output;
	u32 clk_output_sel;
};

static int dp83867_read_status(struct phy_device *phydev)
{
	int status;
	int ret;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	status = phy_read(phydev, MII_DP83867_PHYSTS);
	if (status < 0)
		return status;

	phydev->speed = SPEED_10;
	phydev->duplex = DUPLEX_HALF;

	if (status & DP83867_PHYSTS_SPEED_1000)
		phydev->speed = SPEED_1000;
	else if (status & DP83867_PHYSTS_SPEED_100)
		phydev->speed = SPEED_100;

	if (status & DP83867_PHYSTS_DUPLEX_FULL)
		phydev->duplex = DUPLEX_FULL;

	phydev->pause = phydev->asym_pause = 0;

	return 0;
}

static int dp83867_config_port_mirroring(struct phy_device *phydev)
{
	struct dp83867_private *dp83867 = phydev->priv;
	u16 val;

	val = phy_read_mmd_indirect(phydev, DP83867_CFG4, DP83867_DEVADDR);

	if (dp83867->port_mirroring == DP83867_PORT_MIRROING_EN)
		val |= DP83867_CFG4_PORT_MIRROR_EN;
	else
		val &= ~DP83867_CFG4_PORT_MIRROR_EN;

	phy_write_mmd_indirect(phydev, DP83867_CFG4, DP83867_DEVADDR, val);

	return 0;
}

static int dp83867_of_init(struct phy_device *phydev)
{
	struct dp83867_private *dp83867 = phydev->priv;
	struct device *dev = &phydev->dev;
	struct device_node *of_node = dev->of_node;
	int ret;

	if (!of_node)
		return -ENODEV;

	dp83867->io_impedance = -EINVAL;

	/* Optional configuration */
	ret = of_property_read_u32(of_node, "ti,clk-output-sel",
				   &dp83867->clk_output_sel);
	/* If not set, keep default */
	if (!ret) {
		dp83867->set_clk_output = true;
		/* Valid values are 0 to DP83867_CLK_O_SEL_REF_CLK or
		 * DP83867_CLK_O_SEL_OFF.
		 */
		if (dp83867->clk_output_sel > DP83867_CLK_O_SEL_REF_CLK &&
		    dp83867->clk_output_sel != DP83867_CLK_O_SEL_OFF) {
			dev_err(&phydev->dev, "ti,clk-output-sel value %u out of range\n",
				   dp83867->clk_output_sel);
			return -EINVAL;
		}
	}

	if (of_property_read_bool(of_node, "ti,max-output-impedance"))
		dp83867->io_impedance = DP83867_IO_MUX_CFG_IO_IMPEDANCE_MAX;
	else if (of_property_read_bool(of_node, "ti,min-output-impedance"))
		dp83867->io_impedance = DP83867_IO_MUX_CFG_IO_IMPEDANCE_MIN;

	dp83867->rxctrl_strap_quirk =
			of_property_read_bool(of_node,
					      "ti,dp83867-rxctrl-strap-quirk");

	ret = of_property_read_u32(of_node, "ti,rx-internal-delay",
				   &dp83867->rx_id_delay);
	if (ret && (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID))
		return ret;

	ret = of_property_read_u32(of_node, "ti,tx-internal-delay",
				   &dp83867->tx_id_delay);
	if (ret && (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID))
		return ret;

	if (of_property_read_bool(of_node, "enet-phy-lane-swap"))
		dp83867->port_mirroring = DP83867_PORT_MIRROING_EN;

	if (of_property_read_bool(of_node, "enet-phy-lane-no-swap"))
		dp83867->port_mirroring = DP83867_PORT_MIRROING_DIS;

	return of_property_read_u32(of_node, "ti,fifo-depth",
				    &dp83867->fifo_depth);
}

static inline bool phy_interface_is_sgmii(struct phy_device *phydev)
{
	return phydev->interface == PHY_INTERFACE_MODE_SGMII ||
	       phydev->interface == PHY_INTERFACE_MODE_QSGMII;
}

static int dp83867_config_init(struct phy_device *phydev)
{
	struct dp83867_private *dp83867;
	int ret;
	u16 val, delay;

	if (!phydev->priv) {
		dp83867 = kzalloc(sizeof(*dp83867), GFP_KERNEL);
		if (!dp83867)
			return -ENOMEM;

		phydev->priv = dp83867;
		ret = dp83867_of_init(phydev);
		if (ret)
			return ret;
	} else {
		dp83867 = phydev->priv;
	}

	/* Restart the PHY.  */
	val = phy_read(phydev, DP83867_CTRL);
	phy_write(phydev, DP83867_CTRL, val | DP83867_SW_RESTART);

	if (dp83867->rxctrl_strap_quirk) {
		val = phy_read_mmd_indirect(phydev, DP83867_CFG4,
					    DP83867_DEVADDR);
		val &= ~BIT(7);
		phy_write_mmd_indirect(phydev, DP83867_CFG4,
				       DP83867_DEVADDR, val);
	}

	if (phy_interface_is_rgmii(phydev)) {
		val = DP83867_MDI_CROSSOVER_AUTO << DP83867_MDI_CROSSOVER |
		      dp83867->fifo_depth << DP83867_PHYCR_FIFO_DEPTH_SHIFT;
		ret = phy_write(phydev, MII_DP83867_PHYCTRL, val);
		if (ret)
			return ret;

		val = phy_read_mmd_indirect(phydev, DP83867_RGMIICTL,
					    DP83867_DEVADDR);

		switch (phydev->interface) {
		case PHY_INTERFACE_MODE_RGMII_ID:
			val |= DP83867_RGMII_TX_CLK_DELAY_EN |
			       DP83867_RGMII_RX_CLK_DELAY_EN;
			break;
		case PHY_INTERFACE_MODE_RGMII_TXID:
			val |= DP83867_RGMII_TX_CLK_DELAY_EN;
			break;
		case PHY_INTERFACE_MODE_RGMII_RXID:
			val |= DP83867_RGMII_RX_CLK_DELAY_EN;
			break;
		default:
			break;
		}
		phy_write_mmd_indirect(phydev, DP83867_RGMIICTL,
				       DP83867_DEVADDR, val);

		delay = (dp83867->rx_id_delay |
			(dp83867->tx_id_delay << DP83867_RGMII_TX_CLK_DELAY_SHIFT));

		phy_write_mmd_indirect(phydev, DP83867_RGMIIDCTL,
				       DP83867_DEVADDR, delay);

		if (dp83867->io_impedance >= 0) {
			val = phy_read_mmd_indirect(phydev, DP83867_IO_MUX_CFG,
						    DP83867_DEVADDR);
			val &= ~DP83867_IO_MUX_CFG_IO_IMPEDANCE_CTRL;
			val |= (dp83867->io_impedance &
				DP83867_IO_MUX_CFG_IO_IMPEDANCE_CTRL);

			phy_write_mmd_indirect(phydev, DP83867_IO_MUX_CFG,
					       DP83867_DEVADDR, val);
		}
	} else if (phy_interface_is_sgmii(phydev)) {
		phy_write(phydev, MII_BMCR,
			  BMCR_ANENABLE | BMCR_FULLDPLX | BMCR_SPEED1000);

		phy_write_mmd_indirect(phydev, DP83867_RGMIICTL,
				       DP83867_DEVADDR, 0x0);

		val = DP83867_PHYCTRL_SGMIIEN |
		      DP83867_MDI_CROSSOVER_MDIX << DP83867_MDI_CROSSOVER |
		      dp83867->fifo_depth << DP83867_PHYCTRL_RXFIFO_SHIFT |
		      dp83867->fifo_depth << DP83867_PHYCTRL_TXFIFO_SHIFT;

		phy_write(phydev, MII_DP83867_PHYCTRL, val);
		phy_write(phydev, MII_DP83867_BISCR, 0x0);
	}

	val = phy_read(phydev, MII_DP83867_CFG2);
	val |= DP83867_DOWNSHIFT_EN;
	phy_write(phydev, MII_DP83867_CFG2, val);

	if (dp83867->port_mirroring != DP83867_PORT_MIRROING_KEEP)
		dp83867_config_port_mirroring(phydev);

	/* Clock output selection if muxing property is set */
	if (dp83867->set_clk_output) {
		u16 mask = DP83867_IO_MUX_CFG_CLK_O_DISABLE;

		if (dp83867->clk_output_sel == DP83867_CLK_O_SEL_OFF) {
			val = DP83867_IO_MUX_CFG_CLK_O_DISABLE;
		} else {
			mask |= DP83867_IO_MUX_CFG_CLK_O_SEL_MASK;
			val = dp83867->clk_output_sel <<
				DP83867_IO_MUX_CFG_CLK_O_SEL_SHIFT;
		}

		phy_modify_mmd_indirect(phydev, DP83867_IO_MUX_CFG,
				DP83867_DEVADDR, mask, val);
	}

	return 0;
}

static struct phy_driver dp83867_driver[] = {
	{
		.phy_id = DP83867_PHY_ID,
		.phy_id_mask = 0xfffffff0,
		.drv.name = "TI DP83867",
		.features = PHY_GBIT_FEATURES,
		.config_init = dp83867_config_init,
		.read_status = dp83867_read_status,
	},
};

device_phy_drivers(dp83867_driver);
