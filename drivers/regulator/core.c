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

struct regulator_internal {
	struct list_head list;
	struct device_node *node;
	struct regulator_dev *rdev;
	int enable_count;
	int enable_time_us;
	int min_uv;
	int max_uv;
	char *name;
	const char *supply;
	struct list_head consumer_list;
};

struct regulator {
	struct regulator_internal *ri;
	struct list_head list;
	struct device *dev;
};

static int regulator_map_voltage(struct regulator_dev *rdev, int min_uV,
				 int max_uV)
{
	if (rdev->desc->ops->list_voltage == regulator_list_voltage_linear)
		return regulator_map_voltage_linear(rdev, min_uV, max_uV);

	return -ENOSYS;
}

static int regulator_enable_internal(struct regulator_internal *ri)
{
	struct regulator_dev *rdev = ri->rdev;
	int ret;

	if (ri->enable_count) {
		ri->enable_count++;
		return 0;
	}

	if (!rdev->desc->ops->enable)
		return -ENOSYS;

	/* turn on parent regulator */
	ret = regulator_enable(rdev->supply);
	if (ret)
		return ret;

	ret = rdev->desc->ops->enable(ri->rdev);
	if (ret) {
		regulator_disable(rdev->supply);
		return ret;
	}

	if (ri->enable_time_us)
		udelay(ri->enable_time_us);

	ri->enable_count++;

	return 0;
}

static int regulator_disable_internal(struct regulator_internal *ri)
{
	struct regulator_dev *rdev = ri->rdev;
	int ret;

	if (!ri->enable_count)
		return -EINVAL;

	if (ri->enable_count > 1) {
		ri->enable_count--;
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

	ri->enable_count--;

	return regulator_disable(ri->rdev->supply);
}

static int regulator_set_voltage_internal(struct regulator_internal *ri,
					  int min_uV, int max_uV)
{
	struct regulator_dev *rdev = ri->rdev;
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

static int regulator_resolve_supply(struct regulator_dev *rdev)
{
	struct regulator *supply;
	const char *supply_name;

	if (!rdev || rdev->supply)
		return 0;

	supply_name = rdev->desc->supply_name;
	if (!supply_name)
		return 0;

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
		dev_warn(rdev->dev, "Failed to get '%s' regulator (ignored).\n",
			 supply_name);
		return 0;
	}

	rdev->supply = supply;
	return 0;
}

static struct regulator_internal * __regulator_register(struct regulator_dev *rd, const char *name)
{
	struct regulator_internal *ri;
	int ret;

	ri = xzalloc(sizeof(*ri));
	ri->rdev = rd;

	INIT_LIST_HEAD(&ri->consumer_list);

	list_add_tail(&ri->list, &regulator_list);

	if (name)
		ri->name = xstrdup(name);

	if (rd->boot_on || rd->always_on) {
		ret = regulator_resolve_supply(ri->rdev);
		if (ret < 0)
			goto err;

		ret = regulator_enable_internal(ri);
		if (ret && ret != -ENOSYS)
			goto err;
	}

	return ri;
err:
	list_del(&ri->list);
	free(ri->name);
	free(ri);

	return ERR_PTR(ret);
}


#ifdef CONFIG_OFDEVICE
/*
 * of_regulator_register - register a regulator corresponding to a device_node
 * @rd:		the regulator device providing the ops
 * @node:       the device_node this regulator corresponds to
 *
 * Return: 0 for success or a negative error code
 */
int of_regulator_register(struct regulator_dev *rd, struct device_node *node)
{
	struct regulator_internal *ri;
	const char *name;

	if (!rd || !node)
		return -EINVAL;

	rd->boot_on = of_property_read_bool(node, "regulator-boot-on");
	rd->always_on = of_property_read_bool(node, "regulator-always-on");

	name = of_get_property(node, "regulator-name", NULL);
	if (!name)
		name = node->name;

	ri = __regulator_register(rd, name);
	if (IS_ERR(ri))
		return PTR_ERR(ri);

	ri->node = node;
	node->dev = rd->dev;

	if (rd->desc->off_on_delay)
		ri->enable_time_us = rd->desc->off_on_delay;

	if (rd->desc->fixed_uV && rd->desc->n_voltages == 1)
		ri->min_uv = ri->max_uv = rd->desc->fixed_uV;

	of_property_read_u32(node, "regulator-enable-ramp-delay",
			&ri->enable_time_us);
	of_property_read_u32(node, "regulator-min-microvolt",
			&ri->min_uv);
	of_property_read_u32(node, "regulator-max-microvolt",
			&ri->max_uv);

	return 0;
}

static struct regulator_internal *of_regulator_get(struct device *dev,
						   const char *supply)
{
	char *propname;
	struct regulator_internal *ri;
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
		ri = NULL;
		goto out;
	}

	/*
	 * The device node specifies a supply, so it's mandatory. Return an error when
	 * something goes wrong below.
	 */
	node = of_parse_phandle(dev->of_node, propname, 0);
	if (!node) {
		dev_dbg(dev, "No %s node found\n", propname);
		ri = ERR_PTR(-EINVAL);
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
			ri = NULL;
			goto out;
		}

		ri = ERR_PTR(ret);
		goto out;
	}

	list_for_each_entry(ri, &regulator_list, list) {
		if (ri->node == node) {
			dev_dbg(dev, "Using %s regulator from %s\n",
					propname, node->full_name);
			goto out;
		}
	}

	/*
	 * It is possible that regulator we are looking for will be
	 * added in future initcalls, so, instead of reporting a
	 * complete failure report probe deferral
	 */
	ri = ERR_PTR(-EPROBE_DEFER);
