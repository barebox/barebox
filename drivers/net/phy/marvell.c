/*
 * drivers/net/phy/marvell.c
 *
 * Driver for Marvell PHYs based on Linux driver
 */

#include <common.h>
#include <init.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/smscphy.h>
#include <linux/marvell_phy.h>

/* Marvell Register Page register */
#define MII_MARVELL_PHY_PAGE		22
#define MII_MARVELL_PHY_DEFAULT_PAGE	0

#define MII_M1011_PHY_SCR		0x10
#define MII_M1011_PHY_SCR_AUTO_CROSS	0x0060

#define MII_88E1121_PHY_LED_CTRL	16
#define MII_88E1121_PHY_LED_PAGE	3

#define MII_M1011_PHY_STATUS		0x11
#define MII_M1011_PHY_STATUS_1000	BIT(15)
#define MII_M1011_PHY_STATUS_100	BIT(14)
#define MII_M1011_PHY_STATUS_SPD_MASK	\
	(MII_M1011_PHY_STATUS_1000 | MII_M1011_PHY_STATUS_100)
#define MII_M1011_PHY_STATUS_FULLDUPLEX	BIT(13)
#define MII_M1011_PHY_STATUS_RESOLVED	BIT(11)
#define MII_M1011_PHY_STATUS_LINK	BIT(10)

#define MII_M1111_PHY_LED_CONTROL	0x18
#define MII_M1111_PHY_LED_DIRECT	0x4100
#define MII_M1111_PHY_LED_COMBINE	0x411c
#define MII_M1111_PHY_EXT_CR		0x14
#define MII_M1111_RX_DELAY		0x80
#define MII_M1111_TX_DELAY		0x2
#define MII_M1111_PHY_EXT_SR		0x1b

#define MII_M1111_HWCFG_MODE_MASK		0xf
#define MII_M1111_HWCFG_MODE_COPPER_RGMII	0xb
#define MII_M1111_HWCFG_MODE_FIBER_RGMII	0x3
#define MII_M1111_HWCFG_MODE_SGMII_NO_CLK	0x4
#define MII_M1111_HWCFG_MODE_COPPER_RTBI	0x9
#define MII_M1111_HWCFG_FIBER_COPPER_AUTO	0x8000
#define MII_M1111_HWCFG_FIBER_COPPER_RES	0x2000

#define MII_M1111_COPPER		0
#define MII_M1111_FIBER			1

#define MII_88E1121_PHY_MSCR_PAGE	2
#define MII_88E1121_PHY_MSCR_REG	21
#define MII_88E1121_PHY_MSCR_TX_DELAY	BIT(4)
#define MII_88E1121_PHY_MSCR_RX_DELAY	BIT(5)
#define MII_88E1121_PHY_MSCR_DELAY_MASK	\
	(MII_88E1121_PHY_MSCR_RX_DELAY | MII_88E1121_PHY_MSCR_TX_DELAY)

#define MII_88E1318S_PHY_MSCR1_REG	16
#define MII_88E1318S_PHY_MSCR1_PAD_ODD	BIT(6)

#define MII_88E1540_QSGMII_PAGE		0x4
#define MII_88E1540_QSGMII_CONTROL	0x0
#define MII_88E1540_QSGMII_AUTONEG_EN	BIT(12)

#define MII_88E1510_GEN_CTRL_REG_1              0x14
#define MII_88E1510_GEN_CTRL_REG_1_MODE_MASK    0x7
#define MII_88E1510_GEN_CTRL_REG_1_MODE_SGMII   0x1     /* SGMII to copper */
#define MII_88E1510_GEN_CTRL_REG_1_RESET        0x8000  /* Soft reset */

#define LPA_PAUSE_FIBER	0x180
#define LPA_PAUSE_ASYM_FIBER	0x100

#define ADVERTISE_FIBER_1000HALF	0x40
#define ADVERTISE_FIBER_1000FULL	0x20

#define ADVERTISE_PAUSE_FIBER		0x180
#define ADVERTISE_PAUSE_ASYM_FIBER	0x100

