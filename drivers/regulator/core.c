// SPDX-License-Identifier: GPL-2.0-only
/*
 * barebox regulator support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <regulator.h>
#include <of.h>
#include <malloc.h>
#include <linux/err.h>
#include <deep-probe.h>

static LIST_HEAD(regulator_list);

struct regulator {
	struct regulator_dev *rdev;
	struct regulator_dev *rdev_consumer;
	struct list_head list;
	struct device *dev;
};

const char *rdev_get_name(struct regulator_dev *rdev)
{
	if (rdev->name)
		return rdev->name;

	return "";
}

static int regulator_map_voltage(struct regulator_dev *rdev, int min_uV,
				 int max_uV)
{
	if (rdev->desc->ops->list_voltage == regulator_list_voltage_linear)
		return regulator_map_voltage_linear(rdev, min_uV, max_uV);

	if (rdev->desc->ops->list_voltage == regulator_list_voltage_linear_range)
		return regulator_map_voltage_linear_range(rdev, min_uV, max_uV);

	return regulator_map_voltage_iterate(rdev, min_uV, max_uV);
}

static int regulator_enable_rdev(struct regulator_dev *rdev)
{
	int ret;

	if (rdev->enable_count) {
		rdev->enable_count++;
		return 0;
	}

	if (!rdev->desc->ops->enable)
		return -ENOSYS;

	/* turn on parent regulator */
	ret = regulator_enable(rdev->supply);
	if (ret)
		return ret;

	ret = rdev->desc->ops->enable(rdev);
	if (ret) {
		regulator_disable(rdev->supply);
		return ret;
	}

	if (rdev->enable_time_us)
		udelay(rdev->enable_time_us);

	rdev->enable_count++;

	return 0;
}

static int regulator_disable_rdev(struct regulator_dev *rdev)
{
	int ret;

	if (!rdev->enable_count)
		return -EINVAL;

	if (rdev->enable_count > 1) {
		rdev->enable_count--;
		return 0;
	}

	/* Crisis averted, be loud about the unbalanced regulator use */
	if (WARN_ON(rdev->always_on))
		return -EPERM;

	if (!rdev->desc->ops->disable)
		return -ENOSYS;

	ret = rdev->desc->ops->disable(rdev);
	if (ret)
		return ret;

	rdev->enable_count--;

	return regulator_disable(rdev->supply);
}

static int regulator_set_voltage_rdev(struct regulator_dev *rdev,
					  int min_uV, int max_uV)
{
	const struct regulator_ops *ops = rdev->desc->ops;
	unsigned int selector;
	int best_val = 0;
	int ret;

	if (ops->set_voltage_sel) {
		ret = regulator_map_voltage(rdev, min_uV, max_uV);
		if (ret >= 0) {
			best_val = ops->list_voltage(rdev, ret);
			if (min_uV <= best_val && max_uV >= best_val) {
				selector = ret;
				ret = ops->set_voltage_sel(rdev, selector);
			} else {
				ret = -EINVAL;
			}
		}

		return ret;
	}

	return -ENOSYS;
}

static int regulator_get_voltage_rdev(struct regulator_dev *rdev)
{
	int sel, ret;

	if (rdev->desc->ops->get_voltage_sel) {
		sel = rdev->desc->ops->get_voltage_sel(rdev);
		if (sel < 0)
			return sel;
		ret = rdev->desc->ops->list_voltage(rdev, sel);
	} else if (rdev->desc->ops->get_voltage) {
		ret = rdev->desc->ops->get_voltage(rdev);
	} else if (rdev->desc->ops->list_voltage) {
		ret = rdev->desc->ops->list_voltage(rdev, 0);
	} else if (rdev->desc->fixed_uV && (rdev->desc->n_voltages == 1)) {
		ret = rdev->desc->fixed_uV;
	} else if (rdev->min_uv && rdev->min_uv == rdev->max_uv) {
		ret = rdev->min_uv;
	} else if (rdev->supply) {
		ret = regulator_get_voltage(rdev->supply);
	} else {
		return -EINVAL;
	}

	return ret;
}

