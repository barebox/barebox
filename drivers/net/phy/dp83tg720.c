// SPDX-License-Identifier: GPL-2.0
/* Driver for the Texas Instruments DP83TG720 PHY
 * Copyright (c) 2023 Pengutronix, Oleksij Rempel <kernel@pengutronix.de>
 */
#include <common.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <stdlib.h>

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

#define DP83TG720S_POLL_TIMEOUT_MS		100

#define DP83TG720S_LPS_CFG3			0x18c
/* Power modes are documented as bitfields but used as values */
/* Power Mode 0 is Normal mode */
#define DP83TG720S_LPS_CFG3_PWR_MODE_0		BIT(0)

struct dp83tg720_priv {
	uint64_t last_reset;
};

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
	struct dp83tg720_priv *priv = phydev->priv;
	int ret = 0;

	/* HW reset is needed to recover link if previous link was lost. SW
	 * reset is not enough.
	 */
	phy_write(phydev, DP83TG720S_PHY_RESET, DP83TG720S_HW_RESET);

	phydev->supported = SUPPORTED_1000baseT_Full;
	phydev->advertising = SUPPORTED_1000baseT_Full;
	priv->last_reset = get_time_ns();
	/* Randomize the polling interval to avoid reset synchronization with
	 * the link partner.  The polling interval is set to 150ms +/- 50ms.
	 */
	phydev->polling_interval = (DP83TG720S_POLL_TIMEOUT_MS +
				    (rand() % 10) * 10) * MSECOND;

	/* According to the "DP83TG720R-Q1 1000BASE-T1 Automotive Ethernet PHY
	 * datasheet (Rev. C)" - "T6.2 Post reset stabilization-time prior to
	 * MDC preamble for register access is 1ms."
	 */
	mdelay(1);

	if (phy_interface_is_rgmii(phydev)) {
		ret = dp83tg720_config_rgmii_delay(phydev);
		if (ret)
			return ret;
	}

	/* In case the PHY is bootstrapped in managed mode, we need to
	 * wake it.
	 */
	return phy_write_mmd(phydev, MDIO_MMD_VEND2, DP83TG720S_LPS_CFG3,
			     DP83TG720S_LPS_CFG3_PWR_MODE_0);
}

static int dp83tg720_read_status(struct phy_device *phydev)
{
	struct dp83tg720_priv *priv = phydev->priv;
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
		if (!priv->last_reset ||
		    is_timeout(priv->last_reset, phydev->polling_interval))
			dp83tg720_phy_init(phydev);
	} else {
		phydev->duplex = DUPLEX_FULL;
		phydev->speed = SPEED_1000;
	}

	return 0;
}

static int dp83tg720_probe(struct phy_device *phydev)
{
	struct dp83tg720_priv *priv;

	priv = xzalloc(sizeof(*priv));

	phydev->priv = priv;
	phydev->polling_interval = DP83TG720S_POLL_TIMEOUT_MS * MSECOND;

	return 0;
}

static struct phy_driver dp83tg720_driver[] = {
	{
		PHY_ID_MATCH_MODEL(DP83TG720S_PHY_ID),
		.drv.name	= "TI DP83TG720S",
		.read_status	= dp83tg720_read_status,
		.config_init	= dp83tg720_phy_init,
		.probe		= dp83tg720_probe,
	}
};
device_phy_drivers(dp83tg720_driver);
