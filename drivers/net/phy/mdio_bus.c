// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/net/phy/mdio_bus.c
 *
 * MDIO Bus interface
 *
 * Author: Andy Fleming
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <of_gpio.h>
#include <driver.h>
#include <init.h>
#include <gpio.h>
#include <clock.h>
#include <net.h>
#include <errno.h>
#include <linux/mdio.h>
#include <linux/phy.h>
#include <linux/err.h>
#include <of_device.h>
#include <pinctrl.h>

#define DEFAULT_GPIO_RESET_ASSERT       1000      /* us */
#define DEFAULT_GPIO_RESET_DEASSERT     1000      /* us */

LIST_HEAD(mii_bus_list);

static struct phy_device *mdio_device_create(struct mii_bus *bus, int addr)
{
	struct phy_device *phydev;

	phydev = xzalloc(sizeof(*phydev));

	phydev->addr = addr;
	phydev->bus = bus;
	phydev->dev.bus = &mdio_bus_type;

	dev_set_name(&phydev->dev, "mdio%d-dev%02x", phydev->bus->dev.id,
		     phydev->addr);
	phydev->dev.id = DEVICE_ID_SINGLE;

	return phydev;
}

static int mdio_register_device(struct phy_device *phydev)
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

	return ret;
}

int mdio_driver_register(struct phy_driver *phydrv)
{
	phydrv->drv.bus = &mdio_bus_type;
	phydrv->is_phy = false;

	return register_driver(&phydrv->drv);
}

int mdiobus_detect(struct device *dev)
{
	struct mii_bus *mii = to_mii_bus(dev);
	int i, ret;

	if (mii->is_multiplexed)
		return 0;

	for (i = 0; i < PHY_MAX_ADDR; i++) {
		struct phy_device *phydev;

		phydev = mdiobus_scan(mii, i);
		if (IS_ERR(phydev))
			continue;
		if (phydev->registered)
			continue;
		ret = phy_register_device(phydev);
		if (ret)
			dev_err(dev, "failed to register phy: %s\n", strerror(-ret));
		dev_dbg(dev, "registered phy as /dev/%s\n", phydev->cdev.name);
	}

	return 0;
}

static int of_mdiobus_register_phy(struct mii_bus *mdio, struct device_node *child,
				   u32 addr)
{
	struct phy_device *phy;
	int ret;

	phy = get_phy_device(mdio, addr);
	if (IS_ERR(phy))
		return PTR_ERR(phy);

	/*
	 * Associate the OF node with the device structure so it
	 * can be looked up later
	 */
	child->dev = &phy->dev;
	phy->dev.of_node = child;

	/*
	 * All data is now stored in the phy struct;
	 * register it
	 */
	ret = phy_register_device(phy);
	if (ret)
		return ret;

	dev_dbg(&mdio->dev, "registered phy %s at address %i\n",
		child->name, addr);

	return 0;
}

static int of_mdiobus_register_device(struct mii_bus *mdio,
				      struct device_node *child, u32 addr)
{
	struct phy_device *mdiodev;
	int ret;

	mdiodev = mdio_device_create(mdio, addr);
	if (IS_ERR(mdiodev))
		return PTR_ERR(mdiodev);

	child->dev = &mdiodev->dev;
	mdiodev->dev.of_node = child;

	ret = mdio_register_device(mdiodev);
	if (ret)
		return ret;

	dev_dbg(&mdio->dev, "registered mdio device %s at address %i\n",
		child->name, addr);

	return 0;
}

/*
 * Node is considered a PHY node if:
 * o Compatible string of "ethernet-phy-idX.X"
 * o Compatible string of "ethernet-phy-ieee802.3-c45"
 * o Compatible string of "ethernet-phy-ieee802.3-c22"
 * o No compatibility string
 *
 * A device which is not a phy is expected to have a compatible string
 * indicating what sort of device it is.
 */
static bool of_mdiobus_child_is_phy(struct device_node *np)
{
	struct property *prop;
	const char *cp;

	of_property_for_each_string(np, "compatible", prop, cp) {
                if (!strncmp(cp, "ethernet-phy-id", strlen("ethernet-phy-id")))
			return true;
        }

	if (of_device_is_compatible(np, "ethernet-phy-ieee802.3-c45"))
		return true;

	if (of_device_is_compatible(np, "ethernet-phy-ieee802.3-c22"))
		return true;

	if (!of_find_property(np, "compatible", NULL))
		return true;

	return false;
}

/*
 * Reset the PHY, based on DT info
 *
 * If np is a phy node, and the phy node contains a reset-gpios property
 * then reset the phy.
 */