static int regulator_resolve_supply(struct regulator_dev *rdev)
{
	struct regulator *supply;
	const char *supply_name;

	if (!rdev || rdev->supply)
		return 0;

	supply_name = rdev->desc->supply_name;
	if (!supply_name)
		return 0;

	rdev_dbg(rdev, "resolving %s\n", supply_name);

	supply = regulator_get(rdev->dev, supply_name);
	if (IS_ERR(supply)) {
		if (deep_probe_is_supported())
			return PTR_ERR(supply);

		/* For historic reasons, some regulator consumers don't handle
		 * -EPROBE_DEFER (e.g. vmmc-supply). If we now start propagating
		 * parent EPROBE_DEFER, previously requested vmmc-supply with
		 * always-on parent that worked before will end up not being
		 * requested breaking MMC use. So for non-deep probe systems,
		 * just make best effort to resolve, but don't fail the get if
		 * we couldn't. If you want to get rid of this warning, consider
		 * migrating your platform to have deep probe support.
		 */
		rdev_warn(rdev, "Failed to get '%s' regulator (ignored).\n",
			 supply_name);
		return 0;
	}

	if (supply)
		supply->rdev_consumer = rdev;

	rdev->supply = supply;

	return 0;
}

static int regulator_init_voltage(struct regulator_dev *rdev)
{
	int target_min, target_max, current_uV, ret;

	if (!rdev->min_uv || !rdev->max_uv)
		return 0;

	current_uV = regulator_get_voltage_rdev(rdev);
	if (current_uV < 0) {
		/* This regulator can't be read and must be initialized */
		rdev_info(rdev, "Setting %d-%duV\n", rdev->min_uv, rdev->max_uv);
		regulator_set_voltage_rdev(rdev, rdev->min_uv, rdev->max_uv);
		current_uV = regulator_get_voltage_rdev(rdev);
	}

	if (current_uV < 0) {
		if (current_uV != -EPROBE_DEFER)
			rdev_err(rdev,
				 "failed to get the current voltage: %pe\n",
				 ERR_PTR(current_uV));
		return current_uV;
	}

	/*
	 * If we're below the minimum voltage move up to the
	 * minimum voltage, if we're above the maximum voltage
	 * then move down to the maximum.
	 */
	target_min = current_uV;
	target_max = current_uV;

	if (current_uV < rdev->min_uv) {
		target_min = rdev->min_uv;
		target_max = rdev->min_uv;
	}

	if (current_uV > rdev->max_uv) {
		target_min = rdev->max_uv;
		target_max = rdev->max_uv;
	}

	if (target_min != current_uV || target_max != current_uV) {
		rdev_info(rdev, "Bringing %duV into %d-%duV\n",
			  current_uV, target_min, target_max);
		ret = regulator_set_voltage_rdev(rdev, target_min, target_max);
		if (ret < 0) {
			rdev_err(rdev,
				"failed to apply %d-%duV constraint: %pe\n",
				target_min, target_max, ERR_PTR(ret));
			return ret;
		}
	}

	return 0;
}

static int __regulator_register(struct regulator_dev *rdev, const char *name)
{
	int ret;

	INIT_LIST_HEAD(&rdev->consumer_list);

	list_add_tail(&rdev->list, &regulator_list);

	if (name)
		rdev->name = xstrdup(name);

	ret = regulator_init_voltage(rdev);
	if (ret)
		goto err;

	if (rdev->boot_on || rdev->always_on) {
		ret = regulator_resolve_supply(rdev);
		if (ret < 0)
			goto err;

		ret = regulator_enable_rdev(rdev);
		if (ret && ret != -ENOSYS)
			goto err;
	}

	return 0;
err:
	list_del(&rdev->list);
	free((char *)rdev->name);

	return ret;
}


#ifdef CONFIG_OFDEVICE
/*
 * of_regulator_register - register a regulator corresponding to a device_node
 * rdev:		the regulator device providing the ops
 * @node:       the device_node this regulator corresponds to
 *
 * Return: 0 for success or a negative error code
 */
