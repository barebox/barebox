/*
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

#ifndef __PHY_H
#define __PHY_H

#include <driver.h>
#include <linux/list.h>
#include <linux/ethtool.h>
#include <linux/mii.h>

#define PHY_BASIC_FEATURES	(SUPPORTED_10baseT_Half | \
				 SUPPORTED_10baseT_Full | \
				 SUPPORTED_100baseT_Half | \
				 SUPPORTED_100baseT_Full | \
				 SUPPORTED_Autoneg | \
				 SUPPORTED_TP | \
				 SUPPORTED_MII)

#define PHY_GBIT_FEATURES	(PHY_BASIC_FEATURES | \
				 SUPPORTED_1000baseT_Half | \
				 SUPPORTED_1000baseT_Full)

/* Interface Mode definitions */
typedef enum {
	PHY_INTERFACE_MODE_NA,
	PHY_INTERFACE_MODE_MII,
	PHY_INTERFACE_MODE_GMII,
	PHY_INTERFACE_MODE_SGMII,
	PHY_INTERFACE_MODE_TBI,
	PHY_INTERFACE_MODE_RMII,
	PHY_INTERFACE_MODE_RGMII,
	PHY_INTERFACE_MODE_RGMII_ID,
	PHY_INTERFACE_MODE_RGMII_RXID,
	PHY_INTERFACE_MODE_RGMII_TXID,
	PHY_INTERFACE_MODE_RTBI,
	PHY_INTERFACE_MODE_SMII,
} phy_interface_t;

#define PHY_INIT_TIMEOUT	100000
#define PHY_FORCE_TIMEOUT	10
#define PHY_AN_TIMEOUT		10

#define PHY_MAX_ADDR	32

/*
 * Need to be a little smaller than phydev->dev.bus_id to leave room
 * for the ":%02x"
 */
#define MII_BUS_ID_SIZE	(20 - 3)

#define PHYLIB_FORCE_10		(1 << 0)
#define PHYLIB_FORCE_100	(1 << 1)
#define PHYLIB_FORCE_LINK	(1 << 2)

#define PHYLIB_CAPABLE_1000M	(1 << 0)

/*
 * The Bus class for PHYs.  Devices which provide access to
 * PHYs should register using this structure
 */
struct mii_bus {
	void *priv;
	int (*read)(struct mii_bus *bus, int phy_id, int regnum);
	int (*write)(struct mii_bus *bus, int phy_id, int regnum, u16 val);
	int (*reset)(struct mii_bus *bus);

	struct device_d *parent;

	struct device_d dev;

	/* list of all PHYs on bus */
	struct phy_device *phy_map[PHY_MAX_ADDR];

	/* PHY addresses to be ignored when probing */
	u32 phy_mask;
};
#define to_mii_bus(d) container_of(d, struct mii_bus, dev)

int mdiobus_register(struct mii_bus *bus);
void mdiobus_unregister(struct mii_bus *bus);
struct phy_device *mdiobus_scan(struct mii_bus *bus, int addr);

/**
 * mdiobus_read - Convenience function for reading a given MII mgmt register
 * @bus: the mii_bus struct
 * @addr: the phy address
 * @regnum: register number to read
 */
static inline int mdiobus_read(struct mii_bus *bus, int addr, u32 regnum)
{
	return bus->read(bus, addr, regnum);
}

/**
 * mdiobus_write - Convenience function for writing a given MII mgmt register
 * @bus: the mii_bus struct
 * @addr: the phy address
 * @regnum: register number to write
 * @val: value to write to @regnum
 */
static inline int mdiobus_write(struct mii_bus *bus, int addr, u32 regnum, u16 val)
{
	return bus->write(bus, addr, regnum, val);
}

/* phy_device: An instance of a PHY
 *
 * bus: Pointer to the bus this PHY is on
 * dev: driver model device structure for this PHY
 * phy_id: UID for this device found during discovery
 * dev_flags: Device-specific flags used by the PHY driver.
 * addr: Bus address of PHY
 * attached_dev: The attached enet driver's device instance ptr
 *
 * speed, duplex, pause, supported, advertising, and
 * autoneg are used like in mii_if_info
 */
struct phy_device {
	struct mii_bus *bus;

	struct device_d dev;

	u32 phy_id;

	u32 dev_flags;

	phy_interface_t interface;

	/* Bus address of the PHY (0-31) */
	int addr;

	/*
	 * forced speed & duplex (no autoneg)
	 * partner speed & duplex & pause (autoneg)
	 */
	int speed;
	int duplex;
	int pause;
	int asym_pause;

	/* The most recently read link state */
	int link;

