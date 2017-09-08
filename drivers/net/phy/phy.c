/*
 * drivers/net/phy/phy.c
 *
 * Framework for finding and configuring PHYs.
 * Also contains generic PHY driver
 *
 * Copyright (c) 2009-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <net.h>
#include <malloc.h>
#include <linux/phy.h>
#include <linux/phy.h>
#include <linux/err.h>

#define PHY_AN_TIMEOUT	10

static struct phy_driver genphy_driver;

/**
 * phy_aneg_done - return auto-negotiation status
 * @phydev: target phy_device struct
 *
 * Description: Return the auto-negotiation status from this @phydev
 * Returns > 0 on success or < 0 on error. 0 means that auto-negotiation
 * is still pending.
 */
static int phy_aneg_done(struct phy_device *phydev)
{
	struct phy_driver *drv = to_phy_driver(phydev->dev.driver);

	return drv->aneg_done(phydev);
}

int phy_update_status(struct phy_device *phydev)
{
	struct phy_driver *drv = to_phy_driver(phydev->dev.driver);
	struct eth_device *edev = phydev->attached_dev;
	int ret;
	int oldspeed = phydev->speed, oldduplex = phydev->duplex;

	if (drv) {
		ret = drv->read_status(phydev);
		if (ret)
			return ret;
	}

	if (phydev->speed == oldspeed && phydev->duplex == oldduplex)
		return 0;

	if (phydev->adjust_link)
		phydev->adjust_link(edev);

	if (phydev->link)
		dev_info(&edev->dev, "%dMbps %s duplex link detected\n",
				phydev->speed, phydev->duplex ? "full" : "half");

	return 0;
}

static LIST_HEAD(phy_fixup_list);

/*
 * Creates a new phy_fixup and adds it to the list
 * @bus_id: A string which matches phydev->dev.bus_id (or PHY_ANY_ID)
 * @phy_uid: Used to match against phydev->phy_id (the UID of the PHY)
 * 	It can also be PHY_ANY_UID
 * @phy_uid_mask: Applied to phydev->phy_id and fixup->phy_uid before
 * 	comparison
 * @run: The actual code to be run when a matching PHY is found
 */
int phy_register_fixup(const char *bus_id, u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *))
{
	struct phy_fixup *fixup;

	fixup = xzalloc(sizeof(struct phy_fixup));

	strlcpy(fixup->bus_id, bus_id, sizeof(fixup->bus_id));
	fixup->phy_uid = phy_uid;
	fixup->phy_uid_mask = phy_uid_mask;
	fixup->run = run;

	list_add_tail(&fixup->list, &phy_fixup_list);

	return 0;
}

/* Registers a fixup to be run on any PHY with the UID in phy_uid */
int phy_register_fixup_for_uid(u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *))
{
	return phy_register_fixup(PHY_ANY_ID, phy_uid, phy_uid_mask, run);
}

/* Registers a fixup to be run on the PHY with id string bus_id */
int phy_register_fixup_for_id(const char *bus_id,
		int (*run)(struct phy_device *))
{
	return phy_register_fixup(bus_id, PHY_ANY_UID, 0xffffffff, run);
}

/*
 * Returns 1 if fixup matches phydev in bus_id and phy_uid.
 * Fixups can be set to match any in one or more fields.
 */
static int phy_needs_fixup(struct phy_device *phydev, struct phy_fixup *fixup)
{
	if (strcmp(fixup->bus_id, dev_name(&phydev->dev)) != 0)
		if (strcmp(fixup->bus_id, PHY_ANY_ID) != 0)
			return 0;

	if ((fixup->phy_uid & fixup->phy_uid_mask) !=
			(phydev->phy_id & fixup->phy_uid_mask))
		if (fixup->phy_uid != PHY_ANY_UID)
			return 0;

	return 1;
}
/* Runs any matching fixups for this phydev */
int phy_scan_fixups(struct phy_device *phydev)
{
	struct phy_fixup *fixup;

	list_for_each_entry(fixup, &phy_fixup_list, list) {
		if (phy_needs_fixup(phydev, fixup)) {
			int err;

			err = fixup->run(phydev);

			if (err < 0) {
				return err;
			}
		}
	}

	return 0;
}
/**
 * phy_device_create - creates a struct phy_device.
 * @bus: the target MII bus
 * @addr: PHY address on the MII bus
 * @phy_id: PHY ID.
 */
