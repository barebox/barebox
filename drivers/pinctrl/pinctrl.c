// SPDX-License-Identifier: GPL-2.0-only
/*
 * pinctrl.c - barebox pinctrl support
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 */
#include <common.h>
#include <malloc.h>
#include <pinctrl.h>
#include "core.h"
#include "pinmux.h"
#include <linux/overflow.h>
#include <errno.h>
#include <of.h>
#include <security/config.h>
#include <security/policy.h>

struct pinctrl {
	struct device_node consumer_np;
};

LIST_HEAD(pinctrl_devices_name_list);

struct pinctrl_devices_name_info {
	struct device_node *node;
	const char *name;
	struct list_head list;
};


static LIST_HEAD(pinctrl_consumer_list);

struct pinctrl_state {
	struct property prop;
};

struct pinctrl_consumer_info {
	struct device *dev;
	struct device_node *np;
	int state;
	struct list_head list;
	const char *states[];
};

LIST_HEAD(pinctrl_list);
EXPORT_SYMBOL(pinctrl_list);

static struct pinctrl_dev *pin_to_pinctrl(unsigned int pin)
{
	struct pinctrl_dev *pinctrl;

	list_for_each_entry(pinctrl, &pinctrl_list, list)
		if (pin >= pinctrl->base &&
		    pin < pinctrl->base + pinctrl->npins)
			return pinctrl;
	return NULL;
}

static int pinctrl_gpio_direction_pin(unsigned pin, bool input)
{
	struct pinctrl_dev *pinctrl = pin_to_pinctrl(pin);

	if (!pinctrl)
		return -EINVAL;

	if (!pinctrl->pmxops || !pinctrl->pmxops->gpio_set_direction)
		return -ENOTSUPP;

	return pinctrl->pmxops->gpio_set_direction(pinctrl, NULL, pin, input);
}

int pinctrl_gpio_direction_input_pin(unsigned int pin)
{
	return pinctrl_gpio_direction_pin(pin, true);
}

int pinctrl_gpio_direction_output_pin(unsigned int pin)
{
	return pinctrl_gpio_direction_pin(pin, false);
}

int pinctrl_gpio_get_direction_pin(unsigned pin)
{
	struct pinctrl_dev *pinctrl = pin_to_pinctrl(pin);

	if (!pinctrl)
		return -EINVAL;

	if (!pinctrl->pmxops || !pinctrl->pmxops->gpio_get_direction)
		return -ENOTSUPP;

	return pinctrl->pmxops->gpio_get_direction(pinctrl, NULL, pin);
}

struct pinctrl_dev *get_pinctrl_dev_from_of_node(struct device_node *np)
{
	struct pinctrl_dev *pdev;

	list_for_each_entry(pdev, &pinctrl_list, list)
		if (pdev->node == np)
			return pdev;
	return NULL;
}

struct pinctrl_dev *of_pinctrl_get(struct device_node *np)
{
	return get_pinctrl_dev_from_of_node(np);
}

struct pinctrl_dev *get_pinctrl_dev_from_devname(const char *devname)
{
	struct pinctrl_dev *pdev;

	if (!devname)
		return NULL;

	list_for_each_entry(pdev, &pinctrl_list, list)
		if (!strcmp(dev_name(pdev->dev), devname))
			return pdev;
	return NULL;
}

const char *pinctrl_dev_get_name(struct pinctrl_dev *pctldev)
{
	return pctldev->desc ? pctldev->desc->name : dev_name(pctldev->dev);
}
EXPORT_SYMBOL_GPL(pinctrl_dev_get_name);

const char *pinctrl_dev_get_devname(struct pinctrl_dev *pctldev)
{
	return dev_name(pctldev->dev);
}
EXPORT_SYMBOL_GPL(pinctrl_dev_get_devname);

void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return pctldev->driver_data;
}
EXPORT_SYMBOL_GPL(pinctrl_dev_get_drvdata);

static int pinctrl_config_one(struct device_node *for_node, struct device_node *np)
{
	struct pinctrl_dev *pdev;
	struct device_node *pinctrl_node = np;

	while (1) {
		pinctrl_node = pinctrl_node->parent;
		if (!pinctrl_node)
			return -ENODEV;
		if (of_get_property(pinctrl_node, "compatible", NULL))
			break;
	}

	if (pinctrl_node != for_node)
		of_device_ensure_probed(pinctrl_node);

	pdev = get_pinctrl_dev_from_of_node(pinctrl_node);
	if (!pdev)
		return -ENODEV;
	if (!pdev->ops->set_state)
		return -ENOSYS;

	return pdev->ops->set_state(pdev, np);
}