static void of_mdiobus_reset_phy(struct mii_bus *bus, struct device_node *np)
{
	enum of_gpio_flags of_flags;
	u32 reset_deassert_delay;
	u32 reset_assert_delay;
	unsigned long flags;
	int gpio;
	int ret;

	gpio = of_get_named_gpio_flags(np, "reset-gpios", 0, &of_flags);
	if (!gpio_is_valid(gpio))
		return;

	flags = GPIOF_OUT_INIT_INACTIVE;
	if (of_flags & OF_GPIO_ACTIVE_LOW)
		flags |= GPIOF_ACTIVE_LOW;

	ret = gpio_request_one(gpio, flags, np->name);
	if (ret) {
		dev_err(&bus->dev, "failed to request reset gpio for: %s\n",
			np->name);
		return;
	}

	reset_assert_delay = DEFAULT_GPIO_RESET_ASSERT;
	of_property_read_u32(np, "reset-assert-us", &reset_assert_delay);

	reset_deassert_delay = DEFAULT_GPIO_RESET_DEASSERT;
	of_property_read_u32(np, "reset-deassert-us", &reset_deassert_delay);

	/* reset the PHY */
	dev_dbg(&bus->dev, "reset PHY with GPIO: %d\n", gpio);
	gpio_set_active(gpio, 1);
	udelay(reset_assert_delay);
	gpio_set_active(gpio, 0);
	udelay(reset_deassert_delay);
}

/**
 * of_mdiobus_register - Register mii_bus and create PHYs from the device tree
 * @mdio: pointer to mii_bus structure
 * @np: pointer to device_node of MDIO bus.
 *
 * This function registers the mii_bus structure and registers a phy_device
 * for each child node of @np.
 */
static int of_mdiobus_register(struct mii_bus *mdio, struct device_node *np)
{
	struct device_node *child;
	u32 addr;
	int ret;

	/* Loop over the child nodes and register a phy_device for each one */
	for_each_available_child_of_node(np, child) {
		ret = of_property_read_u32(child, "reg", &addr);
		if (ret) {
			dev_dbg(&mdio->dev, "%pOF has invalid PHY address\n",
				child);
			continue;
		}

		if (addr >= PHY_MAX_ADDR) {
			dev_err(&mdio->dev, "%pOF PHY address %i is too large\n",
				child, addr);
			continue;
		}

		of_pinctrl_select_state_default(child);

		if (of_mdiobus_child_is_phy(child)) {
			of_mdiobus_reset_phy(mdio, child);
			of_mdiobus_register_phy(mdio, child, addr);
		} else {
			of_mdiobus_register_device(mdio, child, addr);
		}
	}

	return 0;
}

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
	dev_set_name(&bus->dev, "miibus");
	bus->dev.parent = bus->parent;
	bus->dev.detect = mdiobus_detect;

	err = register_device(&bus->dev);
	if (err) {
		pr_err("mii_bus %s failed to register\n", bus->dev.name);
		return -EINVAL;
	}

	slice_init(&bus->slice, dev_name(&bus->dev));

	if (bus->reset)
		bus->reset(bus);

	list_add_tail(&bus->list, &mii_bus_list);

	pr_info("%s: probed\n", dev_name(&bus->dev));

	if (bus->dev.of_node) {
		bus->dev.of_node->dev = &bus->dev;

		/* Register PHY's as child node to mdio node */
		of_mdiobus_register(bus, bus->dev.of_node);
	}
	else if (bus->parent->of_node) {
		/*
		 * Register PHY's as child node to the ethernet node,
		 * if there was no mdio node
		 */
		of_mdiobus_register(bus, bus->parent->of_node);
	}

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

	slice_exit(&bus->slice);

	list_del(&bus->list);
}
EXPORT_SYMBOL(mdiobus_unregister);

struct phy_device *mdiobus_scan(struct mii_bus *bus, int addr)
{
	struct phy_device *phydev;

	if (bus->phy_map[addr])
		return bus->phy_map[addr];

	phydev = get_phy_device(bus, addr);
	if (IS_ERR(phydev))
		return phydev;

	return phydev;
}
EXPORT_SYMBOL(mdiobus_scan);


/**
 *
 * mdio_get_bus - get a MDIO bus from its busnum
 *
 * @param	busnum	the desired bus number
 *
 */
struct mii_bus *mdiobus_get_bus(int busnum)
{
	struct mii_bus *mii;

	for_each_mii_bus(mii)
		if (mii->dev.id == busnum)
			return mii;

	return NULL;
}

