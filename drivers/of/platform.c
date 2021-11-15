// SPDX-License-Identifier: GPL-2.0
/*
 * platform.c - bus/device related devicetree functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 */
#include <common.h>
#include <deep-probe.h>
#include <malloc.h>
#include <of.h>
#include <of_address.h>
#include <linux/amba/bus.h>

/**
 * of_find_device_by_node - Find the platform_device associated with a node
 * @np: Pointer to device tree node
 *
 * Returns platform_device pointer, or NULL if not found
 */
struct device_d *of_find_device_by_node(struct device_node *np)
{
	struct device_d *dev;
	int ret;

	ret = of_device_ensure_probed(np);
	if (ret)
		return NULL;

	for_each_device(dev)
		if (dev->device_node == np)
			return dev;
	return NULL;
}
EXPORT_SYMBOL(of_find_device_by_node);

/**
 * of_device_make_bus_id - Use the device node data to assign a unique name
 * @dev: pointer to device structure that is linked to a device tree node
 *
 * This routine will first try using the translated bus address to
 * derive a unique name. If it cannot, then it will prepend names from
 * parent nodes until a unique name can be derived.
 */
static void of_device_make_bus_id(struct device_d *dev)
{
	struct device_node *node = dev->device_node;
	const __be32 *reg;
	u64 addr;

	/* Construct the name, using parent nodes if necessary to ensure uniqueness */
	while (node->parent) {
		/*
		 * If the address can be translated, then that is as much
		 * uniqueness as we need. Make it the first component and return
		 */
		reg = of_get_property(node, "reg", NULL);
		if (reg && (addr = of_translate_address(node, reg)) != OF_BAD_ADDR) {
			dev_set_name(dev, dev->name ? "%llx.%s:%s" : "%llx.%s.of",
				     (unsigned long long)addr, node->name,
				     dev->name);
			return;
		}

		/* format arguments only used if dev_name() resolves to NULL */
		dev_set_name(dev, dev->name ? "%s:%s" : "%s.of",
			     kbasename(node->full_name), dev->name);
		node = node->parent;
	}
}

static void of_dma_configure(struct device_d *dev, struct device_node *np)
{
	u64 dma_addr, paddr, size = 0;
	unsigned long offset;
	int ret;

	ret = of_dma_get_range(np, &dma_addr, &paddr, &size);
	if (ret < 0) {
		dma_addr = offset = 0;
	} else {
		offset = paddr - dma_addr;
	}

	dev->dma_offset = offset;
}

/**
 * of_platform_device_create - Alloc, initialize and register an of_device
 * @np: pointer to node to create device for
 * @parent: device model parent device.
 *
 * Returns pointer to created platform device, or NULL if a device was not
 * registered. Unavailable devices will not get registered.
 */
struct device_d *of_platform_device_create(struct device_node *np,
						struct device_d *parent)
{
	struct device_d *dev;
	struct resource *res = NULL, temp_res;
	resource_size_t resinval;
	int i, ret, num_reg = 0;

	if (!of_device_is_available(np))
		return NULL;

	/*
	 * Linux uses the OF_POPULATED flag to skip already populated/created
	 * devices.
	 */
	if (np->dev)
		return np->dev;

	/* count the io resources */
	if (of_can_translate_address(np))
		while (of_address_to_resource(np, num_reg, &temp_res) == 0)
			num_reg++;

	/* Populate the resource table */
	if (num_reg) {
		res = xzalloc(sizeof(*res) * num_reg);
		for (i = 0; i < num_reg; i++) {
			ret = of_address_to_resource(np, i, &res[i]);
			if (ret) {
				free(res);
				return NULL;
			}
		}
	}

	/* setup generic device info */
	dev = xzalloc(sizeof(*dev));
	dev->id = DEVICE_ID_SINGLE;
	dev->device_node = np;
	dev->parent = parent;
	dev->resource = res;
	dev->num_resources = num_reg;
	of_device_make_bus_id(dev);

	of_dma_configure(dev, np);

	resinval = (-1);

	debug("%s: register device %s, io=%pa\n",
			__func__, dev_name(dev),
		(num_reg) ? &dev->resource[0].start : &resinval);

	BUG_ON(np->dev);
	np->dev = dev;

	ret = platform_device_register(dev);
	if (!ret)
		return dev;

	np->dev = NULL;

	free(dev);
	if (num_reg)
		free(res);
	return NULL;
}

struct driver_d dummy_driver = {
	.name = "dummy-driver",
};

void of_platform_device_dummy_drv(struct device_d *dev)
{
	dev->driver = &dummy_driver;
}

/**
 * of_device_enable_and_register - Enable and register device
 * @np: pointer to node to enable create device for
 *
 * Returns pointer to created platform device, or NULL if a device was not
 * registered. Unavailable devices will not get registered.
 */