/*
 * marvell_read_status
 *
 * Generic status code does not detect Fiber correctly!
 * Description:
 *   Check the link, then figure out the current state
 *   by comparing what we advertise with what the link partner
 *   advertises.  Start by checking the gigabit possibilities,
 *   then move on to 10/100.
 */
static int marvell_read_status(struct phy_device *phydev)
{
	int adv;
	int err;
	int lpa;
	int status = 0;

	/* Update the link, but return if there
	 * was an error */
	err = genphy_update_link(phydev);
	if (err)
		return err;

	if (AUTONEG_ENABLE == phydev->autoneg) {
		status = phy_read(phydev, MII_M1011_PHY_STATUS);
		if (status < 0)
			return status;

		lpa = phy_read(phydev, MII_LPA);
		if (lpa < 0)
			return lpa;

		adv = phy_read(phydev, MII_ADVERTISE);
		if (adv < 0)
			return adv;

		lpa &= adv;

		if (status & MII_M1011_PHY_STATUS_FULLDUPLEX)
			phydev->duplex = DUPLEX_FULL;
		else
			phydev->duplex = DUPLEX_HALF;

		status = status & MII_M1011_PHY_STATUS_SPD_MASK;
		phydev->pause = phydev->asym_pause = 0;

		switch (status) {
		case MII_M1011_PHY_STATUS_1000:
			phydev->speed = SPEED_1000;
			break;

		case MII_M1011_PHY_STATUS_100:
			phydev->speed = SPEED_100;
			break;

		default:
			phydev->speed = SPEED_10;
			break;
		}

		if (phydev->duplex == DUPLEX_FULL) {
			phydev->pause = lpa & LPA_PAUSE_CAP ? 1 : 0;
			phydev->asym_pause = lpa & LPA_PAUSE_ASYM ? 1 : 0;
		}
	} else {
		int bmcr = phy_read(phydev, MII_BMCR);

		if (bmcr < 0)
			return bmcr;

		if (bmcr & BMCR_FULLDPLX)
			phydev->duplex = DUPLEX_FULL;
		else
			phydev->duplex = DUPLEX_HALF;

		if (bmcr & BMCR_SPEED1000)
			phydev->speed = SPEED_1000;
		else if (bmcr & BMCR_SPEED100)
			phydev->speed = SPEED_100;
		else
			phydev->speed = SPEED_10;

		phydev->pause = phydev->asym_pause = 0;
	}

	return 0;
}

#define MII_88E1510_GEN_CTRL_REG_1              0x14

static inline bool phy_interface_is_rgmii(struct phy_device *phydev)
{
	return phydev->interface >= PHY_INTERFACE_MODE_RGMII &&
		phydev->interface <= PHY_INTERFACE_MODE_RGMII_TXID;
};

/*
 * This same function in the Linux kernel parses the marvell,reg-init dt
 * property and does the necessary register writes. It's kept as an exercise for
 * a future user to implement this. :-)
 */
static int marvell_of_reg_init(struct phy_device *phydev)
{
	return 0;
}

static int m88e1121_config_aneg(struct phy_device *phydev)
{
	int err, oldpage, mscr;

	oldpage = phy_read(phydev, MII_MARVELL_PHY_PAGE);

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE,
			MII_88E1121_PHY_MSCR_PAGE);
	if (err < 0)
		return err;

	if (phy_interface_is_rgmii(phydev)) {

		mscr = phy_read(phydev, MII_88E1121_PHY_MSCR_REG) &
			MII_88E1121_PHY_MSCR_DELAY_MASK;

		if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID)
			mscr |= (MII_88E1121_PHY_MSCR_RX_DELAY |
				 MII_88E1121_PHY_MSCR_TX_DELAY);
		else if (phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)
			mscr |= MII_88E1121_PHY_MSCR_RX_DELAY;
		else if (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
			mscr |= MII_88E1121_PHY_MSCR_TX_DELAY;

		err = phy_write(phydev, MII_88E1121_PHY_MSCR_REG, mscr);
		if (err < 0)
			return err;
	}

	phy_write(phydev, MII_MARVELL_PHY_PAGE, oldpage);

	err = phy_write(phydev, MII_BMCR, BMCR_RESET);
	if (err < 0)
		return err;

	err = phy_write(phydev, MII_M1011_PHY_SCR,
			MII_M1011_PHY_SCR_AUTO_CROSS);
	if (err < 0)
		return err;

	return genphy_config_aneg(phydev);
}