struct phy_device *phy_device_create(struct mii_bus *bus, int addr, int phy_id)
{
	struct phy_device *phydev;

	/* We allocate the device, and initialize the
	 * default values */
	phydev = xzalloc(sizeof(*phydev));

	phydev->speed = 0;
	phydev->duplex = -1;
	phydev->pause = phydev->asym_pause = 0;
	phydev->link = 1;
	phydev->autoneg = AUTONEG_ENABLE;

	phydev->addr = addr;
	phydev->phy_id = phy_id;

	phydev->bus = bus;
	phydev->dev.bus = &mdio_bus_type;

	if (bus) {
		sprintf(phydev->dev.name, "mdio%d-phy%02x",
				   phydev->bus->dev.id,
				   phydev->addr);
		phydev->dev.id = DEVICE_ID_SINGLE;
	} else {
		sprintf(phydev->dev.name, "fixed-phy");
		phydev->dev.id = DEVICE_ID_DYNAMIC;
	}

	return phydev;
}
/**
 * get_phy_id - reads the specified addr for its ID.
 * @bus: the target MII bus
 * @addr: PHY address on the MII bus
 * @phy_id: where to store the ID retrieved.
 *
 * Description: Reads the ID registers of the PHY at @addr on the
 *   @bus, stores it in @phy_id and returns zero on success.
 */
int get_phy_id(struct mii_bus *bus, int addr, u32 *phy_id)
{
	int phy_reg;

	/* Grab the bits from PHYIR1, and put them
	 * in the upper half */
	phy_reg = mdiobus_read(bus, addr, MII_PHYSID1);

	if (phy_reg < 0)
		return -EIO;

	*phy_id = (phy_reg & 0xffff) << 16;

	/* Grab the bits from PHYIR2, and put them in the lower half */
	phy_reg = mdiobus_read(bus, addr, MII_PHYSID2);

	if (phy_reg < 0)
		return -EIO;

	*phy_id |= (phy_reg & 0xffff);

	return 0;
}

/**
 * get_phy_device - reads the specified PHY device and returns its @phy_device struct
 * @bus: the target MII bus
 * @addr: PHY address on the MII bus
 *
 * Description: Reads the ID registers of the PHY at @addr on the
 *   @bus, then allocates and returns the phy_device to represent it.
 */
struct phy_device *get_phy_device(struct mii_bus *bus, int addr)
{
	struct phy_device *phydev = NULL;
	u32 phy_id = 0;
	int r;

	r = get_phy_id(bus, addr, &phy_id);
	if (r)
		return ERR_PTR(r);

	/* If the phy_id is mostly Fs, there is no device there */
	if ((phy_id & 0x1fffffff) == 0x1fffffff)
		return ERR_PTR(-ENODEV);

	phydev = phy_device_create(bus, addr, phy_id);

	return phydev;
}

static void phy_config_aneg(struct phy_device *phydev)
{
	struct phy_driver *drv;

	drv = to_phy_driver(phydev->dev.driver);

	if (drv)
		drv->config_aneg(phydev);
}

int phy_register_device(struct phy_device *phydev)
{
	int ret;

	if (phydev->registered)
		return -EBUSY;

	if (!phydev->dev.parent)
		phydev->dev.parent = &phydev->bus->dev;

	ret = register_device(&phydev->dev);
	if (ret)
		return ret;

	if (phydev->bus)
		phydev->bus->phy_map[phydev->addr] = phydev;

	phydev->registered = 1;

	if (phydev->dev.driver)
		return 0;

	phydev->dev.driver = &genphy_driver.drv;
	ret = device_probe(&phydev->dev);
	if (ret) {
		unregister_device(&phydev->dev);
		phydev->registered = 0;
	}

	return ret;
}

void phy_unregister_device(struct phy_device *phydev)
{
	if (!phydev->registered)
		return;

	phydev->bus->phy_map[phydev->addr] = NULL;

	unregister_device(&phydev->dev);
	phydev->registered = 0;
}

static struct phy_device *of_phy_register_fixed_link(struct device_node *np,
						struct eth_device *edev)
{
	struct phy_device *phydev;

	phydev = phy_device_create(NULL, 0, 0);

	phydev->dev.parent = &edev->dev;
	phydev->registered = 1;
	phydev->speed = 1000;
	phydev->duplex = 1;

