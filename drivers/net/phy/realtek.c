// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/net/phy/realtek/realtek_main.c?id=4f0638b12451112de4138689fa679315c8d388dc
/* drivers/net/phy/realtek.c
 *
 * Driver for Realtek PHYs
 *
 * Author: Johnson Leung <r58129@freescale.com>
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 */
#include <common.h>
#include <init.h>
#include <linux/bitops.h>
#include <linux/mdio.h>
#include <linux/string_choices.h>
#include <linux/phy.h>

#define RTL821x_PHYSR				0x11
#define RTL821x_PHYSR_DUPLEX			BIT(13)
#define RTL821x_PHYSR_SPEED			GENMASK(15, 14)

#define RTL821x_INER				0x12
#define RTL8211B_INER_INIT			0x6400
#define RTL8211E_INER_LINK_STATUS		BIT(10)
#define RTL8211F_INER_PME			BIT(7)
#define RTL8211F_INER_LINK_STATUS		BIT(4)

#define RTL821x_INSR				0x13

#define RTL821x_EXT_PAGE_SELECT			0x1e

#define RTL821x_PAGE_SELECT			0x1f

#define RTL8211F_INSR				0x1d

#define RTL821x_SET_EXT_PAGE			0x07

/* RTL8211E extension page 164/0xa4 */
#define RTL8211E_RGMII_EXT_PAGE			0xa4
#define RTL8211E_RGMII_DELAY			0x1c
#define RTL8211E_CTRL_DELAY			BIT(13)
#define RTL8211E_TX_DELAY			BIT(12)
#define RTL8211E_RX_DELAY			BIT(11)
#define RTL8211E_DELAY_MASK			GENMASK(13, 11)

/* RTL8211F PHY configuration */
#define RTL8211F_PHYCR_PAGE			0xa43
#define RTL8211F_PHYCR1				0x18
#define RTL8211F_ALDPS_PLL_OFF			BIT(1)
#define RTL8211F_ALDPS_ENABLE			BIT(2)
#define RTL8211F_ALDPS_XTAL_OFF			BIT(12)

#define RTL8211F_PHYCR2				0x19
#define RTL8211F_CLKOUT_EN			BIT(0)
#define RTL8211F_PHYCR2_PHY_EEE_ENABLE		BIT(5)

/* RTL8211F(D)(I)-VD-CG CLKOUT configuration is specified via magic values
 * to undocumented register pages. The names here do not reflect the datasheet.
 * Unlike other PHY models, CLKOUT configuration does not go through PHYCR2.
 */
#define RTL8211FVD_CLKOUT_PAGE			0xd05
#define RTL8211FVD_CLKOUT_REG			0x11
#define RTL8211FVD_CLKOUT_EN			BIT(8)

/* RTL8211F RGMII configuration */
#define RTL8211F_RGMII_PAGE			0xd08

#define RTL8211F_TXCR				0x11
#define RTL8211F_TX_DELAY			BIT(8)

#define RTL8211F_RXCR				0x15
#define RTL8211F_RX_DELAY			BIT(3)

#define RTL8201F_ISR				0x1e
#define RTL8201F_IER				0x13

#define RTL821x_EXT_PAGE_SELECT			0x1e

/* RTL822X_VND2_XXXXX registers are only accessible when phydev->is_c45
 * is set, they cannot be accessed by C45-over-C22.
 */
#define RTL822X_VND2_C22_REG(reg)		(0xa400 + 2 * (reg))

#define RTL8221B_VND2_INER			0xa4d2
#define RTL8221B_VND2_INER_LINK_STATUS		BIT(4)

#define RTL8221B_VND2_INSR			0xa4d4

#define RTL8366RB_POWER_SAVE			0x15
#define RTL8366RB_POWER_SAVE_ON			BIT(12)

#define RTL_VND2_PHYSR				0xa434
#define RTL_VND2_PHYSR_DUPLEX			BIT(3)
#define RTL_VND2_PHYSR_SPEEDL			GENMASK(5, 4)
#define RTL_VND2_PHYSR_SPEEDH			GENMASK(10, 9)
#define RTL_VND2_PHYSR_MASTER			BIT(11)
#define RTL_VND2_PHYSR_SPEED_MASK		(RTL_VND2_PHYSR_SPEEDL | RTL_VND2_PHYSR_SPEEDH)

#define	RTL_MDIO_AN_10GBT_STAT			0xa5d6

#define RTL_GENERIC_PHYID			0x001cc800
#define RTL_8211FVD_PHYID			0x001cc878
#define RTL_8221B				0x001cc840
#define RTL_8221B_VB_CG				0x001cc849
#define RTL_8221B_VM_CG				0x001cc84a
#define RTL_8251B				0x001cc862
#define RTL_8261C				0x001cc890

