// SPDX-License-Identifier: GPL-2.0-only
/*
 * pinctrl.c - barebox pinctrl support
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 */
#include <common.h>
#include <malloc.h>
#include <pinctrl.h>
#include <linux/overflow.h>
#include <errno.h>
#include <of.h>

struct pinctrl {
	struct device_node consumer_np;
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

static struct pinctrl_device *pin_to_pinctrl(unsigned int pin)
{
	struct pinctrl_device *pinctrl;

	list_for_each_entry(pinctrl, &pinctrl_list, list)
		if (pin > pinctrl->base &&
		    pin < pinctrl->base + pinctrl->npins)
			return pinctrl;
	return NULL;
}

static int pinctrl_gpio_direction(unsigned pin, bool input)
{
	struct pinctrl_device *pinctrl = pin_to_pinctrl(pin);

	if (!pinctrl)
		return -EINVAL;

	BUG_ON(!pinctrl->ops->set_direction);

	return pinctrl->ops->set_direction(pinctrl, pin, input);
}

int pinctrl_gpio_direction_input(unsigned int pin)
{
	return pinctrl_gpio_direction(pin, true);
}

int pinctrl_gpio_direction_output(unsigned int pin)
{
	return pinctrl_gpio_direction(pin, false);
}

int pinctrl_gpio_get_direction(unsigned pin)
{
	struct pinctrl_device *pinctrl = pin_to_pinctrl(pin);

	if (!pinctrl)
		return -EINVAL;

	BUG_ON(!pinctrl->ops->get_direction);

	return pinctrl->ops->get_direction(pinctrl, pin);
}

static struct pinctrl_device *find_pinctrl(struct device_node *node)
{
	struct pinctrl_device *pdev;

	list_for_each_entry(pdev, &pinctrl_list, list)
		if (pdev->node == node)
			return pdev;
	return NULL;
}

static int pinctrl_config_one(struct device_node *for_node, struct device_node *np)
{
	struct pinctrl_device *pdev;
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

	pdev = find_pinctrl(pinctrl_node);
	if (pdev)
		return pdev->ops->set_state(pdev, np);
	else
		return -ENODEV;
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
	int ret = -ENODEV;
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

int of_pinctrl_select_state(struct device_node *np, const char *name)
{
	struct pinctrl *pinctrl = of_pinctrl_get(np);
	struct pinctrl_state *state;

	if (!of_find_property(np, "pinctrl-0", NULL))
		return 0;

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

int pinctrl_register(struct pinctrl_device *pdev)
{
	if (!IS_ENABLED(CONFIG_PINCTRL))
		return -ENOSYS;

	BUG_ON(!pdev->dev->of_node);

	list_add_tail(&pdev->list, &pinctrl_list);

	pdev->node = pdev->dev->of_node;

	pinctrl_select_state_default(pdev->dev);

	return 0;
}

void pinctrl_unregister(struct pinctrl_device *pdev)
{
	list_del(&pdev->list);
}


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