	return phydev;
}

static struct phy_device *of_mdio_find_phy(struct eth_device *edev)
{
	struct device_d *dev;
	struct device_node *phy_node;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return NULL;

	if (!edev->parent || !edev->parent->device_node)
		return NULL;

	phy_node = of_parse_phandle(edev->parent->device_node, "phy-handle", 0);
	if (!phy_node)
		phy_node = of_parse_phandle(edev->parent->device_node, "phy", 0);
	if (!phy_node)
		phy_node = of_parse_phandle(edev->parent->device_node, "phy-device", 0);
	if (!phy_node) {
		phy_node = of_get_child_by_name(edev->parent->device_node, "fixed-link");
		if (phy_node)
			return of_phy_register_fixed_link(phy_node, edev);
	}

	if (!phy_node)
		return NULL;

	bus_for_each_device(&mdio_bus_type, dev) {
		if (dev->device_node == phy_node)
			return container_of(dev, struct phy_device, dev);
	}

	return NULL;
}

static int phy_device_attach(struct phy_device *phy, struct eth_device *edev,
		       void (*adjust_link) (struct eth_device *edev),
		       u32 flags, phy_interface_t interface)
{
	int ret;

	if (phy->attached_dev)
		return -EBUSY;

	phy->interface = interface;
	phy->dev_flags = flags;

	if (!phy->registered) {
		ret = phy_register_device(phy);
		if (ret)
			return ret;
	}

	edev->phydev = phy;
	phy->attached_dev = edev;

	ret = phy_init_hw(phy);
	if (ret)
		return ret;

	/* Sanitize settings based on PHY capabilities */
	if ((phy->supported & SUPPORTED_Autoneg) == 0)
		phy->autoneg = AUTONEG_DISABLE;

	phy_config_aneg(edev->phydev);

	phy->adjust_link = adjust_link;

	return 0;
}

/* Automatically gets and returns the PHY device */
int phy_device_connect(struct eth_device *edev, struct mii_bus *bus, int addr,
		       void (*adjust_link) (struct eth_device *edev),
		       u32 flags, phy_interface_t interface)
{
	struct phy_device *phy;
	unsigned int i;
	int ret = -EINVAL;

	if (edev->phydev) {
		phy_config_aneg(edev->phydev);
		return 0;
	}

	phy = of_mdio_find_phy(edev);
	if (phy) {
		ret = phy_device_attach(phy, edev, adjust_link, flags, interface);

		goto out;
	}

	if (!bus) {
		ret = -ENODEV;
		goto out;
	}

	if (addr >= 0) {
		phy = mdiobus_scan(bus, addr);
		if (IS_ERR(phy)) {
			ret = -EIO;
			goto out;
		}

		ret = phy_device_attach(phy, edev, adjust_link, flags, interface);

		goto out;
	}

	for (i = 0; i < PHY_MAX_ADDR && !edev->phydev; i++) {
		/* skip masked out PHY addresses */
		if (bus->phy_mask & (1 << i))
			continue;

		phy = mdiobus_scan(bus, i);
		if (IS_ERR(phy))
			continue;

		ret = phy_device_attach(phy, edev, adjust_link, flags, interface);

		goto out;
	}

	ret = -ENODEV;
out:
	if (ret)
		puts("Unable to find a PHY (unknown ID?)\n");

	return ret;
}

/* Generic PHY support and helper functions */

/**
 * genphy_config_advert - sanitize and advertise auto-negotiation parameters
 * @phydev: target phy_device struct
 *
 * Description: Writes MII_ADVERTISE with the appropriate values,
 *   after sanitizing the values to make sure we only advertise
 *   what is supported.  Returns < 0 on error, 0 if the PHY's advertisement
 *   hasn't changed, and > 0 if it has changed.
 */