MODULE_DESCRIPTION("Realtek PHY driver");
MODULE_AUTHOR("Johnson Leung");
MODULE_LICENSE("GPL");

struct rtl821x_priv {
	bool enable_aldps;
	bool disable_clk_out;
	struct clk *clk;
	/* rtl8211f */
	u16 iner;
};

static int rtl821x_read_page(struct phy_device *phydev)
{
	return phy_read(phydev, RTL821x_PAGE_SELECT);
}

static int rtl821x_write_page(struct phy_device *phydev, int page)
{
	return phy_write(phydev, RTL821x_PAGE_SELECT, page);
}

static int rtl821x_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->dev;
	struct rtl821x_priv *priv;

	priv = xzalloc(sizeof(*priv));

	priv->clk = clk_get_optional_enabled(dev, NULL);
	if (IS_ERR(priv->clk))
		return dev_err_probe(dev, PTR_ERR(priv->clk),
				     "failed to get phy clock\n");

	priv->enable_aldps = of_property_read_bool(dev->of_node,
						   "realtek,aldps-enable");
	priv->disable_clk_out = of_property_read_bool(dev->of_node,
						      "realtek,clkout-disable");

	phydev->priv = priv;

	return 0;
}

static int rtl8211_config_aneg(struct phy_device *phydev)
{
	int ret;

	ret = genphy_config_aneg(phydev);
	if (ret < 0)
		return ret;

	/* Quirk was copied from vendor driver. Unfortunately it includes no
	 * description of the magic numbers.
	 */
	if (phydev->speed == SPEED_100 && phydev->autoneg == AUTONEG_DISABLE) {
		phy_write(phydev, 0x17, 0x2138);
		phy_write(phydev, 0x0e, 0x0260);
	} else {
		phy_write(phydev, 0x17, 0x2108);
		phy_write(phydev, 0x0e, 0x0000);
	}

	return 0;
}

static int rtl8211c_config_init(struct phy_device *phydev)
{
	/* RTL8211C has an issue when operating in Gigabit slave mode */
	phy_set_bits(phydev, MII_CTRL1000,
		     CTL1000_ENABLE_MASTER | CTL1000_AS_MASTER);

	return genphy_config_init(phydev);
}

static int rtl8211f_config_rgmii_delay(struct phy_device *phydev)
{
	u16 val_txdly, val_rxdly;
	int ret;

	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		val_txdly = 0;
		val_rxdly = 0;
		break;

	case PHY_INTERFACE_MODE_RGMII_RXID:
		val_txdly = 0;
		val_rxdly = RTL8211F_RX_DELAY;
		break;

	case PHY_INTERFACE_MODE_RGMII_TXID:
		val_txdly = RTL8211F_TX_DELAY;
		val_rxdly = 0;
		break;

	case PHY_INTERFACE_MODE_RGMII_ID:
		val_txdly = RTL8211F_TX_DELAY;
		val_rxdly = RTL8211F_RX_DELAY;
		break;

	default: /* the rest of the modes imply leaving delay as is. */
		return 0;
	}

	ret = phy_modify_paged_changed(phydev, RTL8211F_RGMII_PAGE,
				       RTL8211F_TXCR, RTL8211F_TX_DELAY,
				       val_txdly);
	if (ret < 0) {
		phydev_err(phydev, "Failed to update the TX delay register: %pe\n",
			   ERR_PTR(ret));
		return ret;
	} else if (ret) {
		phydev_dbg(phydev,
			   "%s 2ns TX delay (and changing the value from pin-strapping RXD1)\n",
			   str_enable_disable(val_txdly));
	} else {
		phydev_dbg(phydev,
			   "2ns TX delay was already %s (by pin-strapping RXD1 configuration)\n",
			   str_enabled_disabled(val_txdly));
	}

	ret = phy_modify_paged_changed(phydev, RTL8211F_RGMII_PAGE,
				       RTL8211F_RXCR, RTL8211F_RX_DELAY,
				       val_rxdly);
	if (ret < 0) {
		phydev_err(phydev, "Failed to update the RX delay register: %pe\n",
			   ERR_PTR(ret));
		return ret;
	} else if (ret) {
		phydev_dbg(phydev,
			   "%s 2ns RX delay (and changing the value from pin-strapping RXD0)\n",
			   str_enable_disable(val_rxdly));
	} else {
		phydev_dbg(phydev,
			   "2ns RX delay was already %s (by pin-strapping RXD0 configuration)\n",
			   str_enabled_disabled(val_rxdly));
	}

	return 0;
}

