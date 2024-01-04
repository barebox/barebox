/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2009-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 */

#ifndef __PHY_H
#define __PHY_H

#include <driver.h>
#include <slice.h>
#include <linux/list.h>
#include <linux/ethtool.h>
#include <linux/mii.h>

#define PHY_DEFAULT_FEATURES    (SUPPORTED_Autoneg | \
				 SUPPORTED_TP | \
				 SUPPORTED_MII)

#define PHY_10BT_FEATURES       (SUPPORTED_10baseT_Half | \
				 SUPPORTED_10baseT_Full)

#define PHY_100BT_FEATURES      (SUPPORTED_100baseT_Half | \
				 SUPPORTED_100baseT_Full)

#define PHY_1000BT_FEATURES     (SUPPORTED_1000baseT_Half | \
				 SUPPORTED_1000baseT_Full)

#define PHY_BASIC_FEATURES      (PHY_10BT_FEATURES | \
				 PHY_100BT_FEATURES | \
				 PHY_DEFAULT_FEATURES)

#define PHY_GBIT_FEATURES       (PHY_BASIC_FEATURES | \
				 PHY_1000BT_FEATURES)

/**
 * enum phy_interface_t - Interface Mode definitions
 *
 * @PHY_INTERFACE_MODE_NA: Not Applicable - don't touch
 * @PHY_INTERFACE_MODE_INTERNAL: No interface, MAC and PHY combined
 * @PHY_INTERFACE_MODE_MII: Media-independent interface
 * @PHY_INTERFACE_MODE_GMII: Gigabit media-independent interface
 * @PHY_INTERFACE_MODE_SGMII: Serial gigabit media-independent interface
 * @PHY_INTERFACE_MODE_TBI: Ten Bit Interface
 * @PHY_INTERFACE_MODE_REVMII: Reverse Media Independent Interface
 * @PHY_INTERFACE_MODE_RMII: Reduced Media Independent Interface
 * @PHY_INTERFACE_MODE_REVRMII: Reduced Media Independent Interface in PHY role
 * @PHY_INTERFACE_MODE_RGMII: Reduced gigabit media-independent interface
 * @PHY_INTERFACE_MODE_RGMII_ID: RGMII with Internal RX+TX delay
 * @PHY_INTERFACE_MODE_RGMII_RXID: RGMII with Internal RX delay
 * @PHY_INTERFACE_MODE_RGMII_TXID: RGMII with Internal RX delay
 * @PHY_INTERFACE_MODE_RTBI: Reduced TBI
 * @PHY_INTERFACE_MODE_SMII: Serial MII
 * @PHY_INTERFACE_MODE_XGMII: 10 gigabit media-independent interface
 * @PHY_INTERFACE_MODE_XLGMII:40 gigabit media-independent interface
 * @PHY_INTERFACE_MODE_MOCA: Multimedia over Coax
 * @PHY_INTERFACE_MODE_PSGMII: Penta SGMII
 * @PHY_INTERFACE_MODE_QSGMII: Quad SGMII
 * @PHY_INTERFACE_MODE_TRGMII: Turbo RGMII
 * @PHY_INTERFACE_MODE_100BASEX: 100 BaseX
 * @PHY_INTERFACE_MODE_1000BASEX: 1000 BaseX
 * @PHY_INTERFACE_MODE_2500BASEX: 2500 BaseX
 * @PHY_INTERFACE_MODE_5GBASER: 5G BaseR
 * @PHY_INTERFACE_MODE_RXAUI: Reduced XAUI
 * @PHY_INTERFACE_MODE_XAUI: 10 Gigabit Attachment Unit Interface
 * @PHY_INTERFACE_MODE_10GBASER: 10G BaseR
 * @PHY_INTERFACE_MODE_25GBASER: 25G BaseR
 * @PHY_INTERFACE_MODE_USXGMII:  Universal Serial 10GE MII
 * @PHY_INTERFACE_MODE_10GKR: 10GBASE-KR - with Clause 73 AN
 * @PHY_INTERFACE_MODE_QUSGMII: Quad Universal SGMII
 * @PHY_INTERFACE_MODE_1000BASEKX: 1000Base-KX - with Clause 73 AN
 * @PHY_INTERFACE_MODE_MAX: Book keeping
 *
 * Describes the interface between the MAC and PHY.
 */