struct device_d *of_device_enable_and_register(struct device_node *np)
{
	struct device_d *dev;

	of_device_enable(np);

	dev = of_platform_device_create(np, NULL);
	if (!dev)
		return NULL;

	return dev;
}
EXPORT_SYMBOL(of_device_enable_and_register);

/**
 * of_device_enable_and_register_by_name - Enable and register device by name
 * @name: name or path of the device node
 *
 * Returns pointer to created platform device, or NULL if a device was not
 * registered. Unavailable devices will not get registered.
 */
struct device_d *of_device_enable_and_register_by_name(const char *name)
{
	struct device_node *node;

	node = of_find_node_by_name(NULL, name);
	if (!node)
		node = of_find_node_by_path(name);

	if (!node)
		return NULL;

	return of_device_enable_and_register(node);
}
EXPORT_SYMBOL(of_device_enable_and_register_by_name);

/**
 * of_device_enable_and_register_by_alias - Enable and register device by alias
 * @name: alias of the device node
 *
 * Returns pointer to created platform device, or NULL if a device was not
 * registered. Unavailable devices will not get registered.
 */
struct device_d *of_device_enable_and_register_by_alias(const char *alias)
{
	struct device_node *node;

	node = of_find_node_by_alias(NULL, alias);
	if (!node)
		return NULL;

	return of_device_enable_and_register(node);
}
EXPORT_SYMBOL(of_device_enable_and_register_by_alias);

#ifdef CONFIG_ARM_AMBA
static struct device_d *of_amba_device_create(struct device_node *np)
{
	struct amba_device *dev;
	int ret;

	debug("Creating amba device %s\n", np->full_name);

	if (!of_device_is_available(np))
		return NULL;

	/*
	 * Linux uses the OF_POPULATED flag to skip already populated/created
	 * devices.
	 */
	if (np->dev)
		return np->dev;

	dev = xzalloc(sizeof(*dev));

	/* setup generic device info */
	dev->dev.id = DEVICE_ID_SINGLE;
	dev->dev.device_node = np;
	of_device_make_bus_id(&dev->dev);

	ret = of_address_to_resource(np, 0, &dev->res);
	if (ret)
		goto amba_err_free;

	dev->dev.resource = &dev->res;
	dev->dev.num_resources = 1;

	/* Allow the HW Peripheral ID to be overridden */
	of_property_read_u32(np, "arm,primecell-periphid", &dev->periphid);

	debug("register device %pa\n", &dev->dev.resource[0].start);

	ret = amba_device_add(dev);
	if (ret)
		goto amba_err_free;

	np->dev = &dev->dev;

	return &dev->dev;

amba_err_free:
	free(dev);
	return NULL;
}
#else /* CONFIG_ARM_AMBA */
static inline struct device_d *of_amba_device_create(struct device_node *np)
{
	return NULL;
}
#endif /* CONFIG_ARM_AMBA */

/**
 * of_platform_bus_create() - Create a device for a node and its children.
 * @bus: device node of the bus to instantiate
 * @matches: match table for bus nodes
 * @parent: parent for new device, or NULL for top level.
 *
 * Creates a platform_device for the provided device_node, and optionally
 * recursively create devices for all the child nodes.
 */
static int of_platform_bus_create(struct device_node *bus,
				const struct of_device_id *matches,
				struct device_d *parent)
{
	struct device_node *child;
	struct device_d *dev;
	int rc = 0;

	/* Make sure it has a compatible property */
	if (!of_get_property(bus, "compatible", NULL)) {
		pr_debug("%s() - skipping %s, no compatible prop\n",
			__func__, bus->full_name);
		return 0;
	}

	if (of_device_is_compatible(bus, "arm,primecell")) {
		if (of_amba_device_create(bus))
			return 0;
	}

	dev = of_platform_device_create(bus, parent);
	if (!dev || !of_match_node(matches, bus))
		return 0;

	for_each_child_of_node(bus, child) {
		pr_debug("   create child: %s\n", child->full_name);
		rc = of_platform_bus_create(child, matches, dev);
		if (rc)
			break;
	}
	return rc;
}

/**
 * of_platform_populate() - Populate platform_devices from device tree data
 * @root: parent of the first level to probe or NULL for the root of the tree
 * @matches: match table, NULL to use the default
 * @parent: parent to hook devices from, NULL for toplevel
 *
 * This function walks the device tree given by @root node and creates devices
 * from nodes.  It requires all device nodes to have a 'compatible' property,
 * and it is suitable for creating devices which are children of the root
 * node.
 *
 * Returns 0 on success, < 0 on failure.
 */
int of_platform_populate(struct device_node *root,
			const struct of_device_id *matches,
			struct device_d *parent)
{
	struct device_node *child;
	int rc = 0;

	if (!root)
		root = of_find_node_by_path("/");
	if (!root)
		return -EINVAL;

