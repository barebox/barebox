/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/string.h>

#define ATHR_PHY_MAX	5

/*****************/
/* PHY Registers */
/*****************/
#define ATHR_PHY_CONTROL			0x00
#define ATHR_PHY_STATUS				0x01
#define ATHR_PHY_ID1				0x02
#define ATHR_PHY_ID2				0x03
#define ATHR_AUTONEG_ADVERT			0x04
#define ATHR_LINK_PARTNER_ABILITY		0x05
#define ATHR_AUTONEG_EXPANSION			0x06
#define ATHR_NEXT_PAGE_TRANSMIT			0x07
#define ATHR_LINK_PARTNER_NEXT_PAGE		0x08
#define ATHR_1000BASET_CONTROL			0x09
#define ATHR_1000BASET_STATUS			0x0a
#define ATHR_PHY_SPEC_CONTROL			0x10
#define ATHR_PHY_SPEC_STATUS			0x11
#define ATHR_DEBUG_PORT_ADDRESS			0x1d
#define ATHR_DEBUG_PORT_DATA			0x1e

/* Advertisement register. */
#define ATHR_ADVERTISE_ASYM_PAUSE		0x0800
#define ATHR_ADVERTISE_PAUSE			0x0400
#define ATHR_ADVERTISE_100FULL			0x0100
#define ATHR_ADVERTISE_100HALF			0x0080
#define ATHR_ADVERTISE_10FULL			0x0040
#define ATHR_ADVERTISE_10HALF			0x0020

#define ATHR_ADVERTISE_ALL (ATHR_ADVERTISE_ASYM_PAUSE | ATHR_ADVERTISE_PAUSE | \
                            ATHR_ADVERTISE_10HALF | ATHR_ADVERTISE_10FULL | \
                            ATHR_ADVERTISE_100HALF | ATHR_ADVERTISE_100FULL)

/* ATHR_PHY_CONTROL fields */
#define ATHR_CTRL_SOFTWARE_RESET		0x8000
#define ATHR_CTRL_AUTONEGOTIATION_ENABLE	0x1000

/* 1000BASET_CONTROL */
#define ATHR_ADVERTISE_1000FULL			0x0200

/* Phy Specific status fields */
#define ATHR_STATUS_LINK_PASS			0x0400

static u32 ar8327n_reg_read(struct phy_device *phydev, u32 reg_addr)
{
	u32 reg_word_addr;
	u32 phy_addr, tmp_val, reg_val;
	u16 phy_val;
	u8 phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (u16) ((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);

	/* For some registers such as MIBs, since it is read/clear, we should */
	/* read the lower 16-bit register then the higher one */

	/* read register in lower address */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (u8) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	reg_val = (u32) mdiobus_read(phydev->bus, phy_addr, phy_reg);

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (u8) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	reg_val = (u32) mdiobus_read(phydev->bus, phy_addr, phy_reg);
	reg_val |= (tmp_val << 16);

	return reg_val;
}

static void ar8327n_reg_write(struct phy_device *phydev, u32 reg_addr,
			      u32 reg_val)
{
	u32 reg_word_addr;
	u32 phy_addr;
	u16 phy_val;
	u8 phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (u16) ((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);

	/* For some registers such as ARL and VLAN, since they include BUSY bit */
	/* in lower address, we should write the higher 16-bit register then the */
	/* lower one */

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (u8) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (u16) ((reg_val >> 16) & 0xffff);
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);

	/* write register in lower address */
	reg_word_addr--;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (u8) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (u16) (reg_val & 0xffff);
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);
}

static int ar8327n_phy_is_link_alive(struct phy_device *phydev, int phy_addr)
{
	u16 val;

	val = mdiobus_read(phydev->bus, phy_addr, ATHR_PHY_SPEC_STATUS);

	return !!(val & ATHR_STATUS_LINK_PASS);
}

