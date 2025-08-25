// SPDX-License-Identifier: GPL-2.0+
/*
 *  Driver for Analog Devices Industrial Ethernet PHYs
 *
 * Copyright 2019 Analog Devices Inc.
 */
#include <common.h>
#include <init.h>
#include <linux/bitfield.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <linux/string.h>

#define PHY_ID_ADIN1200				0x0283bc20
#define PHY_ID_ADIN1300				0x0283bc30

#define ADIN1300_MII_EXT_REG_PTR		0x0010
#define ADIN1300_MII_EXT_REG_DATA		0x0011

#define ADIN1300_PHY_CTRL1			0x0012
#define   ADIN1300_AUTO_MDI_EN			BIT(10)
#define   ADIN1300_MAN_MDIX_EN			BIT(9)
#define   ADIN1300_DIAG_CLK_EN			BIT(2)

#define ADIN1300_RX_ERR_CNT			0x0014

#define ADIN1300_PHY_CTRL_STATUS2		0x0015
#define   ADIN1300_NRG_PD_EN			BIT(3)
#define   ADIN1300_NRG_PD_TX_EN			BIT(2)
#define   ADIN1300_NRG_PD_STATUS		BIT(1)

#define ADIN1300_PHY_CTRL2			0x0016
#define   ADIN1300_DOWNSPEED_AN_100_EN		BIT(11)
#define   ADIN1300_DOWNSPEED_AN_10_EN		BIT(10)
#define   ADIN1300_GROUP_MDIO_EN		BIT(6)
#define   ADIN1300_DOWNSPEEDS_EN	\
	(ADIN1300_DOWNSPEED_AN_100_EN | ADIN1300_DOWNSPEED_AN_10_EN)

#define ADIN1300_PHY_CTRL3			0x0017
#define   ADIN1300_LINKING_EN			BIT(13)
#define   ADIN1300_DOWNSPEED_RETRIES_MSK	GENMASK(12, 10)

#define ADIN1300_INT_MASK_REG			0x0018
#define   ADIN1300_INT_MDIO_SYNC_EN		BIT(9)
#define   ADIN1300_INT_ANEG_STAT_CHNG_EN	BIT(8)
#define   ADIN1300_INT_ANEG_PAGE_RX_EN		BIT(6)
#define   ADIN1300_INT_IDLE_ERR_CNT_EN		BIT(5)
#define   ADIN1300_INT_MAC_FIFO_OU_EN		BIT(4)
#define   ADIN1300_INT_RX_STAT_CHNG_EN		BIT(3)
#define   ADIN1300_INT_LINK_STAT_CHNG_EN	BIT(2)
#define   ADIN1300_INT_SPEED_CHNG_EN		BIT(1)
#define   ADIN1300_INT_HW_IRQ_EN		BIT(0)
#define ADIN1300_INT_MASK_EN	\
	(ADIN1300_INT_LINK_STAT_CHNG_EN | ADIN1300_INT_HW_IRQ_EN)
#define ADIN1300_INT_STATUS_REG			0x0019

#define ADIN1300_PHY_STATUS1			0x001a
#define   ADIN1300_PAIR_01_SWAP			BIT(11)

/* EEE register addresses, accessible via Clause 22 access using
 * ADIN1300_MII_EXT_REG_PTR & ADIN1300_MII_EXT_REG_DATA.
 * The bit-fields are the same as specified by IEEE for EEE.
 */
#define ADIN1300_EEE_CAP_REG			0x8000
#define ADIN1300_EEE_ADV_REG			0x8001
#define ADIN1300_EEE_LPABLE_REG			0x8002
#define ADIN1300_CLOCK_STOP_REG			0x9400
#define ADIN1300_LPI_WAKE_ERR_CNT_REG		0xa000

#define ADIN1300_CDIAG_RUN			0xba1b
#define   ADIN1300_CDIAG_RUN_EN			BIT(0)

