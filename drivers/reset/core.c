/*
 * Reset Controller framework
 *
 * Copyright 2013 Philipp Zabel, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <common.h>
#include <gpio.h>
#include <malloc.h>
#include <of_gpio.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/reset.h>
#include <linux/reset-controller.h>

static LIST_HEAD(reset_controller_list);

/**
 * struct reset_control - a reset control
 * @rcdev: a pointer to the reset controller device
 *         this reset control belongs to
 * @id: ID of the reset controller in the reset
 *      controller device
 */
struct reset_control {
	struct reset_controller_dev *rcdev;

	int gpio;
	int gpio_active_high;

	struct device_d *dev;
	unsigned int id;
};

/**
 * of_reset_simple_xlate - translate reset_spec to the reset line number
 * @rcdev: a pointer to the reset controller device
 * @reset_spec: reset line specifier as found in the device tree
 * @flags: a flags pointer to fill in (optional)
 *
 * This simple translation function should be used for reset controllers
 * with 1:1 mapping, where reset lines can be indexed by number without gaps.
 */
static int of_reset_simple_xlate(struct reset_controller_dev *rcdev,
			  const struct of_phandle_args *reset_spec)
{
	if (WARN_ON(reset_spec->args_count != rcdev->of_reset_n_cells))
		return -EINVAL;

	if (reset_spec->args[0] >= rcdev->nr_resets)
		return -EINVAL;

	return reset_spec->args[0];
}

/**
 * reset_controller_register - register a reset controller device
 * @rcdev: a pointer to the initialized reset controller device
 */
int reset_controller_register(struct reset_controller_dev *rcdev)
{
	if (!rcdev->of_xlate) {
		rcdev->of_reset_n_cells = 1;
		rcdev->of_xlate = of_reset_simple_xlate;
	}

	list_add(&rcdev->list, &reset_controller_list);

	return 0;
}
EXPORT_SYMBOL_GPL(reset_controller_register);

/**
 * reset_controller_unregister - unregister a reset controller device
 * @rcdev: a pointer to the reset controller device
 */
void reset_controller_unregister(struct reset_controller_dev *rcdev)
{
	list_del(&rcdev->list);
}
EXPORT_SYMBOL_GPL(reset_controller_unregister);

/**
 * reset_control_reset - reset the controlled device
 * @rstc: reset controller
 */