static inline struct pinctrl_state *
of_property_pinctrl_get_state(struct property *prop)
{
	return container_of(prop, struct pinctrl_state, prop);
}

static void pinctrl_update_state_param(struct pinctrl *pinctrl,
				       struct pinctrl_state *state)
{
	struct device_node *np = &pinctrl->consumer_np;
	struct property *prop = &state->prop;
	struct pinctrl_consumer_info *info;
	u16 idx;
	int ret;

	if (!IS_ENABLED(CONFIG_PINCTRL_STATE_PARAM))
		return;

	if (!strstarts(prop->name, "pinctrl-"))
		return;

	ret = kstrtou16(&prop->name[sizeof("pinctrl-") - 1], 10, &idx);
	if (ret)
		return;

	list_for_each_entry(info, &pinctrl_consumer_list, list) {
		if (info->np != np)
			continue;

		info->state = idx;
	}
}

struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *pinctrl,
					   const char *name)
{
	struct device_node *np = &pinctrl->consumer_np;
	int state, ret;
	char propname[sizeof("pinctrl-4294967295")];
	struct property *prop;
	const char *statename;

	/* For each defined state ID */
	for (state = 0; ; state++) {
		/* Retrieve the pinctrl-* property */
		sprintf(propname, "pinctrl-%d", state);
		prop = of_find_property(np, propname, NULL);
		if (!prop)
			return ERR_PTR(-ENODEV);

		/* Determine whether pinctrl-names property names the state */
		ret = of_property_read_string_index(np, "pinctrl-names",
						    state, &statename);
		/*
		 * If not, statename is just the integer state ID. But rather
		 * than dynamically allocate it and have to free it later,
		 * just point part way into the property name for the string.
		 */
		if (ret < 0) {
			/* strlen("pinctrl-") == 8 */
			statename = &propname[8];
		}

		if (strcmp(name, statename))
			continue;

		return of_property_pinctrl_get_state(prop);
	}

	return ERR_PTR(ret);
}

int pinctrl_select_state(struct pinctrl *pinctrl, struct pinctrl_state *state)
{
	int ret = 0;
	const __be32 *list;
	int size, config;
	phandle phandle;
	struct device_node *np = &pinctrl->consumer_np, *np_config;
	struct property *prop = &state->prop;

	list = of_property_get_value(prop);
	size = prop->length / sizeof(*list);

	/* For every referenced pin configuration node in it */
	for (config = 0; config < size; config++) {
		phandle = be32_to_cpup(list++);

		/* Look up the pin configuration node */
		np_config = of_find_node_by_phandle(phandle);
		if (!np_config) {
			pr_err("prop %pOF %s index %i invalid phandle\n",
				np, prop->name, config);
			ret = -EINVAL;
			goto err;
		}

		/* Parse the node */
		ret = pinctrl_config_one(np, np_config);
		if (ret < 0)
			goto err;
	}

	pinctrl_update_state_param(pinctrl, state);
err:
	return ret;
}

static void of_pinctrl_policy_update_or_add(struct device_node *np, const char *name)
{
	struct pinctrl_devices_name_info *dev;

	list_for_each_entry(dev, &pinctrl_devices_name_list, list) {
		if (dev->node == np) {
			free_const(dev->name);
			dev->name = xstrdup_const(name);
			return;
		}
	}
	dev = xzalloc(sizeof(*dev));
	dev->node = np;
	dev->name = xstrdup_const(name);
	list_add(&dev->list, &pinctrl_devices_name_list);
}

int of_pinctrl_select_state(struct device_node *np, const char *name)
{
	struct pinctrl *pinctrl = (struct pinctrl *)np;
	struct pinctrl_state *state = NULL;

	if (!of_find_property(np, "pinctrl-0", NULL))
		return 0;

	if (IS_ENABLED(CONFIG_SECURITY_POLICY_PINCTRL)) {
		const struct security_policy *active_policy = security_policy_get_active();

		of_pinctrl_policy_update_or_add(np, name);

		if (active_policy && active_policy->name && name) {
			char *policy_pinctrl;

			policy_pinctrl = basprintf("barebox,policy-%s-%s",
						   active_policy->name,
						   name);
			state = pinctrl_lookup_state(pinctrl, policy_pinctrl);
			free(policy_pinctrl);
		}
	}
	if (IS_ERR_OR_NULL(state))
		state = pinctrl_lookup_state(pinctrl, name);
	if (IS_ERR(state))
		return PTR_ERR(state);

	return pinctrl_select_state(pinctrl, state);
}