static int ar8327n_phy_setup(struct phy_device *phydev)
{
	struct device_d *dev = &phydev->dev;
	int phy_addr;

	/* start auto negotiation on each phy */
	for (phy_addr = 0; phy_addr < ATHR_PHY_MAX; phy_addr++) {
		mdiobus_write(phydev->bus, phy_addr, ATHR_AUTONEG_ADVERT,
			      ATHR_ADVERTISE_ALL);

		mdiobus_write(phydev->bus, phy_addr, ATHR_1000BASET_CONTROL,
			      ATHR_ADVERTISE_1000FULL);

		/* Reset PHYs*/
		mdiobus_write(phydev->bus, phy_addr, ATHR_PHY_CONTROL,
			      ATHR_CTRL_AUTONEGOTIATION_ENABLE
			      | ATHR_CTRL_SOFTWARE_RESET);
	}

	/*
	 * After the phy is reset, it takes a little while before
	 * it can respond properly.
	 */
	mdelay(1000);

	for (phy_addr = 0; phy_addr < ATHR_PHY_MAX; phy_addr++) {
		int count;

		for (count = 20; count > 0; count--) {
			u16 val;
			val = mdiobus_read(phydev->bus, phy_addr,
					   ATHR_PHY_CONTROL);

			if (!(val & ATHR_CTRL_SOFTWARE_RESET))
				break;

			mdelay(150);
		}

		if (!count) {
			dev_err(dev, "error: port %d, negotiation timeout.\n",
				phy_addr);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int ar8327n_get_link(struct phy_device *phydev)
{
	int phy_addr;
	int live_links = 0;

	for (phy_addr = 0; phy_addr < ATHR_PHY_MAX; phy_addr++) {
		if (ar8327n_phy_is_link_alive(phydev, phy_addr))
			live_links++;
	}

	return (live_links > 0);
}

static int ar8327n_config_init(struct phy_device *phydev)
{
	struct device_d *dev = &phydev->dev;
	int phy_addr = 0;

	if (phydev->interface != PHY_INTERFACE_MODE_RGMII_TXID)
		return 0;

	/* if using header for register configuration, we have to     */
	/* configure s17 register after frame transmission is enabled */

	/* configure the RGMII */
	ar8327n_reg_write(phydev, 0x624, 0x7f7f7f7f);
	ar8327n_reg_write(phydev, 0x10, 0x40000000);
	ar8327n_reg_write(phydev, 0x4, 0x07600000);
	ar8327n_reg_write(phydev, 0xc, 0x01000000);
	ar8327n_reg_write(phydev, 0x7c, 0x0000007e);

	/* AR8327/AR8328 v1.0 fixup */
	if ((ar8327n_reg_read(phydev, 0x0) & 0xffff) == 0x1201) {
		dev_warn(dev, "warning: untested device. PHY v1.0\n");
		for (phy_addr = 0x0; phy_addr <= ATHR_PHY_MAX; phy_addr++) {
			/* For 100M waveform */
			mdiobus_write(phydev->bus, phy_addr, 0x1d, 0x0);
			mdiobus_write(phydev->bus, phy_addr, 0x1e, 0x02ea);
			/* Turn On Gigabit Clock */
			mdiobus_write(phydev->bus, phy_addr, 0x1d, 0x3d);
			mdiobus_write(phydev->bus, phy_addr, 0x1e, 0x68a0);
		}
	}

	/*
	 * set the WAN Port(Port1) Disable Mode so
	 * it can not receive or transmit any frames.
	 */
	ar8327n_reg_write(phydev, 0x066c,
			  ar8327n_reg_read(phydev, 0x066c) & 0xfff8ffff);

	ar8327n_phy_setup(phydev);

	return 0;
}

static int ar8327n_read_status(struct phy_device *phydev)
{
	/* for GMAC0 we have only one static mode */
	phydev->speed = SPEED_1000;
	phydev->duplex = DUPLEX_FULL;
	phydev->pause = phydev->asym_pause = 0;
	phydev->link = ar8327n_get_link(phydev);
	return 0;
}

static int ar8327n_config_aneg(struct phy_device *phydev)
{
	return 0;
}

static int ar8327n_aneg_done(struct phy_device *phydev)
{
	return BMSR_ANEGCOMPLETE;
}

static struct phy_driver ar8327n_driver[] = {
{
	/* QCA AR8327N */
	.phy_id		= 0x004dd034,
	.phy_id_mask	= 0xffffffef,
	.drv.name	= "QCA AR8327N switch",
	.config_init	= ar8327n_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &ar8327n_config_aneg,
	.read_status	= &ar8327n_read_status,
	.aneg_done	= &ar8327n_aneg_done,
}};

static int atheros_phy_init(void)
{
	return phy_drivers_register(ar8327n_driver,
				    ARRAY_SIZE(ar8327n_driver));
}
fs_initcall(atheros_phy_init);