int genphy_config_advert(struct phy_device *phydev)
{
	u32 advertise;
	int oldadv, adv, bmsr;
	int err, changed = 0;

	/* Only allow advertising what
	 * this PHY supports */
	phydev->advertising &= phydev->supported;
	advertise = phydev->advertising;

	/* Setup standard advertisement */
	oldadv = adv = phy_read(phydev, MII_ADVERTISE);

	if (adv < 0)
		return adv;

	adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP |
		 ADVERTISE_PAUSE_ASYM);
	adv |= ethtool_adv_to_mii_adv_t(advertise);

	if (adv != oldadv) {
		err = phy_write(phydev, MII_ADVERTISE, adv);

		if (err < 0)
			return err;
		changed = 1;
	}

	/* Configure gigabit if it's supported */
	bmsr = phy_read(phydev, MII_BMSR);
	if (bmsr < 0)
		return bmsr;

	if (bmsr & BMSR_ESTATEN) {
		oldadv = adv = phy_read(phydev, MII_CTRL1000);

		if (adv < 0)
			return adv;

		adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		adv |= ethtool_adv_to_mii_ctrl1000_t(advertise);

		if (adv != oldadv) {
			err = phy_write(phydev, MII_CTRL1000, adv);

			if (err < 0)
				return err;
			changed = 1;
		}
	}

	return changed;
}

/**
 * genphy_setup_forced - configures/forces speed/duplex from @phydev
 * @phydev: target phy_device struct
 *
 * Description: Configures MII_BMCR to force speed/duplex
 *   to the values in phydev. Assumes that the values are valid.
 *   Please see phy_sanitize_settings().
 */
int genphy_setup_forced(struct phy_device *phydev)
{
	int err;
	int ctl = 0;

	phydev->pause = phydev->asym_pause = 0;

	if (SPEED_1000 == phydev->speed)
		ctl |= BMCR_SPEED1000;
	else if (SPEED_100 == phydev->speed)
		ctl |= BMCR_SPEED100;

	if (DUPLEX_FULL == phydev->duplex)
		ctl |= BMCR_FULLDPLX;

	err = phy_write(phydev, MII_BMCR, ctl);

	return err;
}

int phy_wait_aneg_done(struct phy_device *phydev)
{
	uint64_t start = get_time_ns();

	if (phydev->autoneg == AUTONEG_DISABLE)
		return 0;

	while (!is_timeout(start, PHY_AN_TIMEOUT * SECOND)) {
		if (phy_aneg_done(phydev) > 0)
			break;
	}

	do {
		genphy_update_link(phydev);
		if (phydev->link == 1)
			return 0;
	} while (!is_timeout(start, PHY_AN_TIMEOUT * SECOND));

	return -ETIMEDOUT;
}

/**
 * genphy_restart_aneg - Enable and Restart Autonegotiation
 * @phydev: target phy_device struct
 */
int genphy_restart_aneg(struct phy_device *phydev)
{
	int ctl, pdown;

	ctl = phy_read(phydev, MII_BMCR);

	if (ctl < 0)
		return ctl;

	ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

	/* Don't isolate the PHY if we're negotiating */
	ctl &= ~(BMCR_ISOLATE);

	/* Clear powerdown bit which eventually is set on some phys */
	pdown = ctl & BMCR_PDOWN;
	ctl &= ~BMCR_PDOWN;

	ctl = phy_write(phydev, MII_BMCR, ctl);

	if (ctl < 0)
		return ctl;

	/* Micrel's ksz9031 (and perhaps others?): Changing the PDOWN bit
	 * from '1' to '0' generates an internal reset. Must wait a minimum
	 * of 1ms before read/write access to the PHY registers. */
	if (pdown)
		mdelay(1);

	return 0;
}

/**
 * genphy_config_aneg - restart auto-negotiation or write BMCR
 * @phydev: target phy_device struct
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   advertising, and then restart auto-negotiation.  If it is not
 *   enabled, then we write the BMCR.
 */
int genphy_config_aneg(struct phy_device *phydev)
{
	int result;

	if (AUTONEG_ENABLE != phydev->autoneg)
		return genphy_setup_forced(phydev);

	result = genphy_config_advert(phydev);

	if (result < 0) /* error */
		return result;

	if (result == 0) {
		/* Advertisement hasn't changed, but maybe aneg was never on to
		 * begin with?  Or maybe phy was isolated? */
		int ctl = phy_read(phydev, MII_BMCR);

		if (ctl < 0)
			return ctl;

		if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
			result = 1; /* do restart aneg */
	}

	/* Only restart aneg if we are advertising something different
	 * than we were before.	 */
	if (result > 0)
		result = genphy_restart_aneg(phydev);

	return result;
}