int of_pinctrl_select_state_default(struct device_node *np)
{
	return of_pinctrl_select_state(np, "default");
}

struct pinctrl *pinctrl_get_select(struct device *dev, const char *name)
{
	struct device_node *np;
	int ret;

	np = dev->of_node;
	if (!np)
		return ERR_PTR(-ENODEV);

	ret = of_pinctrl_select_state(np, name);
	if (ret)
		return ERR_PTR(ret);

	return (struct pinctrl *)np;
}

int pinctrl_select_state_default(struct device *dev)
{
	return PTR_ERR_OR_ZERO(pinctrl_get_select(dev, "default"));
}

static int pinctrl_init_controller(struct pinctrl_dev *pdev)
{
	if (!IS_ENABLED(CONFIG_PINCTRL))
		return -ENOSYS;

	BUG_ON(!pdev->dev->of_node);

	if (pdev->desc) {
		if (!pdev->ops)
			pdev->ops = pdev->desc->pctlops;
		if (!pdev->pmxops)
			pdev->pmxops = pdev->desc->pmxops;
		if (!pdev->confops)
			pdev->confops = pdev->desc->confops;
	}

	if (!pdev->ops)
		return -EINVAL;

	pdev->node = pdev->dev->of_node;

	list_add_tail(&pdev->list, &pinctrl_list);

	return 0;
}

/**
 * pinctrl_enable() - enable a pin controller
 * @pdev: the pin controller to enable
 *
 * Applies the pin controller's own default state (hogs).
 */
int pinctrl_enable(struct pinctrl_dev *pdev)
{
	return pinctrl_select_state_default(pdev->dev);
}
EXPORT_SYMBOL_GPL(pinctrl_enable);

/**
 * pinctrl_register() - register a driver-allocated pin controller
 * @pdev: the pin controller with dev set, and ops or desc
 *
 * barebox-specific, also enables the controller. Operations missing in
 * @pdev are taken from @pdev->desc, if set.
 */
int pinctrl_register(struct pinctrl_dev *pdev)
{
	int ret;

	ret = pinctrl_init_controller(pdev);
	if (ret)
		return ret;

	pinctrl_enable(pdev);

	return 0;
}

/**
 * pinctrl_register_and_init() - register and init pin controller
 * @pctldesc: descriptor for this pin controller
 * @dev: parent device for this pin controller
 * @driver_data: private pin controller data for this pin controller
 * @pctldev: pin controller device
 *
 * Note that pinctrl_enable() still needs to be manually called after
 * this once the driver is ready.
 */
int pinctrl_register_and_init(const struct pinctrl_desc *pctldesc,
			      struct device *dev, void *driver_data,
			      struct pinctrl_dev **pctldev)
{
	struct pinctrl_dev *pdev;
	int ret;

	pdev = xzalloc(sizeof(*pdev));
	pdev->desc = pctldesc;
	pdev->dev = dev;
	pdev->driver_data = driver_data;

	ret = pinctrl_init_controller(pdev);
	if (ret) {
		free(pdev);
		return ret;
	}

	*pctldev = pdev;

	return 0;
}
EXPORT_SYMBOL_GPL(pinctrl_register_and_init);

void pinctrl_unregister(struct pinctrl_dev *pdev)
{
	list_del(&pdev->list);
#ifdef CONFIG_GENERIC_PINCTRL_GROUPS
	free(pdev->groups);
#endif
#ifdef CONFIG_GENERIC_PINMUX_FUNCTIONS
	free(pdev->functions);
#endif
}
EXPORT_SYMBOL_GPL(pinctrl_unregister);

#ifdef CONFIG_GENERIC_PINCTRL_GROUPS

/**
 * pinctrl_generic_get_group_count() - returns the number of pin groups
 * @pctldev: pin controller device
 */
int pinctrl_generic_get_group_count(struct pinctrl_dev *pctldev)
{
	return pctldev->num_groups;
}
EXPORT_SYMBOL_GPL(pinctrl_generic_get_group_count);