out:
	free(propname);

	return ri;
}
#else
static struct regulator_internal *of_regulator_get(struct device *dev,
						   const char *supply)
{
	return NULL;
}
#endif

int dev_regulator_register(struct regulator_dev *rd, const char * name, const char* supply)
{
	struct regulator_internal *ri;

	ri = __regulator_register(rd, name);

	ri->supply = supply;

	return 0;
}

static struct regulator_internal *dev_regulator_get(struct device *dev,
						    const char *supply)
{
	struct regulator_internal *ri;
	struct regulator_internal *ret = NULL;
	int match, best = 0;
	const char *dev_id = dev ? dev_name(dev) : NULL;

	list_for_each_entry(ri, &regulator_list, list) {
		match = 0;
		if (ri->name) {
			if (!dev_id || strcmp(ri->name, dev_id))
				continue;
			match += 2;
		}
		if (ri->supply) {
			if (!supply || strcmp(ri->supply, supply))
				continue;
			match += 1;
		}

		if (match > best) {
			ret = ri;
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
	struct regulator_internal *ri = NULL;
	struct regulator *r;
	int ret;

	if (dev->of_node && supply) {
		ri = of_regulator_get(dev, supply);
		if (IS_ERR(ri))
			return ERR_CAST(ri);
	}

	if (!ri) {
		ri = dev_regulator_get(dev, supply);
		if (IS_ERR(ri))
			return ERR_CAST(ri);
	}

	if (!ri)
		return NULL;

	ret = regulator_resolve_supply(ri->rdev);
	if (ret < 0)
		return ERR_PTR(ret);

	r = xzalloc(sizeof(*r));
	r->ri = ri;
	r->dev = dev;

	list_add_tail(&r->list, &ri->consumer_list);

	return r;
}

static struct regulator_internal *regulator_by_name(const char *name)
{
	struct regulator_internal *ri;

	list_for_each_entry(ri, &regulator_list, list)
		if (ri->name && !strcmp(ri->name, name))
			return ri;

	return NULL;
}

struct regulator *regulator_get_name(const char *name)
{
	struct regulator_internal *ri;
	struct regulator *r;

	ri = regulator_by_name(name);
	if (!ri)
		return ERR_PTR(-ENODEV);

	r = xzalloc(sizeof(*r));
	r->ri = ri;

	list_add_tail(&r->list, &ri->consumer_list);

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

	return regulator_enable_internal(r->ri);
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

	return regulator_disable_internal(r->ri);
}

int regulator_set_voltage(struct regulator *r, int min_uV, int max_uV)
{
	if (!r)
		return 0;

	return regulator_set_voltage_internal(r->ri, min_uV, max_uV);
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
	struct regulator_internal *ri;
	struct regulator_dev *rdev;
	int sel, ret;

	if (!regulator)
		return -EINVAL;

	ri = regulator->ri;
	rdev = ri->rdev;

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
	} else if (ri->min_uv && ri->min_uv == ri->max_uv) {
		ret = ri->min_uv;
	} else if (rdev->supply) {
		ret = regulator_get_voltage(rdev->supply);
	} else {
		return -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(regulator_get_voltage);

static void regulator_print_one(struct regulator_internal *ri)
{
	struct regulator *r;

	printf("%-20s %6d %10d %10d\n", ri->name, ri->enable_count, ri->min_uv, ri->max_uv);

	if (!list_empty(&ri->consumer_list)) {
		printf(" consumers:\n");

		list_for_each_entry(r, &ri->consumer_list, list)
			printf("   %s\n", r->dev ? dev_name(r->dev) : "none");
	}
}

/*
 * regulators_print - print informations about all regulators
 */
void regulators_print(void)
{
	struct regulator_internal *ri;

	printf("%-20s %6s %10s %10s\n", "name", "enable", "min_uv", "max_uv");
	list_for_each_entry(ri, &regulator_list, list)
		regulator_print_one(ri);
}