/**
 * genphy_aneg_done - return auto-negotiation status
 * @phydev: target phy_device struct
 *
 * Description: Reads the status register and returns 0 either if
 *   auto-negotiation is incomplete, or if there was an error.
 *   Returns BMSR_ANEGCOMPLETE if auto-negotiation is done.
 */
int genphy_aneg_done(struct phy_device *phydev)
{
	int bmsr = phy_read(phydev, MII_BMSR);

	if (bmsr < 0)
		return bmsr;

	/* Restart auto-negotiation if remote fault */
	if (bmsr & BMSR_RFAULT) {
		puts("PHY remote fault detected\n"
		     "PHY restarting auto-negotiation\n");
		phy_write(phydev, MII_BMCR,
				  BMCR_ANENABLE | BMCR_ANRESTART);
	}

	return bmsr & BMSR_ANEGCOMPLETE;
}
EXPORT_SYMBOL(genphy_aneg_done);

/**
 * genphy_update_link - update link status in @phydev
 * @phydev: target phy_device struct
 *
 * Description: Update the value in phydev->link to reflect the
 *   current link value.  In order to do this, we need to read
 *   the status register twice, keeping the second value.
 */
int genphy_update_link(struct phy_device *phydev)
{
	int status;

	/* Do a fake read */
	status = phy_read(phydev, MII_BMSR);

	if (status < 0)
		return status;

	/* wait phy status update in the phy */
	udelay(1000);

	/* Read link and autonegotiation status */
	status = phy_read(phydev, MII_BMSR);

	if (status < 0)
		return status;

	if ((status & BMSR_LSTATUS) == 0)
		phydev->link = 0;
	else
		phydev->link = 1;

	return 0;
}

/**
 * genphy_read_status - check the link status and update current link state
 * @phydev: target phy_device struct
 *
 * Description: Check the link, then figure out the current state
 *   by comparing what we advertise with what the link partner
 *   advertises.  Start by checking the gigabit possibilities,
 *   then move on to 10/100.
 */