/**
 * pinctrl_generic_get_group_name() - returns the name of a pin group
 * @pctldev: pin controller device
 * @selector: group number
 */
const char *pinctrl_generic_get_group_name(struct pinctrl_dev *pctldev,
					   unsigned int selector)
{
	struct group_desc *group;

	group = pinctrl_generic_get_group(pctldev, selector);
	if (!group)
		return NULL;

	return group->grp.name;
}
EXPORT_SYMBOL_GPL(pinctrl_generic_get_group_name);

/**
 * pinctrl_generic_get_group_pins() - gets the pin group pins
 * @pctldev: pin controller device
 * @selector: group number
 * @pins: pins in the group
 * @num_pins: number of pins in the group
 */
int pinctrl_generic_get_group_pins(struct pinctrl_dev *pctldev,
				   unsigned int selector,
				   const unsigned int **pins,
				   unsigned int *num_pins)
{
	struct group_desc *group;

	group = pinctrl_generic_get_group(pctldev, selector);
	if (!group) {
		dev_err(pctldev->dev, "%s could not find pingroup%i\n",
			__func__, selector);
		return -EINVAL;
	}

	*pins = group->grp.pins;
	*num_pins = group->grp.npins;

	return 0;
}
EXPORT_SYMBOL_GPL(pinctrl_generic_get_group_pins);

/**
 * pinctrl_generic_get_group() - returns a pin group based on the number
 * @pctldev: pin controller device
 * @selector: group number
 *
 * Unlike in Linux, the pointer is only valid until the next
 * pinctrl_generic_add_group().
 */
struct group_desc *pinctrl_generic_get_group(struct pinctrl_dev *pctldev,
					     unsigned int selector)
{
	if (selector >= pctldev->num_groups)
		return NULL;

	return &pctldev->groups[selector];
}
EXPORT_SYMBOL_GPL(pinctrl_generic_get_group);

static int pinctrl_generic_group_name_to_selector(struct pinctrl_dev *pctldev,
						  const char *function)
{
	const struct pinctrl_ops *ops = pctldev->ops;
	int selector = 0;
	int ngroups;

	if (!ops->get_groups_count || !ops->get_group_name)
		return -EINVAL;

	ngroups = ops->get_groups_count(pctldev);

	/* See if this pctldev has this group */
	while (selector < ngroups) {
		const char *gname = ops->get_group_name(pctldev, selector);

		if (gname && !strcmp(function, gname))
			return selector;

		selector++;
	}

	return -EINVAL;
}

/**
 * pinctrl_generic_add_group() - adds a new pin group
 * @pctldev: pin controller device
 * @name: name of the pin group
 * @pins: pins in the pin group
 * @num_pins: number of pins in the pin group
 * @data: pin controller driver specific data
 *
 * Note that the caller must take care of locking.
 */
int pinctrl_generic_add_group(struct pinctrl_dev *pctldev, const char *name,
			      const unsigned int *pins, int num_pins, void *data)
{
	int selector;

	if (!name)
		return -EINVAL;

	selector = pinctrl_generic_group_name_to_selector(pctldev, name);
	if (selector >= 0)
		return selector;

	selector = pctldev->num_groups;

	pctldev->groups = xrealloc(pctldev->groups,
				   (selector + 1) * sizeof(*pctldev->groups));
	pctldev->groups[selector] = PINCTRL_GROUP_DESC(name, pins, num_pins, data);

	pctldev->num_groups++;

	return selector;
}
EXPORT_SYMBOL_GPL(pinctrl_generic_add_group);

#endif /* CONFIG_GENERIC_PINCTRL_GROUPS */

#ifdef CONFIG_GENERIC_PINMUX_FUNCTIONS

/**
 * pinmux_generic_get_function_count() - returns number of functions
 * @pctldev: pin controller device
 */
int pinmux_generic_get_function_count(struct pinctrl_dev *pctldev)
{
	return pctldev->num_functions;
}
EXPORT_SYMBOL_GPL(pinmux_generic_get_function_count);

/**
 * pinmux_generic_get_function_name() - returns the function name
 * @pctldev: pin controller device
 * @selector: function number
 */