static int rtl8211f_config_clk_out(struct phy_device *phydev)
{
	struct rtl821x_priv *priv = phydev->priv;
	struct phy_driver *drv = to_phy_driver(phydev->dev.driver);
	int ret;

	/* The value is preserved if the device tree property is absent */
	if (!priv->disable_clk_out)
		return 0;

	if (drv->phy_id == RTL_8211FVD_PHYID)
		ret = phy_modify_paged(phydev, RTL8211FVD_CLKOUT_PAGE,
				       RTL8211FVD_CLKOUT_REG,
				       RTL8211FVD_CLKOUT_EN, 0);
	else
		ret = phy_modify_paged(phydev, RTL8211F_PHYCR_PAGE,
				       RTL8211F_PHYCR2, RTL8211F_CLKOUT_EN, 0);
	if (ret)
		return ret;

	return genphy_soft_reset(phydev);
}

/* Advance Link Down Power Saving (ALDPS) mode changes crystal/clock behaviour,
 * which causes the RXC clock signal to stop for tens to hundreds of
 * milliseconds.
 *
 * Some MACs need the RXC clock to support their internal RX logic, so ALDPS is
 * only enabled based on an opt-in device tree property.
 */
static int rtl8211f_config_aldps(struct phy_device *phydev)
{
	struct rtl821x_priv *priv = phydev->priv;
	u16 mask = RTL8211F_ALDPS_PLL_OFF |
		   RTL8211F_ALDPS_ENABLE |
		   RTL8211F_ALDPS_XTAL_OFF;

	/* The value is preserved if the device tree property is absent */
	if (!priv->enable_aldps)
		return 0;

	return phy_modify_paged(phydev, RTL8211F_PHYCR_PAGE, RTL8211F_PHYCR1,
				mask, mask);
}

static int rtl8211f_config_phy_eee(struct phy_device *phydev)
{
	/* Disable PHY-mode EEE so LPI is passed to the MAC */
	return phy_modify_paged(phydev, RTL8211F_PHYCR_PAGE, RTL8211F_PHYCR2,
				RTL8211F_PHYCR2_PHY_EEE_ENABLE, 0);
}

static int rtl8211f_config_init(struct phy_device *phydev)
{
	struct device *dev = &phydev->dev;
	int ret;

	ret = rtl8211f_config_aldps(phydev);
	if (ret) {
		dev_err(dev, "aldps mode configuration failed: %pe\n",
			ERR_PTR(ret));
		return ret;
	}

	ret = rtl8211f_config_rgmii_delay(phydev);
	if (ret)
		return ret;

	ret = rtl8211f_config_clk_out(phydev);
	if (ret) {
		dev_err(dev, "clkout configuration failed: %pe\n",
			ERR_PTR(ret));
		return ret;
	}

	return rtl8211f_config_phy_eee(phydev);
}

static int rtl8211e_config_init(struct phy_device *phydev)
{
	int ret = 0, oldpage;
	u16 val;

	/* enable TX/RX delay for rgmii-* modes, and disable them for rgmii. */
	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		val = RTL8211E_CTRL_DELAY | 0;
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		val = RTL8211E_CTRL_DELAY | RTL8211E_TX_DELAY | RTL8211E_RX_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		val = RTL8211E_CTRL_DELAY | RTL8211E_RX_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val = RTL8211E_CTRL_DELAY | RTL8211E_TX_DELAY;
		break;
	default: /* the rest of the modes imply leaving delays as is. */
		return 0;
	}

	/* According to a sample driver there is a 0x1c config register on the
	 * 0xa4 extension page (0x7) layout. It can be used to disable/enable
	 * the RX/TX delays otherwise controlled by RXDLY/TXDLY pins.
	 * The configuration register definition:
	 * 14 = reserved
	 * 13 = Force Tx RX Delay controlled by bit12 bit11,
	 * 12 = RX Delay, 11 = TX Delay
	 * 10:0 = Test && debug settings reserved by realtek
	 */
	oldpage = phy_select_page(phydev, 0x7);
	if (oldpage < 0)
		goto err_restore_page;

	ret = phy_write(phydev, RTL821x_EXT_PAGE_SELECT, 0xa4);
	if (ret)
		goto err_restore_page;

	ret = phy_modify(phydev, 0x1c, RTL8211E_CTRL_DELAY
			 | RTL8211E_TX_DELAY | RTL8211E_RX_DELAY,
			 val);

err_restore_page:
	return phy_restore_page(phydev, oldpage, ret);
}

static int rtl8366rb_config_init(struct phy_device *phydev)
{
	int ret;

	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

	ret = phy_set_bits(phydev, RTL8366RB_POWER_SAVE,
			   RTL8366RB_POWER_SAVE_ON);
	if (ret) {
		dev_err(&phydev->dev,
			"error enabling power management\n");
	}

	return ret;
}