static int m88e1318_config_aneg(struct phy_device *phydev)
{
	int err, oldpage, mscr;

	oldpage = phy_read(phydev, MII_MARVELL_PHY_PAGE);

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE,
			MII_88E1121_PHY_MSCR_PAGE);
	if (err < 0)
		return err;

	mscr = phy_read(phydev, MII_88E1318S_PHY_MSCR1_REG);
	mscr |= MII_88E1318S_PHY_MSCR1_PAD_ODD;

	err = phy_write(phydev, MII_88E1318S_PHY_MSCR1_REG, mscr);
	if (err < 0)
		return err;

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE, oldpage);
	if (err < 0)
		return err;

	return m88e1121_config_aneg(phydev);
}

/**
 * ethtool_adv_to_fiber_adv_t
 * @ethadv: the ethtool advertisement settings
 *
 * A small helper function that translates ethtool advertisement
 * settings to phy autonegotiation advertisements for the
 * MII_ADV register for fiber link.
 */
static inline u32 ethtool_adv_to_fiber_adv_t(u32 ethadv)
{
	u32 result = 0;

	if (ethadv & ADVERTISED_1000baseT_Half)
		result |= ADVERTISE_FIBER_1000HALF;
	if (ethadv & ADVERTISED_1000baseT_Full)
		result |= ADVERTISE_FIBER_1000FULL;

	if ((ethadv & ADVERTISE_PAUSE_ASYM) && (ethadv & ADVERTISE_PAUSE_CAP))
		result |= LPA_PAUSE_ASYM_FIBER;
	else if (ethadv & ADVERTISE_PAUSE_CAP)
		result |= (ADVERTISE_PAUSE_FIBER
			   & (~ADVERTISE_PAUSE_ASYM_FIBER));

	return result;
}

/**
 * marvell_config_aneg_fiber - restart auto-negotiation or write BMCR
 * @phydev: target phy_device struct
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   advertising, and then restart auto-negotiation.  If it is not
 *   enabled, then we write the BMCR. Adapted for fiber link in
 *   some Marvell's devices.
 */
static int marvell_config_aneg_fiber(struct phy_device *phydev)
{
	int changed = 0;
	int err;
	int adv, oldadv;
	u32 advertise;

	if (phydev->autoneg != AUTONEG_ENABLE)
		return genphy_setup_forced(phydev);

	/* Only allow advertising what this PHY supports */
	phydev->advertising &= phydev->supported;
	advertise = phydev->advertising;

	/* Setup fiber advertisement */
	adv = phy_read(phydev, MII_ADVERTISE);
	if (adv < 0)
		return adv;

	oldadv = adv;
	adv &= ~(ADVERTISE_FIBER_1000HALF | ADVERTISE_FIBER_1000FULL
		| LPA_PAUSE_FIBER);
	adv |= ethtool_adv_to_fiber_adv_t(advertise);

	if (adv != oldadv) {
		err = phy_write(phydev, MII_ADVERTISE, adv);
		if (err < 0)
			return err;

		changed = 1;
	}

	if (changed == 0) {
		/* Advertisement hasn't changed, but maybe aneg was never on to
		 * begin with?  Or maybe phy was isolated?
		 */
		int ctl = phy_read(phydev, MII_BMCR);

		if (ctl < 0)
			return ctl;

		if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
			changed = 1; /* do restart aneg */
	}

	/* Only restart aneg if we are advertising something different
	 * than we were before.
	 */
	if (changed > 0)
		changed = genphy_restart_aneg(phydev);

	return changed;
}

