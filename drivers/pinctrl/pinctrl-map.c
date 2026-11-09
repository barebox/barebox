// SPDX-License-Identifier: GPL-2.0-only
/*
 * Apply Linux-style pinctrl maps parsed from the device tree
 *
 * Unlike Linux, maps are not kept in a pinctrl handle, but parsed, applied
 * and freed on every state selection.
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <of.h>
#include <pinctrl.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf-generic.h>

#include "core.h"
#include "pinconf.h"
#include "pinctrl-utils.h"
#include "pinmux.h"

static int pinconf_group_set(struct pinctrl_dev *pctldev, unsigned int group,
			     unsigned long *configs, unsigned int num_configs)
{
	const struct pinconf_ops *ops = pctldev->confops;
	const unsigned int *pins;
	unsigned int num_pins;
	int ret, i;

	if (!ops) {
		dev_err(pctldev->dev, "missing confops\n");
		return -EINVAL;
	}

	if (ops->pin_config_group_set)
		return ops->pin_config_group_set(pctldev, group, configs,
						 num_configs);

	/* barebox extension: configure the group's pins one by one */
	if (!pctldev->ops->get_group_pins)
		return -ENOTSUPP;

	ret = pctldev->ops->get_group_pins(pctldev, group, &pins, &num_pins);
	if (ret)
		return ret;

	for (i = 0; i < num_pins; i++) {
		ret = pinconf_set_config(pctldev, pins[i], configs, num_configs);
		if (ret)
			return ret;
	}

	return 0;
}

static int pinconf_map_apply(struct pinctrl_dev *pctldev,
			     const struct pinctrl_map *map)
{
	const struct pinctrl_map_configs *configs = &map->data.configs;
	int pin, group;

	switch (map->type) {
	case PIN_MAP_TYPE_CONFIGS_PIN:
		pin = pin_get_from_name(pctldev, configs->group_or_pin);
		if (pin < 0) {
			dev_err(pctldev->dev, "could not map pin config for \"%s\"\n",
				configs->group_or_pin);
			return pin;
		}

		return pinconf_set_config(pctldev, pin, configs->configs,
					  configs->num_configs);
	case PIN_MAP_TYPE_CONFIGS_GROUP:
		group = pinctrl_get_group_selector(pctldev,
						   configs->group_or_pin);
		if (group < 0)
			return group;

		return pinconf_group_set(pctldev, group, configs->configs,
					 configs->num_configs);
	default:
		return -EINVAL;
	}
}

static int pinmux_map_apply(struct pinctrl_dev *pctldev,
			    const struct pinctrl_map *map)
{
	const struct pinctrl_ops *pctlops = pctldev->ops;
	const struct pinmux_ops *ops = pctldev->pmxops;
	const unsigned int *pins = NULL;
	unsigned int num_pins = 0;
	int func, group, ret, i;

	if (!ops || !ops->set_mux) {
		dev_err(pctldev->dev, "does not support pinmux\n");
		return -EINVAL;
	}

	func = pinmux_func_name_to_selector(pctldev, map->data.mux.function);
	if (func < 0) {
		dev_err(pctldev->dev, "invalid function %s in map table\n",
			map->data.mux.function);
		return func;
	}

	/*
	 * Unlike Linux, don't fall back to the function's first group,
	 * maps parsed from the device tree always name one.
	 */
	if (!map->data.mux.group) {
		dev_err(pctldev->dev, "no group given for function %s\n",
			map->data.mux.function);
		return -EINVAL;
	}

	group = pinctrl_get_group_selector(pctldev, map->data.mux.group);
	if (group < 0)
		return group;

	/* Like pin_request() in Linux, let the driver veto the pins */
	if (ops->request && pctlops->get_group_pins) {
		ret = pctlops->get_group_pins(pctldev, group, &pins, &num_pins);
		if (ret) {
			/* Linux only warns too, the mux doesn't need the pins */
			dev_warn(pctldev->dev, "could not get pins for group %s\n",
				 map->data.mux.group);
			num_pins = 0;
		}

		for (i = 0; i < num_pins; i++) {
			ret = ops->request(pctldev, pins[i]);
			if (ret) {
				dev_err(pctldev->dev, "could not request pin %d from group %s\n",
					pins[i], map->data.mux.group);
				return ret;
			}
		}
	}

	return ops->set_mux(pctldev, func, group);
}

