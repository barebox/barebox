/*
 * platform.c - bus/device related devicetree functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
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
			dev_set_name(dev, dev->name ? "%llx.%s:%s" : "%llx.%s",
				     (unsigned long long)addr, node->name,
				     dev->name);
			return;
		}

		/* format arguments only used if dev_name() resolves to NULL */
		dev_set_name(dev, dev->name ? "%s:%s" : "%s",
			     kbasename(node->full_name), dev->name);
		node = node->parent;
	}
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
	int i, j, ret, num_reg = 0, match;

	if (!of_device_is_available(np))
		return NULL;

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

		/*
		 * A device may already be registered as platform_device.
		 * Instead of registering the same device again, just
		 * add this node to the existing device.
		 */
		for_each_device(dev) {
			if (!dev->resource)
				continue;

			for (i = 0, match = 0; i < num_reg; i++)
				for (j = 0; j < dev->num_resources; j++)
					if (dev->resource[j].start ==
						res[i].start &&
					    dev->resource[j].end ==
						res[i].end) {
						match++;
						break;
					}

			/* check if all address resources match */
			if (match == num_reg) {
				debug("connecting %s to %s\n",
					np->name, dev_name(dev));
				dev->device_node = np;
				free(res);
				return dev;
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

	resinval = (-1);

	debug("%s: register device %s, io=%pa\n",
			__func__, dev_name(dev),
		(num_reg) ? &dev->resource[0].start : &resinval);

	ret = platform_device_register(dev);
	if (!ret)
		return dev;

	free(dev);
	if (num_reg)
		free(res);
	return NULL;
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

#ifdef CONFIG_ARM_AMBA
static struct device_d *of_amba_device_create(struct device_node *np)
{
	struct amba_device *dev;
	int ret;

	debug("Creating amba device %s\n", np->full_name);

	if (!of_device_is_available(np))
		return NULL;

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

	return &dev->dev;

amba_err_free:
	free(dev);
	return NULL;
}
#else /* CONFIG_ARM_AMBA */
static inline struct amba_device *of_amba_device_create(struct device_node *np)
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