static int m88e1510_config_aneg(struct phy_device *phydev)
{
	int err;

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE, MII_M1111_COPPER);
	if (err < 0)
		goto error;

	/* Configure the copper link first */
	err = m88e1318_config_aneg(phydev);
	if (err < 0)
		goto error;

	/* Then the fiber link */
	err = phy_write(phydev, MII_MARVELL_PHY_PAGE, MII_M1111_FIBER);
	if (err < 0)
		goto error;

	err = marvell_config_aneg_fiber(phydev);
	if (err < 0)
		goto error;

	return phy_write(phydev, MII_MARVELL_PHY_PAGE, MII_M1111_COPPER);

error:
	phy_write(phydev, MII_MARVELL_PHY_PAGE, MII_M1111_COPPER);
	return err;
}

static int marvell_config_init(struct phy_device *phydev)
{
	/* Set registers from marvell,reg-init DT property */
	return marvell_of_reg_init(phydev);
}

static int m88e1540_config_init(struct phy_device *phydev)
{
	u16 reg;
	int ret;

	/* Configure QSGMII auto-negotiation */
	if (phydev->interface == PHY_INTERFACE_MODE_QSGMII) {
		ret = phy_write(phydev, MII_MARVELL_PHY_PAGE,
				MII_88E1540_QSGMII_PAGE);
		if (ret < 0)
			return ret;

		reg = phy_read(phydev, MII_88E1540_QSGMII_CONTROL);
		ret = phy_write(phydev, MII_88E1540_QSGMII_CONTROL,
					reg | MII_88E1540_QSGMII_AUTONEG_EN);
		if (ret < 0)
			return ret;
	}

	/* Configure LED as:
	 * Activity: Blink
	 * Link:     On
	 * No Link:  Off
	 */
	phy_write(phydev, MII_MARVELL_PHY_PAGE, MII_88E1121_PHY_LED_PAGE);
	phy_write(phydev, MII_88E1121_PHY_LED_CTRL, 0x1111);

	/* Power-up the PHY. When going from power down to normal operation,
	 * software reset and auto-negotiation restart are also performed.
	 */
	ret = phy_write(phydev, MII_MARVELL_PHY_PAGE,
				MII_MARVELL_PHY_DEFAULT_PAGE);
	if (ret < 0)
		return ret;
	ret = phy_write(phydev, MII_BMCR,
			phy_read(phydev, MII_BMCR) & ~BMCR_PDOWN);
	if (ret < 0)
		return ret;

	return marvell_config_init(phydev);
}

