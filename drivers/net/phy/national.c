// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/net/phy/national.c
 *
 * Driver for National Semiconductor PHYs
 *
 * based on Stuart Menefy's linux national.c driver
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>

/* Advanced proprietary configuration */
#define NS_EXP_MEM_CTL	0x16
#define NS_EXP_MEM_DATA	0x1d
#define NS_EXP_MEM_ADD	0x1e

#define LED_CTRL_REG 0x13
#define AN_FALLBACK_AN 0x0001
#define AN_FALLBACK_CRC 0x0002
#define AN_FALLBACK_IE 0x0004
#define ALL_FALLBACK_ON (AN_FALLBACK_AN |  AN_FALLBACK_CRC | AN_FALLBACK_IE)

enum hdx_loopback {
	hdx_loopback_on = 0,
	hdx_loopback_off = 1,
};

static u8 ns_exp_read(struct phy_device *phydev, u16 reg)
{
	phy_write(phydev, NS_EXP_MEM_ADD, reg);
	return phy_read(phydev, NS_EXP_MEM_DATA);
}

static void ns_exp_write(struct phy_device *phydev, u16 reg, u8 data)
{
	phy_write(phydev, NS_EXP_MEM_ADD, reg);
	phy_write(phydev, NS_EXP_MEM_DATA, data);
}

static void ns_giga_speed_fallback(struct phy_device *phydev, int mode)
{
	int bmcr = phy_read(phydev, MII_BMCR);

	phy_write(phydev, MII_BMCR, (bmcr | BMCR_PDOWN));

	/* Enable 8 bit expended memory read/write (no auto increment) */
	phy_write(phydev, NS_EXP_MEM_CTL, 0);
	phy_write(phydev, NS_EXP_MEM_ADD, 0x1C0);
	phy_write(phydev, NS_EXP_MEM_DATA, 0x0008);
	phy_write(phydev, MII_BMCR, (bmcr & ~BMCR_PDOWN));
	phy_write(phydev, LED_CTRL_REG, mode);
}

static void ns_10_base_t_hdx_loopack(struct phy_device *phydev, int disable)
{
	if (disable)
		ns_exp_write(phydev, 0x1c0, ns_exp_read(phydev, 0x1c0) | 1);
	else
		ns_exp_write(phydev, 0x1c0,
			     ns_exp_read(phydev, 0x1c0) & 0xfffe);

	pr_debug("10BASE-T HDX loopback %s\n",
		 (ns_exp_read(phydev, 0x1c0) & 0x0001) ? "off" : "on");
}

static int ns_config_init(struct phy_device *phydev)
{
	ns_giga_speed_fallback(phydev, ALL_FALLBACK_ON);
	/* In the latest MAC or switches design, the 10 Mbps loopback
	   is desired to be turned off. */
	ns_10_base_t_hdx_loopack(phydev, hdx_loopback_off);

	return 0;
}

static struct phy_driver dp83865_driver = {
	.phy_id		= 0x20005c70,
	.phy_id_mask	= 0xfffffff0,
	.drv.name	= "NatSemi DP83865",
	.features	= PHY_GBIT_FEATURES |
			SUPPORTED_Pause | SUPPORTED_Asym_Pause,
	.config_init    = ns_config_init,
};

static int ns_phy_init(void)
{
	return phy_driver_register(&dp83865_driver);
}
fs_initcall(ns_phy_init);