int genphy_read_status(struct phy_device *phydev)
{
	int adv;
	int err;
	int lpa;
	int lpagb = 0;

	/* if force the status and link are set */
	if (phydev->force)
		return 0;

	/* Update the link, but return if there
	 * was an error */
	err = genphy_update_link(phydev);
	if (err)
		return err;

	if (AUTONEG_ENABLE == phydev->autoneg) {
		if (phydev->supported & (SUPPORTED_1000baseT_Half
					| SUPPORTED_1000baseT_Full)) {
			lpagb = phy_read(phydev, MII_STAT1000);

			if (lpagb < 0)
				return lpagb;

			adv = phy_read(phydev, MII_CTRL1000);

			if (adv < 0)
				return adv;

			lpagb &= adv << 2;
		}

		lpa = phy_read(phydev, MII_LPA);

		if (lpa < 0)
			return lpa;

		adv = phy_read(phydev, MII_ADVERTISE);

		if (adv < 0)
			return adv;

		lpa &= adv;

		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_HALF;
		phydev->pause = phydev->asym_pause = 0;

		if (lpagb & (LPA_1000FULL | LPA_1000HALF)) {
			phydev->speed = SPEED_1000;

			if (lpagb & LPA_1000FULL)
				phydev->duplex = DUPLEX_FULL;
		} else if (lpa & (LPA_100FULL | LPA_100HALF)) {
			phydev->speed = SPEED_100;

			if (lpa & LPA_100FULL)
				phydev->duplex = DUPLEX_FULL;
		} else
			if (lpa & LPA_10FULL)
				phydev->duplex = DUPLEX_FULL;

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

static inline void mmd_phy_indirect(struct phy_device *phydev, int prtad,
					int devad)
{
	/* Write the desired MMD Devad */
	phy_write(phydev, MII_MMD_CTRL, devad);

	/* Write the desired MMD register address */
	phy_write(phydev, MII_MMD_DATA, prtad);

	/* Select the Function : DATA with no post increment */
	phy_write(phydev, MII_MMD_CTRL, (devad | MII_MMD_CTRL_NOINCR));
}

/**
 * phy_read_mmd_indirect - reads data from the MMD registers
 * @phy_device: phy device
 * @prtad: MMD Address
 * @devad: MMD DEVAD
 *
 * Description: it reads data from the MMD registers (clause 22 to access to
 * clause 45) of the specified phy address.
 * To read these register we have:
 * 1) Write reg 13 // DEVAD
 * 2) Write reg 14 // MMD Address
 * 3) Write reg 13 // MMD Data Command for MMD DEVAD
 * 3) Read  reg 14 // Read MMD data
 */
int phy_read_mmd_indirect(struct phy_device *phydev, int prtad, int devad)
{
	u32 ret;

	mmd_phy_indirect(phydev, prtad, devad);

	/* Read the content of the MMD's selected register */
	ret = phy_read(phydev, MII_MMD_DATA);

	return ret;
}

/**
 * phy_write_mmd_indirect - writes data to the MMD registers
 * @phy_device: phy device
 * @prtad: MMD Address
 * @devad: MMD DEVAD
 * @data: data to write in the MMD register
 *
 * Description: Write data from the MMD registers of the specified
 * phy address.
 * To write these register we have:
 * 1) Write reg 13 // DEVAD
 * 2) Write reg 14 // MMD Address
 * 3) Write reg 13 // MMD Data Command for MMD DEVAD
 * 3) Write reg 14 // Write MMD data
 */
void phy_write_mmd_indirect(struct phy_device *phydev, int prtad, int devad,
				   u16 data)
{
	mmd_phy_indirect(phydev, prtad, devad);

	/* Write the data into MMD's selected register */
	phy_write(phydev, MII_MMD_DATA, data);
}

int genphy_config_init(struct phy_device *phydev)
{
	int val;
	u32 features;

	/* For now, I'll claim that the generic driver supports
	 * all possible port types */
	features = (SUPPORTED_TP | SUPPORTED_MII
			| SUPPORTED_AUI | SUPPORTED_FIBRE |
			SUPPORTED_BNC);

	/* Do we support autonegotiation? */
	val = phy_read(phydev, MII_BMSR);

	if (val < 0)
		return val;

	if (val & BMSR_ANEGCAPABLE)
		features |= SUPPORTED_Autoneg;

	if (val & BMSR_100FULL)
		features |= SUPPORTED_100baseT_Full;
	if (val & BMSR_100HALF)
		features |= SUPPORTED_100baseT_Half;
	if (val & BMSR_10FULL)
		features |= SUPPORTED_10baseT_Full;
	if (val & BMSR_10HALF)
		features |= SUPPORTED_10baseT_Half;

	if (val & BMSR_ESTATEN) {
		val = phy_read(phydev, MII_ESTATUS);

		if (val < 0)
			return val;

		if (val & ESTATUS_1000_TFULL)
			features |= SUPPORTED_1000baseT_Full;
		if (val & ESTATUS_1000_THALF)
			features |= SUPPORTED_1000baseT_Half;
	}

	phydev->supported &= features;
	phydev->advertising &= features;

	return 0;
}

int phy_driver_register(struct phy_driver *phydrv)
{
	phydrv->drv.bus = &mdio_bus_type;

	if (!phydrv->config_init)
		phydrv->config_init = genphy_config_init;

	if (!phydrv->config_aneg)
		phydrv->config_aneg = genphy_config_aneg;

	if (!phydrv->aneg_done)
		phydrv->aneg_done = genphy_aneg_done;

	if (!phydrv->read_status)
		phydrv->read_status = genphy_read_status;

	return register_driver(&phydrv->drv);
}

int phy_drivers_register(struct phy_driver *new_driver, int n)
{
	int i, ret = 0;

	for (i = 0; i < n; i++) {
		ret = phy_driver_register(new_driver + i);
		if (ret)
			return ret;
	}
	return ret;
}

int phy_init_hw(struct phy_device *phydev)
{
	struct phy_driver *phydrv = to_phy_driver(phydev->dev.driver);
	int ret;

	if (!phydrv || !phydrv->config_init)
		return 0;

	ret = phy_scan_fixups(phydev);
	if (ret < 0)
		return ret;

	return phydrv->config_init(phydev);
}

static struct phy_driver genphy_driver = {
	.drv.name = "Generic PHY",
	.phy_id = PHY_ANY_UID,
	.phy_id_mask = PHY_ANY_UID,
	.features = PHY_GBIT_FEATURES | SUPPORTED_MII |
		SUPPORTED_AUI | SUPPORTED_FIBRE |
		SUPPORTED_BNC,
};

static int generic_phy_register(void)
{
	return phy_driver_register(&genphy_driver);
}
device_initcall(generic_phy_register);
