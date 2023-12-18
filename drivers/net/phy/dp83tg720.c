// SPDX-License-Identifier: GPL-2.0
/* Driver for the Texas Instruments DP83TG720 PHY
 * Copyright (c) 2023 Pengutronix, Oleksij Rempel <kernel@pengutronix.de>
 */
#include <common.h>
#include <linux/mdio.h>
#include <linux/phy.h>

#define DP83TG720S_PHY_ID			0x2000a284

/* MDIO_MMD_VEND2 registers */
#define DP83TG720S_MII_REG_10			0x10
#define DP83TG720S_LINK_STATUS			BIT(0)

#define DP83TG720S_RGMII_DELAY_CTRL		0x602
/* In RGMII mode, Enable or disable the internal delay for RXD */
#define DP83TG720S_RGMII_RX_CLK_SEL		BIT(1)
/* In RGMII mode, Enable or disable the internal delay for TXD */
#define DP83TG720S_RGMII_TX_CLK_SEL		BIT(0)

#define DP83TG720S_PHY_RESET			0x1f
#define DP83TG720S_HW_RESET			BIT(15)

static int dp83tg720_config_rgmii_delay(struct phy_device *phydev)
{
	u16 rgmii_delay_mask;
	u16 rgmii_delay = 0;

	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		rgmii_delay = 0;
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		rgmii_delay = DP83TG720S_RGMII_RX_CLK_SEL |
				DP83TG720S_RGMII_TX_CLK_SEL;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		rgmii_delay = DP83TG720S_RGMII_RX_CLK_SEL;
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		rgmii_delay = DP83TG720S_RGMII_TX_CLK_SEL;
		break;
	default:
		return 0;
	}

	rgmii_delay_mask = DP83TG720S_RGMII_RX_CLK_SEL |
			   DP83TG720S_RGMII_TX_CLK_SEL;

	return phy_modify_mmd(phydev, MDIO_MMD_VEND2,
			      DP83TG720S_RGMII_DELAY_CTRL, rgmii_delay_mask,
			      rgmii_delay);
}

static int dp83tg720_phy_init(struct phy_device *phydev)
{
	/* HW reset is needed to recover link if previous link was lost. SW
	 * reset is not enough.
	 */
	phy_write(phydev, DP83TG720S_PHY_RESET, DP83TG720S_HW_RESET);

	phydev->supported = SUPPORTED_1000baseT_Full;
	phydev->advertising = SUPPORTED_1000baseT_Full;

	if (phy_interface_is_rgmii(phydev))
		return dp83tg720_config_rgmii_delay(phydev);

	return 0;
}

static int dp83tg720_read_status(struct phy_device *phydev)
{
	u16 phy_sts;

	phy_sts = phy_read(phydev, DP83TG720S_MII_REG_10);
	phydev->link = !!(phy_sts & DP83TG720S_LINK_STATUS);
	if (!phydev->link) {
		phydev->speed = SPEED_UNKNOWN;
		phydev->duplex = DUPLEX_UNKNOWN;

		/* According to the "DP83TC81x, DP83TG72x Software
		 * Implementation Guide", the PHY needs to be reset after a
		 * link loss or if no link is created after at least 100ms.
		 */
		dp83tg720_phy_init(phydev);
		return 0;
	}

	phydev->duplex = DUPLEX_FULL;
	phydev->speed = SPEED_1000;

	return 0;
}

static struct phy_driver dp83tg720_driver[] = {
	{
		PHY_ID_MATCH_MODEL(DP83TG720S_PHY_ID),
		.drv.name	= "TI DP83TG720S",
		.read_status	= dp83tg720_read_status,
		.config_init	= dp83tg720_phy_init,
	}
};
device_phy_drivers(dp83tg720_driver);
