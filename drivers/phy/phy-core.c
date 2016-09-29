/*
 * phy-core.c  --  Generic Phy framework.
 *
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <common.h>
#include <malloc.h>
#include <linux/phy/phy.h>
#include <usb/phy.h>

static LIST_HEAD(phy_provider_list);
static int phy_ida;

/**
 * phy_create() - create a new phy
 * @dev: device that is creating the new phy
 * @node: device node of the phy
 * @ops: function pointers for performing phy operations
 * @init_data: contains the list of PHY consumers or NULL
 *
 * Called to create a phy using phy framework.
 */
struct phy *phy_create(struct device_d *dev, struct device_node *node,
		       const struct phy_ops *ops,
		       struct phy_init_data *init_data)
{
	int ret;
	int id;
	struct phy *phy;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	phy = kzalloc(sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return ERR_PTR(-ENOMEM);

	id = phy_ida++;

	snprintf(phy->dev.name, MAX_DRIVER_NAME, "phy");
	phy->dev.id = id;
	phy->dev.parent = dev;
	phy->dev.device_node = node ?: dev->device_node;
	phy->id = id;
	phy->ops = ops;
	phy->init_data = init_data;

	ret = register_device(&phy->dev);
	if (ret)
		goto free_ida;

	return phy;

free_ida:
	phy_ida--;
	kfree(phy);
	return ERR_PTR(ret);
}

/**
 * __of_phy_provider_register() - create/register phy provider with the framework
 * @dev: struct device of the phy provider
 * @owner: the module owner containing of_xlate
 * @of_xlate: function pointer to obtain phy instance from phy provider
 *
 * Creates struct phy_provider from dev and of_xlate function pointer.
 * This is used in the case of dt boot for finding the phy instance from
 * phy provider.
 */
struct phy_provider *__of_phy_provider_register(struct device_d *dev,
	struct phy * (*of_xlate)(struct device_d *dev,
	struct of_phandle_args *args))
{
	struct phy_provider *phy_provider;

	phy_provider = kzalloc(sizeof(*phy_provider), GFP_KERNEL);
	if (!phy_provider)
		return ERR_PTR(-ENOMEM);

	phy_provider->dev = dev;
	phy_provider->of_xlate = of_xlate;

	list_add_tail(&phy_provider->list, &phy_provider_list);

	return phy_provider;
}

/**
 * of_phy_provider_unregister() - unregister phy provider from the framework
 * @phy_provider: phy provider returned by of_phy_provider_register()
 *
 * Removes the phy_provider created using of_phy_provider_register().
 */
void of_phy_provider_unregister(struct phy_provider *phy_provider)
{
	if (IS_ERR(phy_provider))
		return;

	list_del(&phy_provider->list);
	kfree(phy_provider);
}

int phy_init(struct phy *phy)
{
	int ret;

	if (!phy)
		return 0;

	if (phy->init_count == 0 && phy->ops->init) {
		ret = phy->ops->init(phy);
		if (ret < 0) {
			dev_err(&phy->dev, "phy init failed --> %d\n", ret);
			return ret;
		}
	}
	++phy->init_count;

	return 0;
}

int phy_exit(struct phy *phy)
{
	int ret;

	if (!phy)
		return 0;

	if (phy->init_count == 1 && phy->ops->exit) {
		ret = phy->ops->exit(phy);
		if (ret < 0) {
			dev_err(&phy->dev, "phy exit failed --> %d\n", ret);
			return ret;
		}
	}
	--phy->init_count;

	return 0;
}

int phy_power_on(struct phy *phy)
{
	int ret;

	if (!phy)
		return 0;

	if (phy->pwr) {
		ret = regulator_enable(phy->pwr);
		if (ret)
			return ret;
	}

	if (phy->power_count == 0 && phy->ops->power_on) {
		ret = phy->ops->power_on(phy);
		if (ret < 0) {
			dev_err(&phy->dev, "phy poweron failed --> %d\n", ret);
			goto out;
		}
	} else {
		ret = 0; /* Override possible ret == -ENOTSUPP */
	}
	++phy->power_count;

	return 0;

out:
	if (phy->pwr)
		regulator_disable(phy->pwr);

	return ret;
}

int phy_power_off(struct phy *phy)
{
	int ret;

	if (!phy)
		return 0;

	if (phy->power_count == 1 && phy->ops->power_off) {
		ret =  phy->ops->power_off(phy);
		if (ret < 0) {
			dev_err(&phy->dev, "phy poweroff failed --> %d\n", ret);
			return ret;
		}
	}
	--phy->power_count;

	if (phy->pwr)
		regulator_disable(phy->pwr);

	return 0;
}

struct usb_phy *phy_to_usbphy(struct phy *phy)
{
	if (!phy)
		return NULL;