/**
 * of_mdio_find_bus - Given an mii_bus node, find the mii_bus.
 * @mdio_bus_np: Pointer to the mii_bus.
 *
 * Returns a reference to the mii_bus, or NULL if none found.
 *
 * Because the association of a device_node and mii_bus is made via
 * mdiobus_register(), the mii_bus cannot be found before it is
 * registered with mdiobus_register().
 *
 */
struct mii_bus *of_mdio_find_bus(struct device_node *mdio_bus_np)
{
	struct mii_bus *mii;

	if (!mdio_bus_np)
		return NULL;

	for_each_mii_bus(mii)
		if (mii->dev.of_node == mdio_bus_np)
			return mii;

	return NULL;
}
EXPORT_SYMBOL(of_mdio_find_bus);


/**
 * mdio_bus_match - determine if given PHY driver supports the given PHY device
 * @dev: target PHY device
 * @drv: given PHY driver
 *
 * Description: Given a PHY device, and a PHY driver, return 0 if
 *   the driver supports the device.  Otherwise, return 1.
 */
static int mdio_bus_match(struct device *dev, struct driver *drv)
{
	struct phy_device *phydev = to_phy_device(dev);
	struct phy_driver *phydrv = to_phy_driver(drv);

	if (phydrv->is_phy) {
		if ((phydrv->phy_id & phydrv->phy_id_mask) ==
		    (phydev->phy_id & phydrv->phy_id_mask))
		return 0;
	} else {
		return device_match(dev, drv);
	}

	return 1;
}

/**
 * mdiobus_read - Convenience function for reading a given MII mgmt register
 * @bus: the mii_bus struct
 * @addr: the phy address
 * @regnum: register number to read
 */
int mdiobus_read(struct mii_bus *bus, int addr, u32 regnum)
{
	int ret;

	slice_acquire(&bus->slice);

	ret = bus->read(bus, addr, regnum);

	slice_release(&bus->slice);

	return ret;
}

/**
 * mdiobus_write - Convenience function for writing a given MII mgmt register
 * @bus: the mii_bus struct
 * @addr: the phy address
 * @regnum: register number to write
 * @val: value to write to @regnum
 */
int mdiobus_write(struct mii_bus *bus, int addr, u32 regnum, u16 val)
{
	int ret;

	slice_acquire(&bus->slice);

	ret = bus->write(bus, addr, regnum, val);

	slice_release(&bus->slice);

	return ret;
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

static struct cdev_operations phydev_ops = {
	.read  = phydev_read,
	.write = phydev_write,
};

static void of_set_phy_supported(struct phy_device *phydev)
{
	struct device_node *node = phydev->dev.of_node;
	u32 max_speed;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return;

	if (!node)
		return;

	if (!of_property_read_u32(node, "max-speed", &max_speed)) {
		/*
		 * The default values for phydev->supported are provided by the PHY
		 * driver "features" member, we want to reset to sane defaults first
		 * before supporting higher speeds.
		 */
		phydev->supported &= PHY_DEFAULT_FEATURES;

		switch (max_speed) {
		default:
			return;

		case SPEED_1000:
			phydev->supported |= PHY_1000BT_FEATURES;
		case SPEED_100:
			phydev->supported |= PHY_100BT_FEATURES;
		case SPEED_10:
			phydev->supported |= PHY_10BT_FEATURES;
		}
	}
}

static int mdio_bus_probe(struct device *_dev)
{
	struct phy_device *dev = to_phy_device(_dev);
	struct phy_driver *drv = to_phy_driver(_dev->driver);

	int ret;

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
	of_set_phy_supported(dev);
	dev->advertising = dev->supported;

	dev_add_param_uint32_ro(&dev->dev, "phy_addr", &dev->addr, "%u");
	dev_add_param_uint32_ro(&dev->dev, "phy_id", &dev->phy_id, "0x%08x");

	dev->cdev.name = xasprintf("mdio%d-phy%02x",
				   dev->bus->dev.id,
				   dev->addr);
	dev->cdev.size = 64;
	dev->cdev.ops = &phydev_ops;
	dev->cdev.priv = dev;
	dev->cdev.dev = _dev;
	devfs_create(&dev->cdev);

	return 0;

err:
	return ret;
}

static void mdio_bus_remove(struct device *_dev)
{
	struct phy_device *dev = to_phy_device(_dev);
	struct phy_driver *drv = to_phy_driver(_dev->driver);
	struct mii_bus *bus = dev->bus;

	if (drv->remove)
		drv->remove(dev);

	free(dev->cdev.name);
	devfs_remove(&dev->cdev);
	bus->phy_map[dev->addr] = NULL;
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