	/* Union of PHY and Attached devices' supported modes */
	/* See mii.h for more info */
	u32 supported;
	u32 advertising;

	int autoneg;
	int force;


	/* private data pointer */
	/* For use by PHYs to maintain extra state */
	void *priv;

	struct eth_device *attached_dev;
	void (*adjust_link)(struct eth_device *dev);

	struct cdev cdev;
};
#define to_phy_device(d) container_of(d, struct phy_device, dev)

/* struct phy_driver: Driver structure for a particular PHY type
 *
 * phy_id: The result of reading the UID registers of this PHY
 *   type, and ANDing them with the phy_id_mask.  This driver
 *   only works for PHYs with IDs which match this field
 * phy_id_mask: Defines the important bits of the phy_id
 * features: A list of features (speed, duplex, etc) supported
 *   by this PHY
 *
 * The drivers must implement config_aneg and read_status.  All
 * other functions are optional. Note that none of these
 * functions should be called from interrupt time.  The goal is
 * for the bus read/write functions to be able to block when the
 * bus transaction is happening, and be freed up by an interrupt
 * (The MPC85xx has this ability, though it is not currently
 * supported in the driver).
 */
struct phy_driver {
	u32 phy_id;
	unsigned int phy_id_mask;
	u32 features;

	/*
	 * Called to initialize the PHY,
	 * including after a reset
	 */
	int (*config_init)(struct phy_device *phydev);

	/*
	 * Called during discovery.  Used to set
	 * up device-specific structures, if any
	 */
	int (*probe)(struct phy_device *phydev);

	/*
	 * Configures the advertisement and resets
	 * autonegotiation if phydev->autoneg is on,
	 * forces the speed to the current settings in phydev
	 * if phydev->autoneg is off
	 */
	int (*config_aneg)(struct phy_device *phydev);

	/* Determines the negotiated speed and duplex */
	int (*read_status)(struct phy_device *phydev);

	/* Clears up any memory if needed */
	void (*remove)(struct phy_device *phydev);

	struct driver_d	 drv;
};
#define to_phy_driver(d) container_of(d, struct phy_driver, drv)

#define PHY_ANY_ID "MATCH ANY PHY"
#define PHY_ANY_UID 0xffffffff

/* A Structure for boards to register fixups with the PHY Lib */
struct phy_fixup {
	struct list_head list;
	char bus_id[20];
	u32 phy_uid;
	u32 phy_uid_mask;
	int (*run)(struct phy_device *phydev);
};

int phy_driver_register(struct phy_driver *drv);
int phy_drivers_register(struct phy_driver *new_driver, int n);
struct phy_device *get_phy_device(struct mii_bus *bus, int addr);
int phy_init(void);
int phy_init_hw(struct phy_device *phydev);

/**
 * phy_read - Convenience function for reading a given PHY register
 * @phydev: the phy_device struct
 * @regnum: register number to read
 */
static inline int phy_read(struct phy_device *phydev, u32 regnum)
{
	return mdiobus_read(phydev->bus, phydev->addr, regnum);
}

/**
 * phy_write - Convenience function for writing a given PHY register
 * @phydev: the phy_device struct
 * @regnum: register number to write
 * @val: value to write to @regnum
 */
static inline int phy_write(struct phy_device *phydev, u32 regnum, u16 val)
{
	return mdiobus_write(phydev->bus, phydev->addr, regnum, val);
}

int phy_device_connect(struct eth_device *dev, struct mii_bus *bus, int addr,
		       void (*adjust_link) (struct eth_device *edev),
		       u32 flags, phy_interface_t interface);

int phy_update_status(struct phy_device *phydev);
int phy_wait_aneg_done(struct phy_device *phydev);

/* Generic PHY support and helper functions */
int genphy_restart_aneg(struct phy_device *phydev);
int genphy_config_aneg(struct phy_device *phydev);
int genphy_update_link(struct phy_device *phydev);
int genphy_read_status(struct phy_device *phydev);
int genphy_config_advert(struct phy_device *phydev);
int genphy_setup_forced(struct phy_device *phydev);
int get_phy_id(struct mii_bus *bus, int addr, u32 *phy_id);

int phy_register_fixup(const char *bus_id, u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *));
int phy_register_fixup_for_id(const char *bus_id,
		int (*run)(struct phy_device *));
int phy_register_fixup_for_uid(u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *));
int phy_scan_fixups(struct phy_device *phydev);

int phy_read_mmd_indirect(struct phy_device *phydev, int prtad, int devad);
void phy_write_mmd_indirect(struct phy_device *phydev, int prtad, int devad,
				   u16 data);

extern struct bus_type mdio_bus_type;
#endif /* __PHYDEV_H__ */
