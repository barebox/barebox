// SPDX-License-Identifier: GPL-2.0-only
/*
 * FPGA Bridge Framework Driver
 *
 *  Copyright (C) 2013-2016 Altera Corporation, All Rights Reserved.
 */

#include <common.h>
#include <fpga-bridge.h>

/**
 * fpga_bridge_enable - Enable transactions on the bridge
 *
 * @bridge: FPGA bridge
 *
 * Return: 0 for success, error code otherwise.
 */
int fpga_bridge_enable(struct fpga_bridge *bridge)
{
	dev_dbg(&bridge->dev, "enable\n");

	if (bridge->br_ops && bridge->br_ops->enable_set)
		return bridge->br_ops->enable_set(bridge, 1);

	return 0;
}
EXPORT_SYMBOL_GPL(fpga_bridge_enable);

/**
 * fpga_bridge_disable - Disable transactions on the bridge
 *
 * @bridge: FPGA bridge
 *
 * Return: 0 for success, error code otherwise.
 */
int fpga_bridge_disable(struct fpga_bridge *bridge)
{
	dev_dbg(&bridge->dev, "disable\n");

	if (bridge->br_ops && bridge->br_ops->enable_set)
		return bridge->br_ops->enable_set(bridge, 0);

	return 0;
}
EXPORT_SYMBOL_GPL(fpga_bridge_disable);

/**
 * of_fpga_bridge_get - get an exclusive reference to a fpga bridge
 *
 * @np: node pointer of a FPGA bridge
 *
 * Return fpga_bridge struct if successful.
 * Return -EBUSY if someone already has a reference to the bridge.
 * Return -ENODEV if @np is not a FPGA Bridge.
 */
struct fpga_bridge *of_fpga_bridge_get(struct device_node *np)

{
	struct device *dev;
	struct fpga_bridge *bridge;
	int ret = -EPROBE_DEFER;

	dev = of_find_device_by_node(np);
	if (!dev || !dev->priv)
		return ERR_PTR(ret);

	bridge = dev->priv;

	return bridge;
}
EXPORT_SYMBOL_GPL(of_fpga_bridge_get);

/**
 * fpga_bridges_enable - enable bridges in a list
 * @bridge_list: list of FPGA bridges
 *
 * Enable each bridge in the list.  If list is empty, do nothing.
 *
 * Return 0 for success or empty bridge list; return error code otherwise.
 */
int fpga_bridges_enable(struct list_head *bridge_list)
{
	struct fpga_bridge *bridge;
	int ret;

	list_for_each_entry(bridge, bridge_list, node) {
		ret = fpga_bridge_enable(bridge);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(fpga_bridges_enable);

/**
 * fpga_bridges_disable - disable bridges in a list
 *
 * @bridge_list: list of FPGA bridges
 *
 * Disable each bridge in the list.  If list is empty, do nothing.
 *
 * Return 0 for success or empty bridge list; return error code otherwise.
 */
int fpga_bridges_disable(struct list_head *bridge_list)
{
	struct fpga_bridge *bridge;
	int ret;

	list_for_each_entry(bridge, bridge_list, node) {
		ret = fpga_bridge_disable(bridge);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(fpga_bridges_disable);

/**
 * fpga_bridges_put - put bridges
 *
 * @bridge_list: list of FPGA bridges
 *
 * For each bridge in the list, put the bridge and remove it from the list.
 * If list is empty, do nothing.
 */
void fpga_bridges_put(struct list_head *bridge_list)
{
	struct fpga_bridge *bridge, *next;

	list_for_each_entry_safe(bridge, next, bridge_list, node)
		list_del(&bridge->node);
}
EXPORT_SYMBOL_GPL(fpga_bridges_put);

/**
 * fpga_bridges_get_to_list - get a bridge, add it to a list
 *
 * @np: node pointer of a FPGA bridge
 * @bridge_list: list of FPGA bridges
 *
 * Get an exclusive reference to the bridge and and it to the list.
 *
 * Return 0 for success, error code from of_fpga_bridge_get() othewise.
 */
int fpga_bridge_get_to_list(struct device_node *np,
			    struct list_head *bridge_list)
{
	struct fpga_bridge *bridge;

	bridge = of_fpga_bridge_get(np);
	if (IS_ERR(bridge))
		return PTR_ERR(bridge);
	list_add(&bridge->node, bridge_list);

	return 0;
}
EXPORT_SYMBOL_GPL(fpga_bridge_get_to_list);

static int set_enable(struct param_d *p, void *priv)
{
	struct fpga_bridge *bridge = priv;

	if (bridge->enable)
		fpga_bridge_enable(bridge);
	else
		fpga_bridge_disable(bridge);

	return 0;
}

/**
 * fpga_bridge_register - register a fpga bridge driver
 * @dev:	FPGA bridge device from pdev
 * @name:	FPGA bridge name
 * @br_ops:	pointer to structure of fpga bridge ops
 * @priv:	FPGA bridge private data
 *
 * Return: 0 for success, error code otherwise.
 */
int fpga_bridge_register(struct device *dev, const char *name,
			 const struct fpga_bridge_ops *br_ops, void *priv)
{
	struct fpga_bridge *bridge;
	int ret = 0;

	if (!name || !strlen(name)) {
		dev_err(dev, "Attempt to register with no name!\n");
		return -EINVAL;
	}

	bridge = xzalloc(sizeof(*bridge));
	if (!bridge)
		return -ENOMEM;

	INIT_LIST_HEAD(&bridge->node);

	bridge->br_ops = br_ops;
	bridge->priv = priv;

	bridge->dev.parent = dev;
	bridge->dev.of_node = dev->of_node;
	bridge->dev.id = DEVICE_ID_DYNAMIC;

	bridge->dev.name = xstrdup(name);

	ret = register_device(&bridge->dev);
	if (ret)
		goto out;

	dev->priv = bridge;

	bridge->enable = 0;
	dev_add_param_bool(&bridge->dev, "enable", set_enable,
			   NULL, &bridge->enable, bridge);

	of_platform_populate(dev->of_node, NULL, dev);

	dev_info(bridge->dev.parent, "fpga bridge [%s] registered\n",
		 bridge->dev.name);

	return 0;

out:
	kfree(bridge);

	return ret;
}
EXPORT_SYMBOL_GPL(fpga_bridge_register);