/* get actual speed to cover the downshift case */
static void rtlgen_decode_physr(struct phy_device *phydev, int val)
{
	/* bit 3
	 * 0: Half Duplex
	 * 1: Full Duplex
	 */
	if (val & RTL_VND2_PHYSR_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	switch (val & RTL_VND2_PHYSR_SPEED_MASK) {
	case 0x0000:
		phydev->speed = SPEED_10;
		break;
	case 0x0010:
		phydev->speed = SPEED_100;
		break;
	case 0x0020:
		phydev->speed = SPEED_1000;
		break;
	case 0x0200:
		phydev->speed = SPEED_10000;
		break;
	case 0x0210:
		phydev->speed = SPEED_2500;
		break;
	default:
		break;
	}
}

static int rtlgen_read_status(struct phy_device *phydev)
{
	int ret, val;

	ret = genphy_read_status(phydev);
	if (ret < 0)
		return ret;

	if (!phydev->link)
		return 0;

	val = phy_read_paged(phydev, 0xa43, 0x12);
	if (val < 0)
		return val;

	rtlgen_decode_physr(phydev, val);

	return 0;
}

static int rtl822x_read_status(struct phy_device *phydev)
{
	int lpadv, ret;

	ret = rtlgen_read_status(phydev);
	if (ret < 0)
		return ret;

	if (phydev->autoneg == AUTONEG_DISABLE)
		return 0;

	lpadv = phy_read_mmd(phydev, MDIO_MMD_VEND2, RTL_MDIO_AN_10GBT_STAT);
	if (lpadv < 0)
		return lpadv;

	return 0;
}

static struct phy_driver realtek_drvs[] = {
	{
		PHY_ID_MATCH_EXACT(0x00008201),
		.drv.name	= "RTL8201CP Ethernet",
		.features	= PHY_BASIC_FEATURES,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc816),
		.drv.name	= "RTL8201F Fast Ethernet",
		.features	= PHY_BASIC_FEATURES,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc840),
		.drv.name	= "RTL8226B_RTL8221B 2.5Gbps PHY",
		.features	= PHY_GBIT_FEATURES,
		.read_status	= rtl822x_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_MODEL(0x001cc880),
		.drv.name	= "RTL8208 Fast Ethernet",
		.features	= PHY_BASIC_FEATURES,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc910),
		.drv.name	= "RTL8211 Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.config_aneg	= rtl8211_config_aneg,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc912),
		.drv.name	= "RTL8211B Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc913),
		.drv.name	= "RTL8211C Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.config_init	= rtl8211c_config_init,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc914),
		.drv.name	= "RTL8211DN Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc915),
		.drv.name	= "RTL8211E Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.config_init	= &rtl8211e_config_init,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc916),
		.drv.name	= "RTL8211F Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.probe		= rtl821x_probe,
		.config_init	= &rtl8211f_config_init,
		.read_status	= rtlgen_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc800),
		.drv.name	= "Generic FE-GE Realtek PHY",
		.features	= PHY_GBIT_FEATURES,
		.config_init	= &rtl8211f_config_init,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(RTL_8211FVD_PHYID),
		.drv.name	= "RTL8211F-VD Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.probe		= rtl821x_probe,
		.config_init	= &rtl8211f_config_init,
		.read_status	= rtlgen_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc838),
		.drv.name	= "RTL8226-CG 2.5Gbps PHY",
		.features	= PHY_GBIT_FEATURES,
		.read_status	= rtl822x_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc848),
		.drv.name	= "RTL8226B-CG_RTL8221B-CG 2.5Gbps PHY",
		.features	= PHY_GBIT_FEATURES,
		.read_status	= rtl822x_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc849),
		.drv.name	= "RTL8221B-VB-CG 2.5Gbps PHY",
		.features	= PHY_GBIT_FEATURES,
		.read_status	= rtl822x_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc84a),
		.drv.name	= "RTL8221B-VM-CG 2.5Gbps PHY",
		.features	= PHY_GBIT_FEATURES,
		.read_status	= rtl822x_read_status,
		.read_page	= rtl821x_read_page,
		.write_page	= rtl821x_write_page,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc961),
		.drv.name	= "RTL8366RB Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
		.config_init	= &rtl8366rb_config_init,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc942),
		.drv.name	= "RTL8365MB-VC Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
	}, {
		PHY_ID_MATCH_EXACT(0x001cc960),
		.drv.name	= "RTL8366S Gigabit Ethernet",
		.features	= PHY_GBIT_FEATURES,
	},
};

device_phy_drivers(realtek_drvs);