static int m88e1111_config_init(struct phy_device *phydev)
{
	int err;
	int temp;

	if ((phydev->interface == PHY_INTERFACE_MODE_RGMII) ||
	    (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID) ||
	    (phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID) ||
	    (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)) {

		temp = phy_read(phydev, MII_M1111_PHY_EXT_CR);
		if (temp < 0)
			return temp;

		if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID) {
			temp |= (MII_M1111_RX_DELAY | MII_M1111_TX_DELAY);
		} else if (phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID) {
			temp &= ~MII_M1111_TX_DELAY;
			temp |= MII_M1111_RX_DELAY;
		} else if (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID) {
			temp &= ~MII_M1111_RX_DELAY;
			temp |= MII_M1111_TX_DELAY;
		}

		err = phy_write(phydev, MII_M1111_PHY_EXT_CR, temp);
		if (err < 0)
			return err;

		temp = phy_read(phydev, MII_M1111_PHY_EXT_SR);
		if (temp < 0)
			return temp;

		temp &= ~(MII_M1111_HWCFG_MODE_MASK);

		if (temp & MII_M1111_HWCFG_FIBER_COPPER_RES)
			temp |= MII_M1111_HWCFG_MODE_FIBER_RGMII;
		else
			temp |= MII_M1111_HWCFG_MODE_COPPER_RGMII;

		err = phy_write(phydev, MII_M1111_PHY_EXT_SR, temp);
		if (err < 0)
			return err;
	}

	if (phydev->interface == PHY_INTERFACE_MODE_SGMII) {
		temp = phy_read(phydev, MII_M1111_PHY_EXT_SR);
		if (temp < 0)
			return temp;

		temp &= ~(MII_M1111_HWCFG_MODE_MASK);
		temp |= MII_M1111_HWCFG_MODE_SGMII_NO_CLK;
		temp |= MII_M1111_HWCFG_FIBER_COPPER_AUTO;

		err = phy_write(phydev, MII_M1111_PHY_EXT_SR, temp);
		if (err < 0)
			return err;
	}

	if (phydev->interface == PHY_INTERFACE_MODE_RTBI) {
		temp = phy_read(phydev, MII_M1111_PHY_EXT_CR);
		if (temp < 0)
			return temp;
		temp |= (MII_M1111_RX_DELAY | MII_M1111_TX_DELAY);
		err = phy_write(phydev, MII_M1111_PHY_EXT_CR, temp);
		if (err < 0)
			return err;

		temp = phy_read(phydev, MII_M1111_PHY_EXT_SR);
		if (temp < 0)
			return temp;
		temp &= ~(MII_M1111_HWCFG_MODE_MASK | MII_M1111_HWCFG_FIBER_COPPER_RES);
		temp |= 0x7 | MII_M1111_HWCFG_FIBER_COPPER_AUTO;
		err = phy_write(phydev, MII_M1111_PHY_EXT_SR, temp);
		if (err < 0)
			return err;

		/* soft reset */
		err = phy_write(phydev, MII_BMCR, BMCR_RESET);
		if (err < 0)
			return err;
		do
			temp = phy_read(phydev, MII_BMCR);
		while (temp & BMCR_RESET);

		temp = phy_read(phydev, MII_M1111_PHY_EXT_SR);
		if (temp < 0)
			return temp;
		temp &= ~(MII_M1111_HWCFG_MODE_MASK | MII_M1111_HWCFG_FIBER_COPPER_RES);
		temp |= MII_M1111_HWCFG_MODE_COPPER_RTBI | MII_M1111_HWCFG_FIBER_COPPER_AUTO;
		err = phy_write(phydev, MII_M1111_PHY_EXT_SR, temp);
		if (err < 0)
			return err;
	}

	err = marvell_of_reg_init(phydev);
	if (err < 0)
		return err;

	return phy_write(phydev, MII_BMCR, BMCR_RESET);
}

static int m88e1121_config_init(struct phy_device *phydev)
{
	u16 reg;
	int ret;

	ret = phy_write(phydev, MII_MARVELL_PHY_PAGE,
			MII_88E1121_PHY_MSCR_PAGE);
	if (ret < 0)
		return ret;

	/* Setup RGMII TX/RX delay */
	reg = phy_read(phydev, MII_88E1121_PHY_MSCR_REG) &
		~MII_88E1121_PHY_MSCR_DELAY_MASK;
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)
		reg |= MII_88E1121_PHY_MSCR_RX_DELAY;
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
		reg |= MII_88E1121_PHY_MSCR_TX_DELAY;
	ret = phy_write(phydev, MII_88E1121_PHY_MSCR_REG, reg);
	if (ret < 0)
		return ret;

	ret = phy_write(phydev, MII_MARVELL_PHY_PAGE,
			MII_MARVELL_PHY_DEFAULT_PAGE);
	if (ret < 0)
		return ret;

	/* Enable auto-crossover */
	ret = phy_write(phydev, MII_M1011_PHY_SCR,
			MII_M1011_PHY_SCR_AUTO_CROSS);
	if (ret < 0)
		return ret;

	/* Reset PHY */
	ret = phy_write(phydev, MII_BMCR,
			phy_read(phydev, MII_BMCR) | BMCR_RESET);
	if (ret < 0)
		return ret;

	return marvell_config_init(phydev);
}


