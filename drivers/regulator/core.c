/*
 * barebox regulator support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 *
 */
#include <common.h>
#include <regulator.h>
#include <of.h>
#include <malloc.h>
#include <linux/err.h>

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
	struct list_head consumer_list;
};

struct regulator {
	struct regulator_internal *ri;
	struct list_head list;
	struct device_d *dev;
};

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

	ri = xzalloc(sizeof(*ri));
	ri->rdev = rd;
	ri->node = node;

	INIT_LIST_HEAD(&ri->consumer_list);

	list_add_tail(&ri->list, &regulator_list);

	name = of_get_property(node, "regulator-name", NULL);

	if (name)
		ri->name = xstrdup(name);

	of_property_read_u32(node, "regulator-enable-ramp-delay",
			&ri->enable_time_us);
	of_property_read_u32(node, "regulator-min-microvolt",
			&ri->min_uv);
	of_property_read_u32(node, "regulator-max-microvolt",
			&ri->max_uv);

	return 0;
}

static struct regulator_internal *of_regulator_get(struct device_d *dev, const char *supply)
{
	char *propname;
	struct regulator_internal *ri;
	struct device_node *node;

	propname = asprintf("%s-supply", supply);

	/*
	 * If the device does have a device node return the dummy regulator.
	 */
	if (!dev->device_node)
		return NULL;

	/*
	 * If the device node does not contain a supply property, this device doesn't
	 * need a regulator. Return the dummy regulator in this case.
	 */
	if (!of_get_property(dev->device_node, propname, NULL)) {
		dev_dbg(dev, "No %s-supply node found, using dummy regulator\n",
				supply);
		ri = NULL;
		goto out;
	}

	/*
	 * The device node specifies a supply, so it's mandatory. Return an error when
	 * something goes wrong below.
	 */
	node = of_parse_phandle(dev->device_node, propname, 0);
	if (!node) {
		dev_dbg(dev, "No %s node found\n", propname);
		ri = ERR_PTR(-EINVAL);
		goto out;
	}

	list_for_each_entry(ri, &regulator_list, list) {
		if (ri->node == node) {
			dev_dbg(dev, "Using %s regulator from %s\n",
					propname, node->full_name);
			goto out;
		}
	}

	ri = ERR_PTR(-ENODEV);
out:
	free(propname);

	return ri;
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
struct regulator *regulator_get(struct device_d *dev, const char *supply)
{
	struct regulator_internal *ri;
	struct regulator *r;

	if (!dev->device_node)
		return NULL;

	ri = of_regulator_get(dev, supply);
	if (IS_ERR(ri))
		return ERR_CAST(ri);

	if (!ri)
		return NULL;

	r = xzalloc(sizeof(*r));
	r->ri = ri;
	r->dev = dev;

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
	struct regulator_internal *ri;
	int ret;

	if (!r)
		return 0;

	ri = r->ri;

	if (ri->enable_count) {
		ri->enable_count++;
		return 0;
	}

	if (!ri->rdev->ops->enable)
		return -ENOSYS;

	ret = ri->rdev->ops->enable(ri->rdev);
	if (ret)
		return ret;

	if (ri->enable_time_us)
		udelay(ri->enable_time_us);

	ri->enable_count++;

	return 0;
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
	struct regulator_internal *ri;
	int ret;

	if (!r)
		return 0;

	ri = r->ri;

	if (!ri->enable_count)
		return -EINVAL;

	if (!ri->rdev->ops->disable)
		return -ENOSYS;

	ret = ri->rdev->ops->disable(ri->rdev);
	if (ret)
		return ret;

	ri->enable_count--;

	return 0;
}

static void regulator_print_one(struct regulator_internal *ri)
{
	struct regulator *r;

	printf("%-20s %6d %10d %10d\n", ri->name, ri->enable_count, ri->min_uv, ri->max_uv);

	if (!list_empty(&ri->consumer_list)) {
		printf(" consumers:\n");

		list_for_each_entry(r, &ri->consumer_list, list)
			printf("   %s\n", dev_name(r->dev));
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