typedef enum {
	PHY_INTERFACE_MODE_NA,
	PHY_INTERFACE_MODE_INTERNAL,
	PHY_INTERFACE_MODE_MII,
	PHY_INTERFACE_MODE_GMII,
	PHY_INTERFACE_MODE_SGMII,
	PHY_INTERFACE_MODE_TBI,
	PHY_INTERFACE_MODE_REVMII,
	PHY_INTERFACE_MODE_RMII,
	PHY_INTERFACE_MODE_REVRMII,
	PHY_INTERFACE_MODE_RGMII,
	PHY_INTERFACE_MODE_RGMII_ID,
	PHY_INTERFACE_MODE_RGMII_RXID,
	PHY_INTERFACE_MODE_RGMII_TXID,
	PHY_INTERFACE_MODE_RTBI,
	PHY_INTERFACE_MODE_SMII,
	PHY_INTERFACE_MODE_XGMII,
	PHY_INTERFACE_MODE_XLGMII,
	PHY_INTERFACE_MODE_MOCA,
	PHY_INTERFACE_MODE_PSGMII,
	PHY_INTERFACE_MODE_QSGMII,
	PHY_INTERFACE_MODE_TRGMII,
	PHY_INTERFACE_MODE_100BASEX,
	PHY_INTERFACE_MODE_1000BASEX,
	PHY_INTERFACE_MODE_2500BASEX,
	PHY_INTERFACE_MODE_5GBASER,
	PHY_INTERFACE_MODE_RXAUI,
	PHY_INTERFACE_MODE_XAUI,
	/* 10GBASE-R, XFI, SFI - single lane 10G Serdes */
	PHY_INTERFACE_MODE_10GBASER,
	PHY_INTERFACE_MODE_25GBASER,
	PHY_INTERFACE_MODE_USXGMII,
	/* 10GBASE-KR - with Clause 73 AN */
	PHY_INTERFACE_MODE_10GKR,
	PHY_INTERFACE_MODE_QUSGMII,
	PHY_INTERFACE_MODE_1000BASEKX,
	PHY_INTERFACE_MODE_MAX,
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

/* Or MII_ADDR_C45 into regnum for read/write on mii_bus to enable the 21 bit
   IEEE 802.3ae clause 45 addressing mode used by 10GIGE phy chips. */
#define MII_ADDR_C45 (1<<30)

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

	struct device *parent;

	struct device dev;

	/* list of all PHYs on bus */
	struct phy_device *phy_map[PHY_MAX_ADDR];

	/* PHY addresses to be ignored when probing */
	u32 phy_mask;

	struct list_head list;

	bool is_multiplexed;

	struct slice slice;
};
#define to_mii_bus(d) container_of(d, struct mii_bus, dev)

static inline struct slice *mdiobus_slice(struct mii_bus *bus)
{
	return &bus->slice;
}

int mdiobus_register(struct mii_bus *bus);
void mdiobus_unregister(struct mii_bus *bus);
struct phy_device *mdiobus_scan(struct mii_bus *bus, int addr);

extern struct list_head mii_bus_list;

int mdiobus_detect(struct device *dev);

#define for_each_mii_bus(mii) \
	list_for_each_entry(mii, &mii_bus_list, list)

struct mii_bus *mdiobus_get_bus(int busnum);

struct mii_bus *of_mdio_find_bus(struct device_node *mdio_bus_np);

int mdiobus_read(struct mii_bus *bus, int addr, u32 regnum);
int mdiobus_write(struct mii_bus *bus, int addr, u32 regnum, u16 val);

/* phy_device: An instance of a PHY
 *
 * @bus: Pointer to the bus this PHY is on
 * @dev: driver model device structure for this PHY
 * @phy_id: UID for this device found during discovery
 * @c45_ids: 802.3-c45 Device Identifiers if is_c45.
 * @dev_flags: Device-specific flags used by the PHY driver.
 * @addr: Bus address of PHY
 * @attached_dev: The attached enet driver's device instance ptr
 *
 * speed, duplex, pause, supported, advertising, and
 * autoneg are used like in mii_if_info
 */
