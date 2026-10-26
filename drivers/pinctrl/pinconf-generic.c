// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/pinctrl/pinconf-generic.c?id=352f5621b8c472bf328f452446bce85c00837223
/*
 * Core driver for the generic pin config portions of the pin control subsystem
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */

#define pr_fmt(fmt) "generic pinconfig core: " fmt

#include <linux/array_size.h>
#include <linux/bitmap.h>
#include <linux/device.h>
#include <init.h>
#include <linux/module.h>
#include <of.h>
#include <linux/slab.h>

#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>

#include "core.h"
#include "pinconf.h"
#include "pinctrl-utils.h"

#ifdef CONFIG_OFTREE /* barebox: CONFIG_OF needs OFDEVICE */
static const struct pinconf_generic_params dt_params[] = {
	{ "bias-bus-hold", PIN_CONFIG_BIAS_BUS_HOLD, 0 },
	{ "bias-disable", PIN_CONFIG_BIAS_DISABLE, 0 },
	{ "bias-high-impedance", PIN_CONFIG_BIAS_HIGH_IMPEDANCE, 0 },
	{ "bias-pull-up", PIN_CONFIG_BIAS_PULL_UP, 1 },
	{ "bias-pull-pin-default", PIN_CONFIG_BIAS_PULL_PIN_DEFAULT, 1 },
	{ "bias-pull-down", PIN_CONFIG_BIAS_PULL_DOWN, 1 },
	{ "drive-open-drain", PIN_CONFIG_DRIVE_OPEN_DRAIN, 0 },
	{ "drive-open-source", PIN_CONFIG_DRIVE_OPEN_SOURCE, 0 },
	{ "drive-push-pull", PIN_CONFIG_DRIVE_PUSH_PULL, 0 },
	{ "drive-strength", PIN_CONFIG_DRIVE_STRENGTH, 0 },
	{ "drive-strength-microamp", PIN_CONFIG_DRIVE_STRENGTH_UA, 0 },
	{ "input-debounce", PIN_CONFIG_INPUT_DEBOUNCE, 0 },
	{ "input-disable", PIN_CONFIG_INPUT_ENABLE, 0 },
	{ "input-enable", PIN_CONFIG_INPUT_ENABLE, 1 },
	{ "input-schmitt", PIN_CONFIG_INPUT_SCHMITT, 0 },
	{ "input-schmitt-disable", PIN_CONFIG_INPUT_SCHMITT_ENABLE, 0 },
	{ "input-schmitt-enable", PIN_CONFIG_INPUT_SCHMITT_ENABLE, 1 },
	{ "input-schmitt-microvolts", PIN_CONFIG_INPUT_SCHMITT_UV, 0 },
	{ "low-power-disable", PIN_CONFIG_MODE_LOW_POWER, 0 },
	{ "low-power-enable", PIN_CONFIG_MODE_LOW_POWER, 1 },
	{ "output-disable", PIN_CONFIG_OUTPUT_ENABLE, 0 },
	{ "output-enable", PIN_CONFIG_OUTPUT_ENABLE, 1 },
	{ "output-high", PIN_CONFIG_LEVEL, 1, },
	{ "output-impedance-ohms", PIN_CONFIG_OUTPUT_IMPEDANCE_OHMS, 0 },
	{ "output-low", PIN_CONFIG_LEVEL, 0, },
	{ "power-source", PIN_CONFIG_POWER_SOURCE, 0 },
	{ "sleep-hardware-state", PIN_CONFIG_SLEEP_HARDWARE_STATE, 0 },
	{ "slew-rate", PIN_CONFIG_SLEW_RATE, 0 },
	{ "skew-delay", PIN_CONFIG_SKEW_DELAY, 0 },
	{ "skew-delay-input-ps", PIN_CONFIG_SKEW_DELAY_INPUT_PS, 0 },
	{ "skew-delay-output-ps", PIN_CONFIG_SKEW_DELAY_OUTPUT_PS, 0 },
	{ "input-threshold-voltage-microvolt", PIN_CONFIG_INPUT_VOLTAGE_UV, 0 },
};

/**
 * parse_fw_cfg() - Parse firmware pinconf parameters
 * @np:	device node
 * @params:	Array of describing generic parameters
 * @count:	Number of entries in @params
 * @cfg:	Array of parsed config options
 * @ncfg:	Number of entries in @cfg
 *
 * Parse the config options described in @params from @np and puts the result
 * in @cfg. @cfg does not need to be empty, entries are added beginning at
 * @ncfg. @ncfg is updated to reflect the number of entries after parsing. @cfg
 * needs to have enough memory allocated to hold all possible entries.
 */