int of_regulator_register(struct regulator_dev *rdev, struct device_node *node)
{
	const char *name;
	int ret;

	if (!rdev || !node)
		return -EINVAL;

	rdev->boot_on = of_property_read_bool(node, "regulator-boot-on");
	rdev->always_on = of_property_read_bool(node, "regulator-always-on");

	name = of_get_property(node, "regulator-name", NULL);
	if (!name)
		name = node->name;

	rdev->node = node;
	node->dev = rdev->dev;

	if (rdev->desc->off_on_delay)
		rdev->enable_time_us = rdev->desc->off_on_delay;

	if (rdev->desc->fixed_uV && rdev->desc->n_voltages == 1)
		rdev->min_uv = rdev->max_uv = rdev->desc->fixed_uV;

	of_property_read_u32(node, "regulator-enable-ramp-delay",
			&rdev->enable_time_us);
	of_property_read_u32(node, "regulator-min-microvolt",
			&rdev->min_uv);
	of_property_read_u32(node, "regulator-max-microvolt",
			&rdev->max_uv);

	ret = __regulator_register(rdev, name);
	if (ret)
		return ret;

	return 0;
}

static struct regulator_dev *of_regulator_get(struct device *dev,
						   const char *supply)
{
	char *propname;
	struct regulator_dev *rdev;
	struct device_node *node, *node_parent;
	int ret;

	/*
	 * If the device does have a device node return the dummy regulator.
	 */
	if (!dev->of_node)
		return NULL;

	propname = basprintf("%s-supply", supply);

	/*
	 * If the device node does not contain a supply property, this device doesn't
	 * need a regulator. Return the dummy regulator in this case.
	 */
	if (!of_get_property(dev->of_node, propname, NULL)) {
		dev_dbg(dev, "No %s-supply node found, using dummy regulator\n",
				supply);
		rdev = NULL;
		goto out;
	}

	/*
	 * The device node specifies a supply, so it's mandatory. Return an error when
	 * something goes wrong below.
	 */
	node = of_parse_phandle(dev->of_node, propname, 0);
	if (!node) {
		dev_dbg(dev, "No %s node found\n", propname);
		rdev = ERR_PTR(-EINVAL);
		goto out;
	}

	ret = of_device_ensure_probed(node);
	if (ret) {
		/*
		 * If "barebox,allow-dummy-supply" property is set for regulator
		 * provider allow use of dummy regulator (NULL is returned).
		 * Check regulator node and its parent if this setting is set
		 * PMIC wide.
		 */
		node_parent = of_get_parent(node);
		if (of_get_property(node, "barebox,allow-dummy-supply", NULL) ||
		    of_get_property(node_parent, "barebox,allow-dummy-supply", NULL)) {
			dev_dbg(dev, "Allow use of dummy regulator for " \
				"%s-supply\n", supply);
			rdev = NULL;
			goto out;
		}

		rdev = ERR_PTR(ret);
		goto out;
	}

	list_for_each_entry(rdev, &regulator_list, list) {
		if (rdev->node == node) {
			dev_dbg(dev, "Using %s regulator from %pOF\n",
					propname, node);
			goto out;
		}
	}

	/*
	 * It is possible that regulator we are looking for will be
	 * added in future initcalls, so, instead of reporting a
	 * complete failure report probe deferral
	 */
	rdev = ERR_PTR(-EPROBE_DEFER);
out:
	free(propname);

	return rdev;
}
#else
static struct regulator_dev *of_regulator_get(struct device *dev,
						   const char *supply)
{
	return NULL;
}
#endif

int dev_regulator_register(struct regulator_dev *rdev, const char *name)
{
	int ret;

	ret = __regulator_register(rdev, name);
	if (ret)
		return ret;

	return 0;
}

static struct regulator_dev *dev_regulator_get(struct device *dev,
					       const char *supply)
{
	struct regulator_dev *rdev;
	struct regulator_dev *ret = NULL;
	int match, best = 0;
	const char *dev_id = dev ? dev_name(dev) : NULL;

	list_for_each_entry(rdev, &regulator_list, list) {
		match = 0;
		if (rdev->name) {
			if (!dev_id || strcmp(rdev->name, dev_id))
				continue;
			match += 2;
		}
		if (rdev->desc->supply_name) {
			if (!supply || strcmp(rdev->desc->supply_name, supply))
				continue;
			match += 1;
		}

		if (match > best) {
			ret = rdev;
			if (match != 3)
				best = match;
			else
				break;
		}
	}

	return ret;
}