static int pinctrl_map_apply(struct pinctrl_dev *pctldev,
			     const struct pinctrl_map *map,
			     unsigned int num_maps)
{
	int ret, i;

	for (i = 0; i < num_maps; i++, map++) {
		switch (map->type) {
		case PIN_MAP_TYPE_DUMMY_STATE:
			ret = 0;
			break;
		case PIN_MAP_TYPE_MUX_GROUP:
			ret = pinmux_map_apply(pctldev, map);
			break;
		case PIN_MAP_TYPE_CONFIGS_PIN:
		case PIN_MAP_TYPE_CONFIGS_GROUP:
			ret = pinconf_map_apply(pctldev, map);
			break;
		default:
			ret = -EINVAL;
			break;
		}

		if (ret)
			return ret;
	}

	return 0;
}

static void pinctrl_map_free(struct pinctrl_dev *pctldev,
			     struct pinctrl_map *map, unsigned int num_maps)
{
	if (pctldev->ops->dt_free_map)
		pctldev->ops->dt_free_map(pctldev, map, num_maps);
	else
		pinctrl_utils_free_map(pctldev, map, num_maps);
}

/*
 * barebox extension: mux each pinmux cell with the driver's U-Boot style
 * pinmux_property_set and apply the node's configs to the returned pin.
 */
static int pinctrl_pinmux_apply(struct pinctrl_dev *pctldev,
				struct device_node *np)
{
	const struct pinmux_ops *ops = pctldev->pmxops;
	unsigned long *configs;
	unsigned int num_configs;
	int count, ret, i;

	if (!ops || !ops->pinmux_property_set) {
		dev_err(pctldev->dev, "%pOF: pinmux property not supported\n", np);
		return -ENOTSUPP;
	}

	count = of_property_count_elems_of_size(np, "pinmux", sizeof(u32));
	if (count < 0)
		return count;

	ret = pinconf_generic_parse_dt_config(np, pctldev, &configs,
					      &num_configs);
	if (ret)
		return ret;

	for (i = 0; i < count; i++) {
		u32 pinmux_group;
		int pin;

		ret = of_property_read_u32_index(np, "pinmux", i, &pinmux_group);
		if (ret)
			goto out;

		pin = ops->pinmux_property_set(pctldev, pinmux_group);
		if (pin < 0) {
			ret = pin;
			goto out;
		}

		if (num_configs) {
			ret = pinconf_set_config(pctldev, pin, configs,
						 num_configs);
			if (ret)
				goto out;
		}
	}

out:
	kfree(configs);

	return ret;
}

static int pinctrl_map_set_state_one(struct pinctrl_dev *pctldev,
				     struct device_node *np)
{
	struct pinctrl_map *map = NULL;
	unsigned int reserved_maps = 0, num_maps = 0;
	int ret;

	if (of_property_present(np, "pinmux"))
		return pinctrl_pinmux_apply(pctldev, np);

	ret = pinconf_generic_dt_subnode_to_map(pctldev, np, &map,
						&reserved_maps, &num_maps,
						PIN_MAP_TYPE_INVALID);
	if (!ret)
		ret = pinctrl_map_apply(pctldev, map, num_maps);

	pinctrl_utils_free_map(pctldev, map, num_maps);

	return ret;
}

/**
 * pinctrl_map_set_state() - apply a device tree pin configuration node
 * @pctldev: pin controller the node belongs to
 * @np: pin configuration node referenced by a pinctrl-N property
 *
 * Used for drivers without a set_state callback. The node is parsed with the
 * driver's dt_node_to_map if any, else with the generic pinconf parser.
 */
int pinctrl_map_set_state(struct pinctrl_dev *pctldev, struct device_node *np)
{
	const struct pinctrl_ops *ops = pctldev->ops;
	struct device_node *child;
	struct pinctrl_map *map;
	unsigned int num_maps;
	int ret;

	if (ops->dt_node_to_map) {
		ret = ops->dt_node_to_map(pctldev, np, &map, &num_maps);
		if (ret)
			return ret;

		ret = pinctrl_map_apply(pctldev, map, num_maps);
		pinctrl_map_free(pctldev, map, num_maps);

		return ret;
	}

	ret = pinctrl_map_set_state_one(pctldev, np);
	if (ret)
		return ret;

	for_each_available_child_of_node(np, child) {
		ret = pinctrl_map_set_state_one(pctldev, child);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(pinctrl_map_set_state);