static int parse_fw_cfg(struct device_node *np,
			const struct pinconf_generic_params *params,
			unsigned int count, unsigned long *cfg,
			unsigned int *ncfg)
{
	int i, test;

	unsigned long *properties __free(bitmap) = bitmap_zalloc(count, GFP_KERNEL);

	for (i = 0; i < count; i++) {
		u32 val;
		int ret;
		const struct pinconf_generic_params *par = &params[i];

		if (par->values && par->num_values) {
			ret = of_property_match_property_string(np,
								    par->property,
								    par->values, par->num_values);
			if (ret == -ENOENT)
				return ret;
			if (ret >= 0) {
				val = ret;
				ret = 0;
			}
		} else {
			ret = of_property_read_u32(np, par->property, &val);
		}

		/* property not found */
		if (ret == -EINVAL)
			continue;

		/* use default value, when no value is specified */
		if (ret)
			val = par->default_value;

		/* if param is greater than count, these are custom properties */
		if (par->param <= count) {
			ret = test_and_set_bit(par->param, properties);
			if (ret) {
				pr_err("%pOF: conflicting setting detected for %s\n",
				       np, par->property);
				return -EINVAL;
			}
		}

		pr_debug("found %s with value %u\n", par->property, val);
		cfg[*ncfg] = pinconf_to_config_packed(par->param, val);
		(*ncfg)++;
	}

	if (test_bit(PIN_CONFIG_DRIVE_STRENGTH, properties) &&
			test_bit(PIN_CONFIG_DRIVE_STRENGTH_UA, properties))
		pr_err("%pOF: cannot have multiple drive strength properties\n",
		       np);

	test = test_bit(PIN_CONFIG_BIAS_BUS_HOLD, properties) +
		test_bit(PIN_CONFIG_BIAS_DISABLE, properties) +
		test_bit(PIN_CONFIG_BIAS_HIGH_IMPEDANCE, properties) +
		test_bit(PIN_CONFIG_BIAS_PULL_UP, properties) +
		test_bit(PIN_CONFIG_BIAS_PULL_PIN_DEFAULT, properties) +
		test_bit(PIN_CONFIG_BIAS_PULL_DOWN, properties);
	if (test > 1)
		pr_err("%pOF: cannot have multiple bias configurations\n",
		       np);

	test = test_bit(PIN_CONFIG_DRIVE_OPEN_DRAIN, properties) +
		test_bit(PIN_CONFIG_DRIVE_OPEN_SOURCE, properties) +
		test_bit(PIN_CONFIG_DRIVE_PUSH_PULL, properties);
	if (test > 1)
		pr_err("%pOF: cannot have multiple drive configurations\n",
		       np);

	return 0;
}

/**
 * pinconf_generic_parse_dt_pinmux()
 * parse the pinmux properties into generic pin mux values.
 * @np: node containing the pinmux properties
 * @dev: pincontrol core device
 * @pid: array with pin identity entries
 * @pmux: array with pin mux value entries
 * @npins: number of pins
 *
 * pinmux property: mux value [0,7]bits and pin identity [8,31]bits.
 */
int pinconf_generic_parse_dt_pinmux(struct device_node *np, struct device *dev,
				    unsigned int **pid, unsigned int **pmux,
				    unsigned int *npins)
{
	unsigned int *pid_t;
	unsigned int *pmux_t;
	unsigned int npins_t, i;
	int ret;

	ret = of_property_count_elems_of_size(np, "pinmux", sizeof(u32));
	if (ret < 0) {
		dev_info(dev, "Missing pinmux property\n");
		return ret;
	}

	npins_t = ret;
	if (npins_t == 0) {
		dev_info(dev, "pinmux property doesn't have entries\n");
		return -ENODATA;
	}

	if (!pid || !pmux || !npins) {
		dev_err(dev, "parameters error\n");
		return -EINVAL;
	}

	pid_t = kcalloc(npins_t, sizeof(*pid_t), GFP_KERNEL);
	pmux_t = kcalloc(npins_t, sizeof(*pmux_t), GFP_KERNEL);
	if (!pid_t || !pmux_t) {
		ret = -ENOMEM;
		goto exit;
	}

	ret = of_property_read_u32_array(np, "pinmux", pmux_t, npins_t);
	if (ret) {
		dev_err(dev, "get pinmux value fail\n");
		goto exit;
	}

	for (i = 0; i < npins_t; i++) {
		pid_t[i] = pmux_t[i] >> 8;
		pmux_t[i] = pmux_t[i] & 0xff;
	}
	*pid = pid_t;
	*pmux = pmux_t;
	*npins = npins_t;

	return 0;
exit:
	kfree(pid_t);
	kfree(pmux_t);
	return ret;
}
EXPORT_SYMBOL_GPL(pinconf_generic_parse_dt_pinmux);

/**
 * pinconf_generic_parse_dt_config()
 * parse the config properties into generic pinconfig values.
 * @np: node containing the pinconfig properties
 * @pctldev: pincontrol device
 * @configs: array with nconfigs entries containing the generic pinconf values
 *           must be freed when no longer necessary.
 * @nconfigs: number of configurations
 */