/*
 * regulator_get - get the supply for a device.
 * @dev:	the device a supply is requested for
 * @supply:     the supply name
 *
 * This returns a supply for a device. Check the result with IS_ERR().
 * NULL is a valid regulator, the dummy regulator.
 *
 * Return: a regulator object or an error pointer
 */
struct regulator *regulator_get(struct device *dev, const char *supply)
{
	struct regulator_dev *rdev = NULL;
	struct regulator *r;
	int ret;

	if (dev->of_node && supply) {
		rdev = of_regulator_get(dev, supply);
		if (IS_ERR(rdev))
			return ERR_CAST(rdev);
	}

	if (!rdev) {
		rdev = dev_regulator_get(dev, supply);
		if (IS_ERR(rdev))
			return ERR_CAST(rdev);
	}

	if (!rdev)
		return NULL;

	ret = regulator_resolve_supply(rdev);
	if (ret < 0)
		return ERR_PTR(ret);

	r = xzalloc(sizeof(*r));
	r->rdev = rdev;
	r->dev = dev;

	list_add_tail(&r->list, &rdev->consumer_list);

	return r;
}

void regulator_put(struct regulator *r)
{
	if (IS_ERR_OR_NULL(r))
		return;
	list_del(&r->list);
	free(r);
}

static struct regulator_dev *regulator_by_name(const char *name)
{
	struct regulator_dev *rdev;

	list_for_each_entry(rdev, &regulator_list, list) {
		if (rdev->name && !strcmp(rdev->name, name))
			return rdev;
	}

	return NULL;
}

struct regulator *regulator_get_name(const char *name)
{
	struct regulator_dev *rdev;
	struct regulator *r;

	rdev = regulator_by_name(name);
	if (!rdev)
		return ERR_PTR(-ENODEV);

	r = xzalloc(sizeof(*r));
	r->rdev = rdev;

	list_add_tail(&r->list, &rdev->consumer_list);

	return r;
}

/*
 * regulator_enable - enable a regulator.
 * @r:		the regulator to enable
 *
 * This enables a regulator. Regulators are reference counted, only the
 * first enable operation will enable the regulator.
 *
 * Return: 0 for success or a negative error code
 */
int regulator_enable(struct regulator *r)
{
	if (!r)
		return 0;

	return regulator_enable_rdev(r->rdev);
}

/*
 * regulator_disable - disable a regulator.
 * @r:		the regulator to disable
 *
 * This disables a regulator. Regulators are reference counted, only the
 * when balanced with regulator_enable the regulator will be disabled.
 *
 * Return: 0 for success or a negative error code
 */
int regulator_disable(struct regulator *r)
{
	if (!r)
		return 0;

	return regulator_disable_rdev(r->rdev);
}

int regulator_set_voltage(struct regulator *r, int min_uV, int max_uV)
{
	if (!r)
		return 0;

	return regulator_set_voltage_rdev(r->rdev, min_uV, max_uV);
}

/**
 * regulator_bulk_get - get multiple regulator consumers
 *
 * @dev:           Device to supply
 * @num_consumers: Number of consumers to register
 * @consumers:     Configuration of consumers; clients are stored here.
 *
 * @return 0 on success, an errno on failure.
 *
 * This helper function allows drivers to get several regulator
 * consumers in one operation.  If any of the regulators cannot be
 * acquired then any regulators that were allocated will be freed
 * before returning to the caller.
 */
int regulator_bulk_get(struct device *dev, int num_consumers,
		       struct regulator_bulk_data *consumers)
{
	int i;
	int ret;

	for (i = 0; i < num_consumers; i++)
		consumers[i].consumer = NULL;

	for (i = 0; i < num_consumers; i++) {
		consumers[i].consumer = regulator_get(dev,
						      consumers[i].supply);
		if (IS_ERR(consumers[i].consumer)) {
			ret = PTR_ERR(consumers[i].consumer);
			consumers[i].consumer = NULL;
			goto err;
		}
	}

	return 0;

err:
	if (ret != -EPROBE_DEFER)
		dev_err(dev, "Failed to get supply '%s': %d\n",
			consumers[i].supply, ret);
	else
		dev_dbg(dev, "Failed to get supply '%s', deferring\n",
			consumers[i].supply);

	return ret;
}
EXPORT_SYMBOL_GPL(regulator_bulk_get);