	if (!phy->ops->to_usbphy)
		return ERR_PTR(-EINVAL);

	return phy->ops->to_usbphy(phy);
}

static struct phy_provider *of_phy_provider_lookup(struct device_node *node)
{
	struct phy_provider *phy_provider;
	struct device_node *child;

	list_for_each_entry(phy_provider, &phy_provider_list, list) {
		if (phy_provider->dev->device_node == node)
			return phy_provider;

		for_each_child_of_node(phy_provider->dev->device_node, child)
			if (child == node)
				return phy_provider;
	}

	return ERR_PTR(-ENODEV);
}

/**
 * _of_phy_get() - lookup and obtain a reference to a phy by phandle
 * @np: device_node for which to get the phy
 * @index: the index of the phy
 *
 * Returns the phy associated with the given phandle value,
 * after getting a refcount to it or -ENODEV if there is no such phy or
 * -EPROBE_DEFER if there is a phandle to the phy, but the device is
 * not yet loaded. This function uses of_xlate call back function provided
 * while registering the phy_provider to find the phy instance.
 */
static struct phy *_of_phy_get(struct device_node *np, int index)
{
	int ret;
	struct phy_provider *phy_provider;
	struct of_phandle_args args;

	ret = of_parse_phandle_with_args(np, "phys", "#phy-cells",
		index, &args);
	if (ret)
		return ERR_PTR(-ENODEV);

	phy_provider = of_phy_provider_lookup(args.np);
	if (IS_ERR(phy_provider)) {
		return ERR_PTR(-ENODEV);
	}

	return phy_provider->of_xlate(phy_provider->dev, &args);
}

/**
 * of_phy_get() - lookup and obtain a reference to a phy using a device_node.
 * @np: device_node for which to get the phy
 * @con_id: name of the phy from device's point of view
 *
 * Returns the phy driver, after getting a refcount to it; or
 * -ENODEV if there is no such phy. The caller is responsible for
 * calling phy_put() to release that count.
 */
struct phy *of_phy_get(struct device_node *np, const char *con_id)
{
	int index = 0;

	if (con_id)
		index = of_property_match_string(np, "phy-names", con_id);

	return _of_phy_get(np, index);
}

/**
 * of_phy_get_by_phandle() - lookup and obtain a reference to a phy.
 * @dev: device that requests this phy
 * @phandle - name of the property holding the phy phandle value
 * @index - the index of the phy
 *
 * Returns the phy driver, after getting a refcount to it; or
 * -ENODEV if there is no such phy. The caller is responsible for
 * calling phy_put() to release that count.
 */
struct phy *of_phy_get_by_phandle(struct device_d *dev, const char *phandle,
				  u8 index)
{
	struct device_node *np;
	struct phy_provider *phy_provider;

	if (!dev->device_node) {
		dev_dbg(dev, "device does not have a device node entry\n");
		return ERR_PTR(-EINVAL);
	}

	np = of_parse_phandle(dev->device_node, phandle, index);
	if (!np) {
		dev_dbg(dev, "failed to get %s phandle in %s node\n", phandle,
			dev->device_node->full_name);
		return ERR_PTR(-ENODEV);
	}

	phy_provider = of_phy_provider_lookup(np);
	if (IS_ERR(phy_provider)) {
		return ERR_PTR(-ENODEV);
	}

	return phy_provider->of_xlate(phy_provider->dev, NULL);
}

/**
 * phy_get() - lookup and obtain a reference to a phy.
 * @dev: device that requests this phy
 * @string: the phy name as given in the dt data or the name of the controller
 * port for non-dt case
 *
 * Returns the phy driver, after getting a refcount to it; or
 * -ENODEV if there is no such phy.  The caller is responsible for
 * calling phy_put() to release that count.
 */
struct phy *phy_get(struct device_d *dev, const char *string)
{
	int index = 0;
	struct phy *phy = ERR_PTR(-ENODEV);

	if (string == NULL) {
		dev_warn(dev, "missing string\n");
		return ERR_PTR(-EINVAL);
	}

	if (dev->device_node) {
		index = of_property_match_string(dev->device_node, "phy-names",
			string);
		phy = _of_phy_get(dev->device_node, index);
	}

	return phy;
}

/**
 * phy_optional_get() - lookup and obtain a reference to an optional phy.
 * @dev: device that requests this phy
 * @string: the phy name as given in the dt data or the name of the controller
 * port for non-dt case
 *
 * Returns the phy driver, after getting a refcount to it; or
 * NULL if there is no such phy.  The caller is responsible for
 * calling phy_put() to release that count.
 */
struct phy *phy_optional_get(struct device_d *dev, const char *string)
{
	struct phy *phy = phy_get(dev, string);

	if (PTR_ERR(phy) == -ENODEV)
		phy = NULL;

	return phy;
}