int pinconf_generic_parse_dt_config(struct device_node *np,
				    struct pinctrl_dev *pctldev,
				    unsigned long **configs,
				    unsigned int *nconfigs)
{
	unsigned long *cfg;
	unsigned int max_cfg, ncfg = 0;
	int ret;

	if (!np)
		return -EINVAL;

	/* allocate a temporary array big enough to hold one of each option */
	max_cfg = ARRAY_SIZE(dt_params);
	/* the descriptor is optional in barebox */
	if (pctldev && pctldev->desc)
		max_cfg += pctldev->desc->num_custom_params;
	cfg = kcalloc(max_cfg, sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;

	ret = parse_fw_cfg(np, dt_params, ARRAY_SIZE(dt_params), cfg, &ncfg);
	if (ret)
		goto out;
	if (pctldev && pctldev->desc && pctldev->desc->num_custom_params &&
		pctldev->desc->custom_params) {
		ret = parse_fw_cfg(np, pctldev->desc->custom_params,
				   pctldev->desc->num_custom_params, cfg, &ncfg);
		if (ret)
			goto out;
	}

	/* no configs found at all */
	if (ncfg == 0) {
		*configs = NULL;
		*nconfigs = 0;
		goto out;
	}

	/*
	 * Now limit the number of configs to the real number of
	 * found properties.
	 */
	*configs = memdup_array(cfg, ncfg);
	if (!*configs) {
		ret = -ENOMEM;
		goto out;
	}

	*nconfigs = ncfg;

out:
	kfree(cfg);
	return ret;
}
EXPORT_SYMBOL_GPL(pinconf_generic_parse_dt_config);

int pinconf_generic_dt_subnode_to_map(struct pinctrl_dev *pctldev,
		struct device_node *np, struct pinctrl_map **map,
		unsigned int *reserved_maps, unsigned int *num_maps,
		enum pinctrl_map_type type)
{
	int ret;
	const char *function;
	struct device *dev = pctldev->dev;
	unsigned long *configs = NULL;
	unsigned int num_configs = 0;
	unsigned int reserve, strings_count;
	struct property *prop;
	const char *group;
	const char *subnode_target_type = "pins";

	ret = of_property_count_strings(np, "pins");
	if (ret < 0) {
		ret = of_property_count_strings(np, "groups");
		if (ret < 0)
			/* skip this node; may contain config child nodes */
			return 0;
		if (type == PIN_MAP_TYPE_INVALID)
			type = PIN_MAP_TYPE_CONFIGS_GROUP;
		subnode_target_type = "groups";
	} else {
		if (type == PIN_MAP_TYPE_INVALID)
			type = PIN_MAP_TYPE_CONFIGS_PIN;
	}
	strings_count = ret;

	ret = of_property_read_string(np, "function", &function);
	if (ret < 0) {
		/* EINVAL=missing, which is fine since it's optional */
		if (ret != -EINVAL)
			dev_err(dev, "%pOF: could not parse property function\n",
				np);
		function = NULL;
	}

	ret = pinconf_generic_parse_dt_config(np, pctldev, &configs,
					      &num_configs);
	if (ret < 0) {
		dev_err(dev, "%pOF: could not parse node property\n", np);
		return ret;
	}

	reserve = 0;
	if (function != NULL)
		reserve++;
	if (num_configs)
		reserve++;

	reserve *= strings_count;

	ret = pinctrl_utils_reserve_map(pctldev, map, reserved_maps,
			num_maps, reserve);
	if (ret < 0)
		goto exit;

	of_property_for_each_string(np, subnode_target_type, prop, group) {
		if (function) {
			ret = pinctrl_utils_add_map_mux(pctldev, map,
					reserved_maps, num_maps, group,
					function);
			if (ret < 0)
				goto exit;
		}

		if (num_configs) {
			ret = pinctrl_utils_add_map_configs(pctldev, map,
					reserved_maps, num_maps, group, configs,
					num_configs, type);
			if (ret < 0)
				goto exit;
		}
	}
	ret = 0;

exit:
	kfree(configs);
	return ret;
}
EXPORT_SYMBOL_GPL(pinconf_generic_dt_subnode_to_map);

int pinconf_generic_dt_node_to_map(struct pinctrl_dev *pctldev,
		struct device_node *np_config, struct pinctrl_map **map,
		unsigned int *num_maps, enum pinctrl_map_type type)
{
	unsigned int reserved_maps;
	int ret;

	reserved_maps = 0;
	*map = NULL;
	*num_maps = 0;

	ret = pinconf_generic_dt_subnode_to_map(pctldev, np_config, map,
						&reserved_maps, num_maps, type);
	if (ret < 0)
		goto exit;

	for_each_available_child_of_node_scoped(np_config, np) {
		ret = pinconf_generic_dt_subnode_to_map(pctldev, np, map,
					&reserved_maps, num_maps, type);
		if (ret < 0)
			goto exit;
	}
	return 0;

exit:
	pinctrl_utils_free_map(pctldev, *map, *num_maps);
	return ret;
}
EXPORT_SYMBOL_GPL(pinconf_generic_dt_node_to_map);

void pinconf_generic_dt_free_map(struct pinctrl_dev *pctldev,
				 struct pinctrl_map *map,
				 unsigned int num_maps)
{
	pinctrl_utils_free_map(pctldev, map, num_maps);
}
EXPORT_SYMBOL_GPL(pinconf_generic_dt_free_map);

#endif
