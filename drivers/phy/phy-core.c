// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * phy-core.c  --  Generic Phy framework.
 *
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <common.h>
#include <malloc.h>
#include <linux/phy/phy.h>
#include <linux/usb/phy.h>

static LIST_HEAD(phy_provider_list);
static int phy_ida;

#define for_each_phy(p) list_for_each_entry(p, &phy_class.devices, dev.class_list)

DEFINE_DEV_CLASS(phy_class, "phy");

static const char *phy_mode_tostr(struct phy *phy)
{
	switch (phy->attrs.mode) {
	case PHY_MODE_INVALID:
		return "invalid";
	case PHY_MODE_USB_HOST:
		return "usb-host";
	case PHY_MODE_USB_HOST_LS:
		return "usb-host-ls";
	case PHY_MODE_USB_HOST_FS:
		return "usb-host-fs";
	case PHY_MODE_USB_HOST_HS:
		return "usb-host-hs";
	case PHY_MODE_USB_HOST_SS:
		return "usb-host-ss";
	case PHY_MODE_USB_DEVICE:
		return "usb-device";
	case PHY_MODE_USB_DEVICE_LS:
		return "usb-device-ls";
	case PHY_MODE_USB_DEVICE_FS:
		return "usb-device-fs";
	case PHY_MODE_USB_DEVICE_HS:
		return "usb-device-hs";
	case PHY_MODE_USB_DEVICE_SS:
		return "usb-device-ss";
	case PHY_MODE_USB_OTG:
		return "usb-otg";
	case PHY_MODE_UFS_HS_A:
		return "ufs-hs-a";
	case PHY_MODE_UFS_HS_B:
		return "ufs-hs-b";
	case PHY_MODE_PCIE:
		return "pcie";
	case PHY_MODE_ETHERNET:
		return "ethernet";
	case PHY_MODE_MIPI_DPHY:
		return "mipi-dphy";
	case PHY_MODE_SATA:
		return "sata";
	case PHY_MODE_LVDS:
		return "lvds";
	case PHY_MODE_DP:
		return "dp";
	}

	return "unknown";
}

static void phy_devinfo(struct device *dev)
{
	struct phy *phy = container_of(dev, struct phy, dev);

	printf("PHY info:\n");
	if (phy->attrs.mode != PHY_MODE_INVALID)
		printf("  mode: %s\n", phy_mode_tostr(phy));
	printf("  init count: %u\n", phy->init_count);
	printf("  power count: %u\n", phy->power_count);
}

/**
 * phy_create() - create a new phy
 * @dev: device that is creating the new phy
 * @node: device node of the phy
 * @ops: function pointers for performing phy operations
 *
 * Called to create a phy using phy framework.
 */
struct phy *phy_create(struct device *dev, struct device_node *node,
		       const struct phy_ops *ops)
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

	phy->dev.id = id;
	phy->dev.parent = dev;
	phy->dev.of_node = node ?: dev->of_node;
	phy->id = id;
	phy->ops = ops;

	/* phy-supply */
	phy->pwr = regulator_get(&phy->dev, "phy");
	if (IS_ERR(phy->pwr)) {
		ret = PTR_ERR(phy->pwr);
		if (ret == -EPROBE_DEFER)
			goto free_ida;

		phy->pwr = NULL;
	}

	ret = class_register_device(&phy_class, &phy->dev, "phy");
	if (ret)
		goto free_ida;

	devinfo_add(&phy->dev, phy_devinfo);

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
struct phy_provider *__of_phy_provider_register(struct device *dev,
						struct phy * (*of_xlate)(struct device *dev,
							const struct of_phandle_args *args))
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

int phy_set_mode_ext(struct phy *phy, enum phy_mode mode, int submode)
{
	int ret;

	if (!phy || !phy->ops->set_mode)
		return 0;

	ret = phy->ops->set_mode(phy, mode, submode);
	if (!ret)
		phy->attrs.mode = mode;

	return ret;
}
EXPORT_SYMBOL_GPL(phy_set_mode_ext);

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
	int ret;

	ret = of_device_ensure_probed(node);
	if (ret)
		return ERR_PTR(ret);

	list_for_each_entry(phy_provider, &phy_provider_list, list) {
		if (phy_provider->dev->of_node == node)
			return phy_provider;

		for_each_child_of_node(phy_provider->dev->of_node, child)
			if (child == node)
				return phy_provider;
	}

	return ERR_PTR(-EPROBE_DEFER);
}

/**
 * of_phy_simple_xlate() - returns the phy instance from phy provider
 * @dev: the PHY provider device
 * @args: of_phandle_args (not used here)
 *
 * Intended to be used by phy provider for the common case where #phy-cells is
 * 0. For other cases where #phy-cells is greater than '0', the phy provider
 * should provide a custom of_xlate function that reads the *args* and returns
 * the appropriate phy.
 */
struct phy *of_phy_simple_xlate(struct device *dev,
				const struct of_phandle_args *args)
{
	struct phy *phy;

	for_each_phy(phy) {
		if (args->np != phy->dev.of_node)
			continue;

		return phy;
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
		return ERR_CAST(phy_provider);
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
struct phy *of_phy_get_by_phandle(struct device *dev, const char *phandle,
				  u8 index)
{
	struct device_node *np;
	struct phy_provider *phy_provider;

	if (!dev->of_node) {
		dev_dbg(dev, "device does not have a device node entry\n");
		return ERR_PTR(-EINVAL);
	}

	np = of_parse_phandle(dev->of_node, phandle, index);
	if (!np) {
		dev_dbg(dev, "failed to get %s phandle in %pOF node\n", phandle,
			dev->of_node);
		return ERR_PTR(-ENODEV);
	}

	phy_provider = of_phy_provider_lookup(np);
	if (IS_ERR(phy_provider)) {
		return ERR_CAST(phy_provider);
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
struct phy *phy_get(struct device *dev, const char *string)
{
	int index = 0;
	struct phy *phy = ERR_PTR(-ENODEV);

	if (string == NULL) {
		dev_warn(dev, "missing string\n");
		return ERR_PTR(-EINVAL);
	}

	if (dev->of_node) {
		index = of_property_match_string(dev->of_node, "phy-names",
						 string);
		phy = _of_phy_get(dev->of_node, index);
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
struct phy *phy_optional_get(struct device *dev, const char *string)
{
	struct phy *phy = phy_get(dev, string);

	if (PTR_ERR(phy) == -ENODEV)
		phy = NULL;

	return phy;
}

/**
 * phy_get_by_index() - lookup and obtain a reference to a phy by index.
 * @dev: device with node that references this phy
 * @index: index of the phy
 *
 * Gets the phy using _of_phy_get()
 */
struct phy *phy_get_by_index(struct device *dev, int index)
{
	if (!dev->of_node)
		return ERR_PTR(-ENODEV);

	return _of_phy_get(dev->of_node, index);
}