struct phy_device {
	struct mii_bus *bus;

	struct device dev;

	u32 phy_id;

	unsigned is_c45:1;

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

	int registered;

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
 * @phy_id: The result of reading the UID registers of this PHY
 *   type, and ANDing them with the phy_id_mask.  This driver
 *   only works for PHYs with IDs which match this field
 * @phy_id_mask: Defines the important bits of the phy_id
 * @features: A list of features (speed, duplex, etc) supported
 *   by this PHY
 * @driver_data: Static driver data
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
	const void *driver_data;
	bool is_phy;

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

	/* Determines the auto negotiation result */
	int (*aneg_done)(struct phy_device *phydev);

	/* Determines the negotiated speed and duplex */
	int (*read_status)(struct phy_device *phydev);

	/* Clears up any memory if needed */
	void (*remove)(struct phy_device *phydev);

	int (*read_page)(struct phy_device *phydev);
	int (*write_page)(struct phy_device *phydev, int page);

	struct driver	 drv;
};
#define to_phy_driver(d) ((d) ? container_of(d, struct phy_driver, drv) : NULL)

#define PHY_ANY_ID "MATCH ANY PHY"
#define PHY_ANY_UID 0xffffffff

#define PHY_ID_MATCH_EXACT(id) .phy_id = (id), .phy_id_mask = GENMASK(31, 0)
#define PHY_ID_MATCH_MODEL(id) .phy_id = (id), .phy_id_mask = GENMASK(31, 4)
#define PHY_ID_MATCH_VENDOR(id) .phy_id = (id), .phy_id_mask = GENMASK(31, 10)

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
struct phy_device *of_phy_register_fixed_link(struct device_node *np,
		                              struct eth_device *edev);

#define phy_register_drivers_macro(level, drvs)				\
        static int __init drvs##_register(void)				\
        {								\
                return phy_drivers_register(drvs, ARRAY_SIZE(drvs));	\
        }								\
        level##_initcall(drvs##_register)

#define device_phy_drivers(drvs)	\
        phy_register_drivers_macro(device, drvs)

#define phy_register_driver_macro(level, drv)		\
        static int __init drv##_register(void)		\
        {						\
                return phy_driver_register(&drv);	\
        }						\
        level##_initcall(drv##_register)

#define device_phy_driver(drv)	\
        phy_register_driver_macro(device, drv)

int phy_save_page(struct phy_device *phydev);
int phy_select_page(struct phy_device *phydev, int page);
int phy_restore_page(struct phy_device *phydev, int oldpage, int ret);
int phy_read_paged(struct phy_device *phydev, int page, u32 regnum);
int phy_write_paged(struct phy_device *phydev, int page, u32 regnum, u16 val);
int phy_modify_paged(struct phy_device *phydev, int page, u32 regnum,
		     u16 mask, u16 set);

struct phy_device *phy_device_create(struct mii_bus *bus, int addr, int phy_id);
int phy_register_device(struct phy_device* dev);
void phy_unregister_device(struct phy_device *phydev);

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

int phy_modify(struct phy_device *phydev, u32 regnum, u16 mask, u16 set);

/**
 * phy_set_bits - Convenience function for setting bits in a PHY register
 * @phydev: the phy_device struct
 * @regnum: register number to write
 * @val: bits to set
 */
static inline int phy_set_bits(struct phy_device *phydev, u32 regnum, u16 val)
{
	return phy_modify(phydev, regnum, 0, val);
}

/**
 * phy_clear_bits - Convenience function for clearing bits in a PHY register
 * @phydev: the phy_device struct
 * @regnum: register number to write
 * @val: bits to clear
 */
static inline int phy_clear_bits(struct phy_device *phydev, u32 regnum, u16 val)
{
	return phy_modify(phydev, regnum, val, 0);
}

/**
 * phy_interface_mode_is_rgmii - Convenience function for testing if a
 * PHY interface mode is RGMII (all variants)
 * @mode: the phy_interface_t enum
 */
static inline bool phy_interface_mode_is_rgmii(phy_interface_t mode)
{
	return mode >= PHY_INTERFACE_MODE_RGMII &&
		mode <= PHY_INTERFACE_MODE_RGMII_TXID;
};