	for_each_child_of_node(root, child) {
		rc = of_platform_bus_create(child, matches, parent);
		if (rc)
			break;
	}

	return rc;
}
EXPORT_SYMBOL_GPL(of_platform_populate);

struct device_d *of_device_create_on_demand(struct device_node *np)
{
	struct device_node *parent;
	struct device_d *parent_dev, *dev;

	if (!deep_probe_is_supported())
		return NULL;

	parent = of_get_parent(np);
	if (!parent)
		return NULL;

	if (!np->dev)
		pr_debug("Creating device for %s\n", np->full_name);

	/* Create all parent devices needed for the requested device */
	parent_dev = parent->dev ? : of_device_create_on_demand(parent);
	if (IS_ERR(parent_dev))
		return parent_dev;

	/*
	 * Parent devices like i2c/spi controllers are populating their own
	 * devices. So it can be that the requested device already exists after
	 * the parent device creation.
	 */
	if (np->dev)
		return np->dev;

	if (of_device_is_compatible(np, "arm,primecell"))
		dev = of_amba_device_create(np);
	else
		dev = of_platform_device_create(np, parent_dev);

	return dev ? : ERR_PTR(-ENODEV);
}

/**
 * of_device_ensure_probed() - ensures that a device is probed
 *
 * @np: the device_node handle which should be probed
 *
 * Ensures that the device is populated and probed so frameworks can make use of
 * it.
 *
 * Return: %0 on success
 *	   %-ENODEV if either the device can't be populated, the driver is
 *	     missing or the driver probe returns an error.
 */
int of_device_ensure_probed(struct device_node *np)
{
	struct device_d *dev;

	if (!deep_probe_is_supported())
		return 0;

	dev = of_device_create_on_demand(np);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	BUG_ON(!dev);

	/*
	 * The deep-probe mechanism relies on the fact that all necessary
	 * drivers are added before the device creation. Furthermore deep-probe
	 * is the answer to the EPROBE_DEFER errno so we must ensure that the
	 * driver was probed successfully after the device creation. Both
	 * requirements are fulfilled if 'dev->driver' is not NULL.
	 */
	if (!dev->driver)
		return -ENODEV;

	return 0;
}
EXPORT_SYMBOL_GPL(of_device_ensure_probed);

/**
 * of_device_ensure_probed_by_alias() - ensures that a device is probed
 *
 * @alias: the alias string to search for a device
 *
 * The function search for a given alias string and ensures that the device is
 * populated and probed if found.
 *
 * Return: %0 on success
 *	   %-ENODEV if either the device can't be populated, the driver is
 *	     missing or the driver probe returns an error
 *	   %-EINVAL if alias can't be found
 */
int of_device_ensure_probed_by_alias(const char *alias)
{
	struct device_node *dev_node;

	dev_node = of_find_node_by_alias(NULL, alias);
	if (!dev_node)
		return -EINVAL;

	return of_device_ensure_probed(dev_node);
}
EXPORT_SYMBOL_GPL(of_device_ensure_probed_by_alias);

/**
 * of_devices_ensure_probed_by_dev_id() - ensures that devices are probed
 *
 * @ids: the matching 'struct of_device_id' ids
 *
 * The function start searching the device tree from @np and populates and
 * probes devices which match @ids.
 *
 * Return: %0 on success
 *	   %-ENODEV if either the device wasn't found, can't be populated,
 *	     the driver is missing or the driver probe returns an error
 */
int of_devices_ensure_probed_by_dev_id(const struct of_device_id *ids)
{
	struct device_node *np;
	int err, ret = 0;

	for_each_matching_node(np, ids) {
		if (!of_device_is_available(np))
			continue;

		err = of_device_ensure_probed(np);
		if (err)
			ret = err;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(of_devices_ensure_probed_by_dev_id);

/**
 * of_devices_ensure_probed_by_property() - ensures that devices are probed
 *
 * @property_name: The property name to search for
 *
 * The function starts searching the whole device tree and populates and probes
 * devices which matches @property_name.
 *
 * Return: %0 on success
 *	   %-ENODEV if either the device wasn't found, can't be populated,
 *	     the driver is missing or the driver probe returns an error
 */
int of_devices_ensure_probed_by_property(const char *property_name)
{
	struct device_node *node;

	for_each_node_with_property(node, property_name) {
		int ret;

		ret = of_device_ensure_probed(node);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(of_devices_ensure_probed_by_property);

static int of_stdoutpath_init(void)
{
	struct device_node *np;

	np = of_get_stdoutpath(NULL);
	if (!np)
		return 0;

	/*
	 * With deep probe support the device providing the console
	 * can come quite late in the probe order. Make sure it's
	 * probed now so that we get output earlier.
	 */
	return of_device_ensure_probed(np);
}
postconsole_initcall(of_stdoutpath_init);
