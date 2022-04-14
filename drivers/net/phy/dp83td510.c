// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <linux/phy.h>

#define DP83TD510E_PHY_ID		0x20000181

#define DP83TD510E_PHY_STS		0x10
#define DP83TD510E_LINK_STATUS		BIT(0)

static int dp83td510_read_status(struct phy_device *phydev)
{
	u16 phy_sts;

	phy_sts = phy_read(phydev, DP83TD510E_PHY_STS);

	phydev->link = !!(phy_sts & DP83TD510E_LINK_STATUS);
	if (phydev->link) {
		phydev->duplex = DUPLEX_FULL;
		phydev->speed = SPEED_10;
	} else {
		phydev->speed = SPEED_UNKNOWN;
		phydev->duplex = DUPLEX_UNKNOWN;
	}

	return 0;
}

static int dp83td510_config_init(struct phy_device *phydev)
{
	phydev->supported = SUPPORTED_10baseT_Full | SUPPORTED_Autoneg;
	phydev->advertising = SUPPORTED_10baseT_Full | SUPPORTED_Autoneg;

	return 0;
}

static struct phy_driver dp83td510_driver[] = {
	{
		PHY_ID_MATCH_MODEL(DP83TD510E_PHY_ID),
		.drv.name	= "TI DP83TD510E",
		.read_status	= dp83td510_read_status,
		.config_init	= dp83td510_config_init,
	}
};
device_phy_drivers(dp83td510_driver);