/**
 * phy_interface_is_rgmii - Convenience function for testing if a PHY interface
 * is RGMII (all variants)
 * @phydev: the phy_device struct
 */
static inline bool phy_interface_is_rgmii(struct phy_device *phydev)
{
	return phy_interface_mode_is_rgmii(phydev->interface);
};

int phy_device_connect(struct eth_device *dev, struct mii_bus *bus, int addr,
		       void (*adjust_link) (struct eth_device *edev),
		       u32 flags, phy_interface_t interface);

int phy_update_status(struct phy_device *phydev);
int phy_wait_aneg_done(struct phy_device *phydev);

/* Generic PHY support and helper functions */
int genphy_config_init(struct phy_device *phydev);
int genphy_restart_aneg(struct phy_device *phydev);
int genphy_config_aneg(struct phy_device *phydev);
int genphy_aneg_done(struct phy_device *phydev);
int genphy_update_link(struct phy_device *phydev);
int genphy_read_status(struct phy_device *phydev);
int genphy_config_advert(struct phy_device *phydev);
int genphy_setup_forced(struct phy_device *phydev);
int get_phy_id(struct mii_bus *bus, int addr, u32 *phy_id);

int phy_register_fixup(const char *bus_id, u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *));
int phy_register_fixup_for_id(const char *bus_id,
		int (*run)(struct phy_device *));
int phy_scan_fixups(struct phy_device *phydev);
int phy_read_mmd_indirect(struct phy_device *phydev, int prtad, int devad);
void phy_write_mmd_indirect(struct phy_device *phydev, int prtad, int devad,
				   u16 data);
int phy_modify_mmd_indirect(struct phy_device *phydev, int prtad, int devad,
				    u16 mask, u16 set);

int phy_read_mmd(struct phy_device *phydev, int devad, u32 regnum);
int phy_write_mmd(struct phy_device *phydev, int devad, u32 regnum, u16 val);
int phy_modify_mmd(struct phy_device *phydev, int devad, u32 regnum,
		   u16 mask, u16 set);
int phy_modify_mmd_changed(struct phy_device *phydev, int devad, u32 regnum,
			   u16 mask, u16 set);

/**
 * phy_set_bits_mmd - Convenience function for setting bits in a register
 * on MMD
 * @phydev: the phy_device struct
 * @devad: the MMD containing register to modify
 * @regnum: register number to modify
 * @val: bits to set
 */
static inline int phy_set_bits_mmd(struct phy_device *phydev, int devad,
		u32 regnum, u16 val)
{
	return phy_modify_mmd(phydev, devad, regnum, 0, val);
}

/**
 * phy_clear_bits_mmd - Convenience function for clearing bits in a register
 * on MMD
 * @phydev: the phy_device struct
 * @devad: the MMD containing register to modify
 * @regnum: register number to modify
 * @val: bits to clear
 */
static inline int phy_clear_bits_mmd(struct phy_device *phydev, int devad,
		u32 regnum, u16 val)
{
	return phy_modify_mmd(phydev, devad, regnum, val, 0);
}

static inline bool phy_acquired(struct phy_device *phydev)
{
	return phydev && phydev->bus && slice_acquired(&phydev->bus->slice);
}

#define phydev_err(_phydev, format, args...)	\
	dev_err(&_phydev->dev, format, ##args)

#define phydev_err_probe(_phydev, err, format, args...)	\
	dev_err_probe(&_phydev->dev, err, format, ##args)

#define phydev_info(_phydev, format, args...)	\
	dev_info(&_phydev->dev, format, ##args)

#define phydev_warn(_phydev, format, args...)	\
	dev_warn(&_phydev->dev, format, ##args)

#define phydev_dbg(_phydev, format, args...)	\
	dev_dbg(&_phydev->dev, format, ##args)

#ifdef CONFIG_PHYLIB
int phy_register_fixup_for_uid(u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *));
#else
static inline int phy_register_fixup_for_uid(u32 phy_uid, u32 phy_uid_mask,
		int (*run)(struct phy_device *))
{
	return -ENOSYS;
}
#endif

extern struct bus_type mdio_bus_type;
#endif /* __PHYDEV_H__ */