/*
 * The XSIM3/2/1 and XSHRT3/2/1 are actually relative.
 * For CDIAG_DTLD_RSLTS(0) it's ADIN1300_CDIAG_RSLT_XSIM3/2/1
 * For CDIAG_DTLD_RSLTS(1) it's ADIN1300_CDIAG_RSLT_XSIM3/2/0
 * For CDIAG_DTLD_RSLTS(2) it's ADIN1300_CDIAG_RSLT_XSIM3/1/0
 * For CDIAG_DTLD_RSLTS(3) it's ADIN1300_CDIAG_RSLT_XSIM2/1/0
 */
#define ADIN1300_CDIAG_DTLD_RSLTS(x)		(0xba1d + (x))
#define   ADIN1300_CDIAG_RSLT_BUSY		BIT(10)
#define   ADIN1300_CDIAG_RSLT_XSIM3		BIT(9)
#define   ADIN1300_CDIAG_RSLT_XSIM2		BIT(8)
#define   ADIN1300_CDIAG_RSLT_XSIM1		BIT(7)
#define   ADIN1300_CDIAG_RSLT_SIM		BIT(6)
#define   ADIN1300_CDIAG_RSLT_XSHRT3		BIT(5)
#define   ADIN1300_CDIAG_RSLT_XSHRT2		BIT(4)
#define   ADIN1300_CDIAG_RSLT_XSHRT1		BIT(3)
#define   ADIN1300_CDIAG_RSLT_SHRT		BIT(2)
#define   ADIN1300_CDIAG_RSLT_OPEN		BIT(1)
#define   ADIN1300_CDIAG_RSLT_GOOD		BIT(0)

#define ADIN1300_CDIAG_FLT_DIST(x)		(0xba21 + (x))

#define ADIN1300_GE_SOFT_RESET_REG		0xff0c
#define   ADIN1300_GE_SOFT_RESET		BIT(0)

#define ADIN1300_GE_CLK_CFG_REG			0xff1f
#define   ADIN1300_GE_CLK_CFG_MASK		GENMASK(5, 0)
#define   ADIN1300_GE_CLK_CFG_RCVR_125		BIT(5)
#define   ADIN1300_GE_CLK_CFG_FREE_125		BIT(4)
#define   ADIN1300_GE_CLK_CFG_REF_EN		BIT(3)
#define   ADIN1300_GE_CLK_CFG_HRT_RCVR		BIT(2)
#define   ADIN1300_GE_CLK_CFG_HRT_FREE		BIT(1)
#define   ADIN1300_GE_CLK_CFG_25		BIT(0)

#define ADIN1300_GE_RGMII_CFG_REG		0xff23
#define   ADIN1300_GE_RGMII_RX_MSK		GENMASK(8, 6)
#define   ADIN1300_GE_RGMII_RX_SEL(x)		\
		FIELD_PREP(ADIN1300_GE_RGMII_RX_MSK, x)
#define   ADIN1300_GE_RGMII_GTX_MSK		GENMASK(5, 3)
#define   ADIN1300_GE_RGMII_GTX_SEL(x)		\
		FIELD_PREP(ADIN1300_GE_RGMII_GTX_MSK, x)
#define   ADIN1300_GE_RGMII_RXID_EN		BIT(2)
#define   ADIN1300_GE_RGMII_TXID_EN		BIT(1)
#define   ADIN1300_GE_RGMII_EN			BIT(0)

/* RGMII internal delay settings for rx and tx for ADIN1300 */
#define ADIN1300_RGMII_1_60_NS			0x0001
#define ADIN1300_RGMII_1_80_NS			0x0002
#define	ADIN1300_RGMII_2_00_NS			0x0000
#define	ADIN1300_RGMII_2_20_NS			0x0006
#define	ADIN1300_RGMII_2_40_NS			0x0007

#define ADIN1300_GE_RMII_CFG_REG		0xff24
#define   ADIN1300_GE_RMII_FIFO_DEPTH_MSK	GENMASK(6, 4)
#define   ADIN1300_GE_RMII_FIFO_DEPTH_SEL(x)	\
		FIELD_PREP(ADIN1300_GE_RMII_FIFO_DEPTH_MSK, x)