const char *
pinmux_generic_get_function_name(struct pinctrl_dev *pctldev,
				 unsigned int selector)
{
	const struct function_desc *function;

	function = pinmux_generic_get_function(pctldev, selector);
	if (!function)
		return NULL;

	return function->func->name;
}
EXPORT_SYMBOL_GPL(pinmux_generic_get_function_name);

/**
 * pinmux_generic_get_function() - returns a function based on the number
 * @pctldev: pin controller device
 * @selector: function number
 *
 * The pointer is only valid until the next pinmux_generic_add_pinfunction().
 */
const struct function_desc *
pinmux_generic_get_function(struct pinctrl_dev *pctldev, unsigned int selector)
{
	if (selector >= pctldev->num_functions)
		return NULL;

	return &pctldev->functions[selector];
}
EXPORT_SYMBOL_GPL(pinmux_generic_get_function);

static int pinmux_func_name_to_selector(struct pinctrl_dev *pctldev,
					const char *function)
{
	const struct pinmux_ops *ops = pctldev->pmxops;
	unsigned int nfuncs = ops->get_functions_count(pctldev);
	unsigned int selector = 0;

	/* See if this pctldev has this function */
	while (selector < nfuncs) {
		const char *fname = ops->get_function_name(pctldev, selector);

		if (fname && !strcmp(function, fname))
			return selector;

		selector++;
	}

	return -EINVAL;
}

/**
 * pinmux_generic_add_pinfunction() - adds a function group
 * @pctldev: pin controller device
 * @func: pinfunction structure describing the function group
 * @data: pin controller driver specific data
 */
int pinmux_generic_add_pinfunction(struct pinctrl_dev *pctldev,
				   const struct pinfunction *func, void *data)
{
	struct function_desc *function;
	int selector;

	selector = pinmux_func_name_to_selector(pctldev, func->name);
	if (selector >= 0)
		return selector;

	selector = pctldev->num_functions;

	pctldev->functions = xrealloc(pctldev->functions,
				      (selector + 1) * sizeof(*pctldev->functions));
	function = &pctldev->functions[selector];
	function->func = xmemdup(func, sizeof(*func));
	function->data = data;

	pctldev->num_functions++;

	return selector;
}
EXPORT_SYMBOL_GPL(pinmux_generic_add_pinfunction);

#endif /* CONFIG_GENERIC_PINMUX_FUNCTIONS */


#ifdef CONFIG_PINCTRL_STATE_PARAM
static int pinctrl_state_param_set(struct param_d *p, void *priv)
{
	struct pinctrl_consumer_info *info = priv;

	return of_pinctrl_select_state(info->np, info->states[info->state]);
}

void of_pinctrl_register_consumer(struct device *dev,
				  struct device_node *np)
{
	struct pinctrl_consumer_info *info;
	int ret, nstates;

	nstates = of_property_count_strings(np, "pinctrl-names");
	if (nstates <= 0)
		return;

	info = malloc(struct_size(info, states, nstates));
	if (!info)
		return;

	info->dev = dev;
	info->np = np;
	info->state = PARAM_ENUM_UNKNOWN;

	ret = of_property_read_string_array(np, "pinctrl-names", info->states, nstates);
	if (ret < 0)
		return;

	list_add(&info->list, &pinctrl_consumer_list);

	dev_add_param_enum(dev, "pinctrl_state",
			   pinctrl_state_param_set, NULL,
			   &info->state, info->states, nstates, info);
}

void of_pinctrl_unregister_consumer(struct device *dev)
{
	struct pinctrl_consumer_info *info;

	list_for_each_entry(info, &pinctrl_consumer_list, list) {
		if (info->dev != dev)
			continue;

		list_del(&info->list);
		return;
	}
}
#endif

static int pinctrl_change_policy(struct notifier_block *nb,
				  unsigned long _ignored,
				  void *_data)
{
	struct pinctrl_devices_name_info *dev;

	list_for_each_entry(dev, &pinctrl_devices_name_list, list) {
		of_pinctrl_select_state(dev->node, dev->name);
	}
	return 0;
}

static struct notifier_block pinctrl_policy_notifier = {
	.notifier_call = pinctrl_change_policy
};

static int pinctrl_policy_init(void)
{
	if (!IS_ENABLED(CONFIG_SECURITY_POLICY_PINCTRL))
		return 0;

	return notifier_chain_register(&sconfig_name_notifier, &pinctrl_policy_notifier);
}
pure_initcall(pinctrl_policy_init);