int reset_control_reset(struct reset_control *rstc)
{
	if (!rstc)
		return 0;

	if (rstc->rcdev->ops->reset)
		return rstc->rcdev->ops->reset(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_reset);

/**
 * reset_control_assert - asserts the reset line
 * @rstc: reset controller
 */
int reset_control_assert(struct reset_control *rstc)
{
	if (!rstc)
		return 0;

	if (rstc->gpio >= 0)
		return gpio_direction_output(rstc->gpio, rstc->gpio_active_high);

	if (rstc->rcdev->ops->assert)
		return rstc->rcdev->ops->assert(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_assert);

/**
 * reset_control_deassert - deasserts the reset line
 * @rstc: reset controller
 */
int reset_control_deassert(struct reset_control *rstc)
{
	if (!rstc)
		return 0;

	if (rstc->gpio >= 0)
		return gpio_direction_output(rstc->gpio, !rstc->gpio_active_high);

	if (rstc->rcdev->ops->deassert)
		return rstc->rcdev->ops->deassert(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_deassert);

/**
 * of_reset_control_get - Lookup and obtain a reference to a reset controller.
 * @node: device to be reset by the controller
 * @id: reset line name
 *
 * Returns a struct reset_control or IS_ERR() condition containing errno.
 *
 * Use of id names is optional.
 */
static struct reset_control *of_reset_control_get(struct device_node *node,
						  const char *id)
{
	struct reset_control *rstc;
	struct reset_controller_dev *r, *rcdev;
	struct of_phandle_args args;
	int index = 0;
	int rstc_id;
	int ret;

	if (!of_get_property(node, "resets", NULL))
		return NULL;

	if (id)
		index = of_property_match_string(node,
						 "reset-names", id);
	ret = of_parse_phandle_with_args(node, "resets", "#reset-cells",
					 index, &args);
	if (ret)
		return ERR_PTR(ret);

	rcdev = NULL;
	list_for_each_entry(r, &reset_controller_list, list) {
		if (args.np == r->of_node) {
			rcdev = r;
			break;
		}
	}

	if (!rcdev) {
		return ERR_PTR(-ENODEV);
	}

	rstc_id = rcdev->of_xlate(rcdev, &args);
	if (rstc_id < 0)
		return ERR_PTR(rstc_id);

	rstc = kzalloc(sizeof(*rstc), GFP_KERNEL);
	if (!rstc)
		return ERR_PTR(-ENOMEM);

	rstc->rcdev = rcdev;
	rstc->id = rstc_id;
	rstc->gpio = -ENODEV;

	return rstc;
}

static struct reset_control *
gpio_reset_control_get(struct device_d *dev, const char *id)
{
	struct reset_control *rc;
	int gpio;
	enum of_gpio_flags flags;

	if (id)
		return ERR_PTR(-EINVAL);

	if (!of_get_property(dev->device_node, "reset-gpios", NULL))
		return NULL;

	gpio = of_get_named_gpio_flags(dev->device_node, "reset-gpios", 0, &flags);
	if (gpio < 0)
		return ERR_PTR(gpio);

	rc = xzalloc(sizeof(*rc));
	rc->gpio = gpio;
	rc->gpio_active_high = !(flags & OF_GPIO_ACTIVE_LOW);

	return rc;
}

/**
 * reset_control_get - Lookup and obtain a reference to a reset controller.
 * @dev: device to be reset by the controller
 * @id: reset line name
 *
 * Returns a struct reset_control or IS_ERR() condition containing errno.
 *
 * Use of id names is optional.
 */
struct reset_control *reset_control_get(struct device_d *dev, const char *id)
{
	struct reset_control *rstc;

	if (!dev)
		return ERR_PTR(-EINVAL);

	rstc = of_reset_control_get(dev->device_node, id);
	if (IS_ERR(rstc))
		return ERR_CAST(rstc);

	/*
	 * If there is no dedicated reset controller device, check if we have
	 * a reset line controlled by a GPIO instead.
	 */
	if (!rstc) {
		rstc = gpio_reset_control_get(dev, id);
		if (IS_ERR(rstc))
			return ERR_CAST(rstc);
	}

	if (!rstc)
		return NULL;

	rstc->dev = dev;

	return rstc;
}
EXPORT_SYMBOL_GPL(reset_control_get);

/**
 * reset_control_put - free the reset controller
 * @rstc: reset controller
 */

void reset_control_put(struct reset_control *rstc)
{
	if (IS_ERR_OR_NULL(rstc))
		return;

	kfree(rstc);
}
EXPORT_SYMBOL_GPL(reset_control_put);

/**
 * device_reset - find reset controller associated with the device
 *                and perform reset
 * @dev: device to be reset by the controller
 *
 * Convenience wrapper for reset_control_get() and reset_control_reset().
 * This is useful for the common case of devices with single, dedicated reset
 * lines.
 */
int device_reset(struct device_d *dev)
{
	struct reset_control *rstc;
	int ret;

	rstc = reset_control_get(dev, NULL);
	if (IS_ERR(rstc)) {
		if (PTR_ERR(rstc) == -ENOENT)
			return 0;

		return PTR_ERR(rstc);
	}

	ret = reset_control_reset(rstc);

	reset_control_put(rstc);

	return ret;
}
EXPORT_SYMBOL_GPL(device_reset);

int device_reset_us(struct device_d *dev, int us)
{
	struct reset_control *rstc;
	int ret;

	rstc = reset_control_get(dev, NULL);
	if (IS_ERR(rstc)) {
		if (PTR_ERR(rstc) == -ENOENT)
			return 0;

		return PTR_ERR(rstc);
	}

	ret = reset_control_assert(rstc);
	if (ret)
		return ret;

	udelay(us);

	ret = reset_control_deassert(rstc);
	if (ret)
		return ret;

	reset_control_put(rstc);

	return ret;
}
EXPORT_SYMBOL_GPL(device_reset_us);