#define   ADIN1300_GE_RMII_EN			BIT(0)

/* RMII fifo depth values */
#define ADIN1300_RMII_4_BITS			0x0000
#define ADIN1300_RMII_8_BITS			0x0001
#define ADIN1300_RMII_12_BITS			0x0002
#define ADIN1300_RMII_16_BITS			0x0003
#define ADIN1300_RMII_20_BITS			0x0004
#define ADIN1300_RMII_24_BITS			0x0005

/**
 * struct adin_cfg_reg_map - map a config value to aregister value
 * @cfg:	value in device configuration
 * @reg:	value in the register
 */
struct adin_cfg_reg_map {
	int cfg;
	int reg;
};

static const struct adin_cfg_reg_map adin_rgmii_delays[] = {
	{ 1600, ADIN1300_RGMII_1_60_NS },
	{ 1800, ADIN1300_RGMII_1_80_NS },
	{ 2000, ADIN1300_RGMII_2_00_NS },
	{ 2200, ADIN1300_RGMII_2_20_NS },
	{ 2400, ADIN1300_RGMII_2_40_NS },
	{ },
};

static const struct adin_cfg_reg_map adin_rmii_fifo_depths[] = {
	{ 4,  ADIN1300_RMII_4_BITS },
	{ 8,  ADIN1300_RMII_8_BITS },
	{ 12, ADIN1300_RMII_12_BITS },
	{ 16, ADIN1300_RMII_16_BITS },
	{ 20, ADIN1300_RMII_20_BITS },
	{ 24, ADIN1300_RMII_24_BITS },
	{ },
};

static int adin_lookup_reg_value(const struct adin_cfg_reg_map *tbl, int cfg)
{
	size_t i;

	for (i = 0; tbl[i].cfg; i++) {
		if (tbl[i].cfg == cfg)
			return tbl[i].reg;
	}

	return -EINVAL;
}

static u32 adin_get_reg_value(struct phy_device *phydev,
			      const char *prop_name,
			      const struct adin_cfg_reg_map *tbl,
			      u32 dflt)
{
	struct device *dev = &phydev->bus->dev;
	u32 val;
	int rc;

	if (of_property_read_u32(dev->of_node, prop_name, &val))
		return dflt;

	rc = adin_lookup_reg_value(tbl, val);
	if (rc < 0) {
		phydev_warn(phydev,
			    "Unsupported value %u for %s using default (%u)\n",
			    val, prop_name, dflt);
		return dflt;
	}

	return rc;
}

static u16 adin_ext_read(struct phy_device *phydev, const u32 regnum)
{
	u16 val;

	phy_write(phydev, ADIN1300_MII_EXT_REG_PTR, regnum);
	val = phy_read(phydev, ADIN1300_MII_EXT_REG_DATA);

	phydev_dbg(phydev, "%s: adin@0x%x 0x%x=0x%x\n", __func__, phydev->addr,
		   regnum, val);

	return val;
}

static int adin_ext_write(struct phy_device *phydev, const u32 regnum, const u16 val)
{
	phydev_dbg(phydev, "%s: adin@0x%x 0x%x=0x%x\n", __func__, phydev->addr,
		   regnum, val);

	phy_write(phydev, ADIN1300_MII_EXT_REG_PTR, regnum);

	return phy_write(phydev, ADIN1300_MII_EXT_REG_DATA, val);
}

