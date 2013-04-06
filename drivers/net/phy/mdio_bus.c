/*
 * drivers/net/phy/mdio_bus.c
 *
 * MDIO Bus interface
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <driver.h>
#include <init.h>
#include <clock.h>
#include <net.h>
#include <errno.h>
#include <linux/phy.h>
#include <linux/err.h>

/**
 * mdiobus_register - bring up all the PHYs on a given bus and attach them to bus
 * @bus: target mii_bus
 *
 * Description: Called by a bus driver to bring up all the PHYs
 *   on a given bus, and attach them to the bus.
 *
 * Returns 0 on success or < 0 on error.
 */
int mdiobus_register(struct mii_bus *bus)
{
	int err;

	if (NULL == bus ||
			NULL == bus->read ||
			NULL == bus->write)
		return -EINVAL;

	bus->dev.priv = bus;
	bus->dev.id = DEVICE_ID_DYNAMIC;
	strcpy(bus->dev.name, "miibus");
	bus->dev.parent = bus->parent;

	err = register_device(&bus->dev);
	if (err) {
		pr_err("mii_bus %s failed to register\n", bus->dev.name);
		return -EINVAL;
	}

	if (bus->reset)
		bus->reset(bus);

	pr_info("%s: probed\n", dev_name(&bus->dev));
	return 0;
}
EXPORT_SYMBOL(mdiobus_register);

void mdiobus_unregister(struct mii_bus *bus)
{
	int i;

	for (i = 0; i < PHY_MAX_ADDR; i++) {
		if (bus->phy_map[i])
			unregister_device(&bus->phy_map[i]->dev);
		bus->phy_map[i] = NULL;
	}
}
EXPORT_SYMBOL(mdiobus_unregister);

struct phy_device *mdiobus_scan(struct mii_bus *bus, int addr)
{
	struct phy_device *phydev;

	if (bus->phy_map[addr])
		return bus->phy_map[addr];

	phydev = get_phy_device(bus, addr);
	if (IS_ERR(phydev) || phydev == NULL)
		return phydev;

	bus->phy_map[addr] = phydev;

	return phydev;
}
EXPORT_SYMBOL(mdiobus_scan);

/**
 * mdio_bus_match - determine if given PHY driver supports the given PHY device
 * @dev: target PHY device
 * @drv: given PHY driver
 *
 * Description: Given a PHY device, and a PHY driver, return 1 if
 *   the driver supports the device.  Otherwise, return 0.
 */
static int mdio_bus_match(struct device_d *dev, struct driver_d *drv)
{
	struct phy_device *phydev = to_phy_device(dev);
	struct phy_driver *phydrv = to_phy_driver(drv);

	return ((phydrv->phy_id & phydrv->phy_id_mask) !=
		(phydev->phy_id & phydrv->phy_id_mask));
}

static ssize_t phydev_read(struct cdev *cdev, void *_buf, size_t count, loff_t offset, ulong flags)
{
	int i = count;
	uint16_t *buf = _buf;
	struct phy_device *phydev = cdev->priv;

	while (i > 0) {
		*buf = phy_read(phydev, offset / 2);
		buf++;
		i -= 2;
		offset += 2;
	}

	return count;
}

static ssize_t phydev_write(struct cdev *cdev, const void *_buf, size_t count, loff_t offset, ulong flags)
{
	int i = count;
	const uint16_t *buf = _buf;
	struct phy_device *phydev = cdev->priv;

	while (i > 0) {
		phy_write(phydev, offset / 2, *buf);
		buf++;
		i -= 2;
		offset += 2;
	}

	return count;
}

static struct file_operations phydev_ops = {
	.read  = phydev_read,
	.write = phydev_write,
	.lseek = dev_lseek_default,
};

static int mdio_bus_probe(struct device_d *_dev)
{
	struct phy_device *dev = to_phy_device(_dev);
	struct phy_driver *drv = to_phy_driver(_dev->driver);

	int ret;

	dev->attached_dev->phydev = dev;

	if (drv->probe) {
		ret = drv->probe(dev);
		if (ret)
			goto err;
	}

	if (dev->dev_flags) {
		if (dev->dev_flags & PHYLIB_FORCE_10) {
			dev->speed = SPEED_10;
			dev->duplex = DUPLEX_FULL;
			dev->autoneg = !AUTONEG_ENABLE;
			dev->force = 1;
			dev->link = 1;
		} else if (dev->dev_flags & PHYLIB_FORCE_100) {
			dev->speed = SPEED_100;
			dev->duplex = DUPLEX_FULL;
			dev->autoneg = !AUTONEG_ENABLE;
			dev->force = 1;
			dev->link = 1;
		}
	}

	/* Start out supporting everything. Eventually,
	 * a controller will attach, and may modify one
	 * or both of these values */
	dev->supported = drv->features;
	dev->advertising = drv->features;

	ret = phy_init_hw(dev);
	if (ret)
		goto err;

	/* Sanitize settings based on PHY capabilities */
	if ((dev->supported & SUPPORTED_Autoneg) == 0)
		dev->autoneg = AUTONEG_DISABLE;

	dev_add_param_int_ro(&dev->dev, "phy_addr", dev->addr, "%d");
	dev_add_param_int_ro(&dev->dev, "phy_id", dev->phy_id, "0x%08x");

	dev->cdev.name = asprintf("phy%d", _dev->id);
	dev->cdev.size = 64;
	dev->cdev.ops = &phydev_ops;
	dev->cdev.priv = dev;
	dev->cdev.dev = _dev;
	devfs_create(&dev->cdev);

	return 0;

err:
	dev->attached_dev->phydev = NULL;
	dev->attached_dev = NULL;
	return ret;
}

static void mdio_bus_remove(struct device_d *_dev)
{
	struct phy_device *dev = to_phy_device(_dev);
	struct phy_driver *drv = to_phy_driver(_dev->driver);

	if (drv->remove)
		drv->remove(dev);

	free(dev->cdev.name);
	devfs_remove(&dev->cdev);
}

struct bus_type mdio_bus_type = {
	.name		= "mdio_bus",
	.match		= mdio_bus_match,
	.probe		= mdio_bus_probe,
	.remove		= mdio_bus_remove,
};
EXPORT_SYMBOL(mdio_bus_type);

static int mdio_bus_init(void)
{
	return bus_register(&mdio_bus_type);
}
pure_initcall(mdio_bus_init);