/**
 * regulator_bulk_enable - enable multiple regulator consumers
 *
 * @num_consumers: Number of consumers
 * @consumers:     Consumer data; clients are stored here.
 * @return         0 on success, an errno on failure
 *
 * This convenience API allows consumers to enable multiple regulator
 * clients in a single API call.  If any consumers cannot be enabled
 * then any others that were enabled will be disabled again prior to
 * return.
 */
int regulator_bulk_enable(int num_consumers,
			  struct regulator_bulk_data *consumers)
{
	int ret;
	int i;

	for (i = 0; i < num_consumers; i++) {
		ret = regulator_enable(consumers[i].consumer);
		if (ret)
			goto err;
	}

	return 0;

err:
	while (--i >= 0)
		regulator_disable(consumers[i].consumer);

	return ret;
}
EXPORT_SYMBOL_GPL(regulator_bulk_enable);

/**
 * regulator_bulk_disable - disable multiple regulator consumers
 *
 * @num_consumers: Number of consumers
 * @consumers:     Consumer data; clients are stored here.
 * @return         0 on success, an errno on failure
 *
 * This convenience API allows consumers to disable multiple regulator
 * clients in a single API call.  If any consumers cannot be disabled
 * then any others that were disabled will be enabled again prior to
 * return.
 */
int regulator_bulk_disable(int num_consumers,
			   struct regulator_bulk_data *consumers)
{
	int i;
	int ret, r;

	for (i = num_consumers - 1; i >= 0; --i) {
		ret = regulator_disable(consumers[i].consumer);
		if (ret != 0)
			goto err;
	}

	return 0;

err:
	pr_err("Failed to disable %s: %d\n", consumers[i].supply, ret);
	for (++i; i < num_consumers; ++i) {
		r = regulator_enable(consumers[i].consumer);
		if (r != 0)
			pr_err("Failed to re-enable %s: %d\n",
			       consumers[i].supply, r);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(regulator_bulk_disable);

/**
 * regulator_bulk_free - free multiple regulator consumers
 *
 * @num_consumers: Number of consumers
 * @consumers:     Consumer data; clients are stored here.
 *
 * This convenience API allows consumers to free multiple regulator
 * clients in a single API call.
 */
void regulator_bulk_free(int num_consumers,
			 struct regulator_bulk_data *consumers)
{
	int i;

	for (i = 0; i < num_consumers; i++)
		consumers[i].consumer = NULL;
}
EXPORT_SYMBOL_GPL(regulator_bulk_free);

int regulator_get_voltage(struct regulator *regulator)
{
	if (!regulator)
		return -EINVAL;

	return regulator_get_voltage_rdev(regulator->rdev);
}
EXPORT_SYMBOL_GPL(regulator_get_voltage);

static int regulator_name_indent(unsigned flags)
{
	return 30 + (flags & REGULATOR_PRINT_DEVS ? 50 : 0);
}

static void regulator_print_one(struct regulator_dev *rdev, int level, unsigned flags)
{
	const char *name = rdev->name;
	struct regulator *r;
	char buf[256];

	if (!rdev)
		return;

	if (flags & REGULATOR_PRINT_DEVS) {
		snprintf(buf, sizeof(buf), "%s  %s", dev_name(rdev->dev), rdev->name);
		name = buf;
	}

	printf("%*s%-*s %6d %10d %10d\n", level * 3, "",
	       regulator_name_indent(flags) - level * 3,
	       name, rdev->enable_count, rdev->min_uv, rdev->max_uv);

	list_for_each_entry(r, &rdev->consumer_list, list) {
		if (r->rdev_consumer)
			regulator_print_one(r->rdev_consumer, level + 1, flags);
		else
			printf("%*s%s\n", (level + 1) * 3, "", r->dev ? dev_name(r->dev) : "none");
	}
}

/*
 * regulators_print - print informations about all regulators
 */
void regulators_print(unsigned flags)
{
	struct regulator_dev *rdev;

	printf("%-*s %6s %10s %10s\n", regulator_name_indent(flags),
	       "name", "enable", "min_uv", "max_uv");
	list_for_each_entry(rdev, &regulator_list, list) {
		if (!rdev->supply)
			regulator_print_one(rdev, 0, flags);
	}
}