static int adin_config_rgmii_mode(struct phy_device *phydev)
{
	u32 val;
	int reg;

	if (!phy_interface_is_rgmii(phydev))
		return phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1,
					  ADIN1300_GE_RGMII_CFG_REG,
					  ADIN1300_GE_RGMII_EN);

	reg = adin_ext_read(phydev, ADIN1300_GE_RGMII_CFG_REG);
	if (reg < 0)
		return reg;

	reg |= ADIN1300_GE_RGMII_EN;

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID) {
		reg |= ADIN1300_GE_RGMII_RXID_EN;

		val = adin_get_reg_value(phydev, "adi,rx-internal-delay-ps",
					 adin_rgmii_delays,
					 ADIN1300_RGMII_2_00_NS);
		reg &= ~ADIN1300_GE_RGMII_RX_MSK;
		reg |= ADIN1300_GE_RGMII_RX_SEL(val);
	} else {
		reg &= ~ADIN1300_GE_RGMII_RXID_EN;
	}

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID) {
		reg |= ADIN1300_GE_RGMII_TXID_EN;

		val = adin_get_reg_value(phydev, "adi,tx-internal-delay-ps",
					 adin_rgmii_delays,
					 ADIN1300_RGMII_2_00_NS);
		reg &= ~ADIN1300_GE_RGMII_GTX_MSK;
		reg |= ADIN1300_GE_RGMII_GTX_SEL(val);
	} else {
		reg &= ~ADIN1300_GE_RGMII_TXID_EN;
	}

	return adin_ext_write(phydev, ADIN1300_GE_RGMII_CFG_REG, reg);
}

static int adin_config_rmii_mode(struct phy_device *phydev)
{
	u32 val;
	int reg;

	if (phydev->interface != PHY_INTERFACE_MODE_RMII)
		return phy_clear_bits_mmd(phydev, MDIO_MMD_VEND1,
					  ADIN1300_GE_RMII_CFG_REG,
					  ADIN1300_GE_RMII_EN);

	reg = phy_read_mmd(phydev, MDIO_MMD_VEND1, ADIN1300_GE_RMII_CFG_REG);
	if (reg < 0)
		return reg;

	reg |= ADIN1300_GE_RMII_EN;

	val = adin_get_reg_value(phydev, "adi,fifo-depth-bits",
				 adin_rmii_fifo_depths,
				 ADIN1300_RMII_8_BITS);

	reg &= ~ADIN1300_GE_RMII_FIFO_DEPTH_MSK;
	reg |= ADIN1300_GE_RMII_FIFO_DEPTH_SEL(val);

	return phy_write_mmd(phydev, MDIO_MMD_VEND1,
			     ADIN1300_GE_RMII_CFG_REG, reg);
}

static int adin_config_init(struct phy_device *phydev)
{
	int rc;

	rc = adin_config_rgmii_mode(phydev);
	if (rc < 0)
		return rc;

	rc = adin_config_rmii_mode(phydev);
	if (rc < 0)
		return rc;

	return genphy_config_init(phydev);
}

static int adin_soft_reset(struct phy_device *phydev)
{
	int rc;

	/* Read link and autonegotiation status */
	/* The reset bit is self-clearing, set it and wait */
	rc = phy_set_bits_mmd(phydev, MDIO_MMD_VEND1,
			      ADIN1300_GE_SOFT_RESET_REG,
			      ADIN1300_GE_SOFT_RESET);
	if (rc < 0)
		return rc;

	mdelay(20);

	/* If we get a read error something may be wrong */
	rc = phy_read_mmd(phydev, MDIO_MMD_VEND1,
			  ADIN1300_GE_SOFT_RESET_REG);

	return rc < 0 ? rc : 0;
}

static int adin_config_aneg(struct phy_device *phydev)
{
	int ret;

	ret = genphy_config_aneg(phydev);
	if (ret < 0)
		return ret;

	/* Enable Autonegotiation */
	phy_write(phydev, MII_BMCR, BMCR_ANENABLE);
	phydev->autoneg = AUTONEG_ENABLE;

	return 0;
}

static struct phy_driver adin_driver[] = {
	{
		.phy_id		= 0x0283bc30,
		.phy_id_mask	= 0xffffffff,
		.drv.name	= "ADIN1300",
		.features	= PHY_GBIT_FEATURES,
		.config_init	= adin_config_init,
		.soft_reset	= adin_soft_reset,
		.config_aneg	= adin_config_aneg,
		.read_status	= genphy_read_status,
	},
};

device_phy_drivers(adin_driver);