static int m88e1318s_config_init(struct phy_device *phydev)
{
	u16 reg;
	int ret;

	ret = phy_write(phydev, MII_MARVELL_PHY_PAGE,
			MII_88E1121_PHY_MSCR_PAGE);
	if (ret < 0)
		return ret;

	reg = phy_read(phydev, MII_88E1318S_PHY_MSCR1_REG);
	reg |= MII_88E1318S_PHY_MSCR1_PAD_ODD;
	ret = phy_write(phydev, MII_88E1318S_PHY_MSCR1_REG, reg);
	if (ret < 0)
		return ret;

	return m88e1121_config_init(phydev);
}

static int m88e1510_config_init(struct phy_device *phydev)
{
	int err;
	int temp;

	/* SGMII-to-Copper mode initialization */
	if (phydev->interface == PHY_INTERFACE_MODE_SGMII) {
		/* Select page 18 */
		err = phy_write(phydev, MII_MARVELL_PHY_PAGE, 18);
		if (err < 0)
			return err;

		/* In reg 20, write MODE[2:0] = 0x1 (SGMII to Copper) */
		temp = phy_read(phydev, MII_88E1510_GEN_CTRL_REG_1);
		temp &= ~MII_88E1510_GEN_CTRL_REG_1_MODE_MASK;
		temp |= MII_88E1510_GEN_CTRL_REG_1_MODE_SGMII;
		err = phy_write(phydev, MII_88E1510_GEN_CTRL_REG_1, temp);
		if (err < 0)
			return err;

		/* PHY reset is necessary after changing MODE[2:0] */
		temp |= MII_88E1510_GEN_CTRL_REG_1_RESET;
		err = phy_write(phydev, MII_88E1510_GEN_CTRL_REG_1, temp);
		if (err < 0)
			return err;

		/* Reset page selection */
		err = phy_write(phydev, MII_MARVELL_PHY_PAGE, 0);
		if (err < 0)
			return err;
	}

	return m88e1121_config_init(phydev);
}

static struct phy_driver marvell_drivers[] = {
	{
		.phy_id = MARVELL_PHY_ID_88E1111,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.drv.name = "Marvell 88E1111",
		.features = PHY_GBIT_FEATURES,
		.config_init = &m88e1111_config_init,
		.config_aneg = &genphy_config_aneg,
		.read_status = &marvell_read_status,
	},
	{
		.phy_id = MARVELL_PHY_ID_88E1121R,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.drv.name = "Marvell 88E1121R",
		.features = PHY_GBIT_FEATURES,
		.config_init = &m88e1121_config_init,
		.config_aneg = &m88e1121_config_aneg,
		.read_status = &marvell_read_status,
	},
	{
		.phy_id = MARVELL_PHY_ID_88E1318S,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.drv.name = "Marvell 88E1318S",
		.features = PHY_GBIT_FEATURES,
		.config_init = &m88e1318s_config_init,
		.config_aneg = &m88e1318_config_aneg,
		.read_status = &marvell_read_status,
	},
	{
		.phy_id = MARVELL_PHY_ID_88E1543,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.drv.name = "Marvell 88E1543",
		.features = PHY_GBIT_FEATURES,
		.config_init = &m88e1540_config_init,
		.config_aneg = &genphy_config_aneg,
		.read_status = &marvell_read_status,
	},
	{
		.phy_id = MARVELL_PHY_ID_88E1510,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.drv.name = "Marvell 88E1510",
		.features = PHY_GBIT_FEATURES | SUPPORTED_FIBRE,
		.config_init = &m88e1510_config_init,
		.config_aneg = &m88e1510_config_aneg,
		.read_status = &marvell_read_status,
	},
	{
		.phy_id = MARVELL_PHY_ID_88E1540,
		.phy_id_mask = MARVELL_PHY_ID_MASK,
		.drv.name = "Marvell 88E1540",
		.features = PHY_GBIT_FEATURES,
		.config_init = &m88e1540_config_init,
		.config_aneg = &m88e1510_config_aneg,
		.read_status = &marvell_read_status,
	},
};

static int __init marvell_phy_init(void)
{
	return phy_drivers_register(marvell_drivers,
				    ARRAY_SIZE(marvell_drivers));
}
fs_initcall(marvell_phy_init);
