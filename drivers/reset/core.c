// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Reset Controller framework
 *
 * Copyright 2013 Philipp Zabel, Pengutronix
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

	struct device *dev;
	unsigned int id;
	bool array;
};

/**
 * struct reset_control_array - an array of reset controls
 * @base: reset control for compatibility with reset control API functions
 * @num_rstcs: number of reset controls
 * @rstc: array of reset controls
 */
struct reset_control_array {
	struct reset_control base;
	unsigned int num_rstcs;
	struct reset_control *rstc[];
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
 * reset_control_status - returns a negative errno if not supported, a
 * positive value if the reset line is asserted, or zero if the reset
 * line is not asserted or if the desc is NULL (optional reset).
 * @rstc: reset controller
 */
int reset_control_status(struct reset_control *rstc)
{
	if (!rstc)
		return 0;

	if (WARN_ON(IS_ERR(rstc)))
		return -EINVAL;

	if (rstc->rcdev->ops->status)
		return rstc->rcdev->ops->status(rstc->rcdev, rstc->id);

	return -ENOTSUPP;
}
EXPORT_SYMBOL_GPL(reset_control_status);

static inline struct reset_control_array *
rstc_to_array(struct reset_control *rstc) {
	return container_of(rstc, struct reset_control_array, base);
}

static int reset_control_array_reset(struct reset_control_array *resets)
{
	int ret, i;

	for (i = 0; i < resets->num_rstcs; i++) {
		ret = reset_control_reset(resets->rstc[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int reset_control_array_assert(struct reset_control_array *resets)
{
	int ret, i;

	for (i = 0; i < resets->num_rstcs; i++) {
		ret = reset_control_assert(resets->rstc[i]);
		if (ret)
			goto err;
	}

	return 0;

err:
	while (i--)
		reset_control_deassert(resets->rstc[i]);
	return ret;
}

static int reset_control_array_deassert(struct reset_control_array *resets)
{
	int ret, i;

	for (i = 0; i < resets->num_rstcs; i++) {
		ret = reset_control_deassert(resets->rstc[i]);
		if (ret)
			goto err;
	}

	return 0;

err:
	while (i--)
		reset_control_assert(resets->rstc[i]);
	return ret;
}

static inline bool reset_control_is_array(struct reset_control *rstc)
{
	return rstc->array;
}

/**
 * reset_control_reset - reset the controlled device
 * @rstc: reset controller
 */
int reset_control_reset(struct reset_control *rstc)
{
	if (!rstc)
		return 0;

	if (reset_control_is_array(rstc))
		return reset_control_array_reset(rstc_to_array(rstc));

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

	if (reset_control_is_array(rstc))
		return reset_control_array_assert(rstc_to_array(rstc));

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

	if (reset_control_is_array(rstc))
		return reset_control_array_deassert(rstc_to_array(rstc));

	if (rstc->gpio >= 0)
		return gpio_direction_output(rstc->gpio, !rstc->gpio_active_high);

	if (rstc->rcdev->ops->deassert)
		return rstc->rcdev->ops->deassert(rstc->rcdev, rstc->id);

	return -ENOSYS;
}
EXPORT_SYMBOL_GPL(reset_control_deassert);

/**
 * reset_control_get_count - Count reset lines
 * @dev: device
 *
 * Returns number of resets, 0 if none specified
 */
int reset_control_get_count(struct device *dev)
{
	return of_count_phandle_with_args(dev->of_node, "resets",
					  "#reset-cells");
}

/**
 * of_reset_control_get_by_index - Lookup and obtain a reference to a reset controller.
 * @node: device to be reset by the controller
 * @index: reset line index
 *
 * Returns a struct reset_control or IS_ERR() condition containing errno.
 */
static struct reset_control *of_reset_control_get_by_index(struct device_node *node,
							   int index)
{
	struct reset_control *rstc;
	struct reset_controller_dev *r, *rcdev;
	struct of_phandle_args args;
	int rstc_id;
	int ret;

	if (!of_get_property(node, "resets", NULL))
		return NULL;

	ret = of_parse_phandle_with_args(node, "resets", "#reset-cells",
					 index, &args);
	if (ret)
		return ERR_PTR(ret);

	/* Ignore error, as CLK_OF_DECLARE resets have no proper driver. */
	of_device_ensure_probed(args.np);

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

/**
 * of_reset_control_get - Lookup and obtain a reference to a reset controller.
 * @node: device to be reset by the controller
 * @id: reset line name
 *
 * Returns a struct reset_control or IS_ERR() condition containing errno.
 *
 * Use of id names is optional.
 */
struct reset_control *of_reset_control_get(struct device_node *node,
					   const char *id)
{
	int index = 0;

	if (id) {
		index = of_property_match_string(node, "reset-names", id);
		if (index < 0)
			return ERR_PTR(-ENOENT);
	}

	return of_reset_control_get_by_index(node, index);
}

static struct reset_control *
gpio_reset_control_get(struct device *dev, const char *id)
{
	struct reset_control *rc;
	int gpio;
	enum of_gpio_flags flags;

	if (id)
		return ERR_PTR(-EINVAL);

	if (!of_get_property(dev->of_node, "reset-gpios", NULL))
		return NULL;

	gpio = of_get_named_gpio_flags(dev->of_node, "reset-gpios", 0, &flags);
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
struct reset_control *reset_control_get(struct device *dev, const char *id)
{
	struct reset_control *rstc;

	if (!dev)
		return ERR_PTR(-EINVAL);

	rstc = of_reset_control_get(dev->of_node, id);
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
int device_reset(struct device *dev)
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

/*
 * APIs to manage an array of reset controls.
 */

/**
 * reset_control_array_get - Get a list of reset controls
 *
 * @dev: device that requests the reset controls array
 *
 * Returns pointer to allocated reset_control on success or error on failure
 */
struct reset_control *reset_control_array_get(struct device *dev)
{
	struct reset_control_array *resets;
	struct reset_control *rstc;
	struct device_node *np = dev->of_node;
	int num, i;

	num = reset_control_get_count(dev);
	if (num < 0)
		return ERR_PTR(num);

	resets = kzalloc(struct_size(resets, rstc, num), GFP_KERNEL);
	if (!resets)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < num; i++) {
		rstc = of_reset_control_get_by_index(np, i);
		if (IS_ERR(rstc))
			goto err_rst;
		resets->rstc[i] = rstc;
	}
	resets->num_rstcs = num;
	resets->base.array = true;

	return &resets->base;

err_rst:
	while (--i >= 0)
		reset_control_put(resets->rstc[i]);

	kfree(resets);

	return rstc;
}

int device_reset_all(struct device *dev)
{
	struct reset_control *rstc;
	int ret, i;

	for (i = 0; i < reset_control_get_count(dev); i++) {
		int ret;

		rstc = of_reset_control_get_by_index(dev->of_node, i);
		if (IS_ERR(rstc))
			return PTR_ERR(rstc);

		ret = reset_control_reset(rstc);
		if (ret)
			return ret;

		reset_control_put(rstc);
	}

	if (i == 0) {
		rstc = gpio_reset_control_get(dev, NULL);

		ret = reset_control_reset(rstc);
		if (ret)
			return ret;

		reset_control_put(rstc);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(device_reset_all);

int device_reset_us(struct device *dev, int us)
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
