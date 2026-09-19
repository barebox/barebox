// SPDX-License-Identifier: GPL-2.0-only
/*
 * pinctrl.c - barebox pinctrl support
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>
 */
#include <common.h>
#include <malloc.h>
#include <gpio.h>
#include <pinctrl.h>
#include "core.h"
#include "pinconf.h"
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

/**
 * pin_get_from_name() - look up a pin number from a name
 * @pctldev: the pin control device to lookup the pin on
 * @name: the name of the pin to look up
 */
int pin_get_from_name(struct pinctrl_dev *pctldev, const char *name)
{
	unsigned int i, pin;

	if (!pctldev->desc)
		return -EINVAL;

	/* The pin number can be retrived from the pin controller descriptor */
	for (i = 0; i < pctldev->desc->npins; i++) {
		const struct pinctrl_pin_desc *desc = &pctldev->desc->pins[i];

		pin = desc->number;
		/* Pin space may be sparse */
		if (desc->name && !strcmp(name, desc->name))
			return pin;
	}

	return -EINVAL;
}

/**
 * pinctrl_get_group_selector() - returns the group selector for a group
 * @pctldev: the pin controller handling the group
 * @pin_group: the pin group to look up
 */
int pinctrl_get_group_selector(struct pinctrl_dev *pctldev,
			       const char *pin_group)
{
	const struct pinctrl_ops *pctlops = pctldev->ops;
	unsigned int group_selector = 0;
	unsigned int ngroups;

	if (!pctlops->get_groups_count || !pctlops->get_group_name) {
		dev_err(pctldev->dev, "does not support pin groups\n");
		return -EINVAL;
	}

	ngroups = pctlops->get_groups_count(pctldev);

	while (group_selector < ngroups) {
		const char *gname = pctlops->get_group_name(pctldev,
							    group_selector);
		if (gname && !strcmp(gname, pin_group)) {
			dev_dbg(pctldev->dev,
				"found group selector %u for %s\n",
				group_selector,
				pin_group);
			return group_selector;
		}

		group_selector++;
	}

	dev_err(pctldev->dev, "does not have pin group %s\n",
		pin_group);

	return -EINVAL;
}

/**
 * pinctrl_get_group_pins() - return all pins for a group
 * @pctldev: the pin controller device
 * @pin_group: the name of the pin group
 * @pins: the pins in the group
 * @num_pins: the number of pins in the group
 */
int pinctrl_get_group_pins(struct pinctrl_dev *pctldev, const char *pin_group,
			   const unsigned int **pins, unsigned int *num_pins)
{
	const struct pinctrl_ops *pctlops = pctldev->ops;
	int gs;

	if (!pctlops->get_group_pins)
		return -EINVAL;

	gs = pinctrl_get_group_selector(pctldev, pin_group);
	if (gs < 0)
		return gs;

	return pctlops->get_group_pins(pctldev, gs, pins, num_pins);
}
EXPORT_SYMBOL_GPL(pinctrl_get_group_pins);

/**
 * pinmux_func_name_to_selector() - returns the function selector for a name
 * @pctldev: the pin controller handling the function
 * @function: the name of the function to look up
 */
int pinmux_func_name_to_selector(struct pinctrl_dev *pctldev,
				 const char *function)
{
	const struct pinmux_ops *ops = pctldev->pmxops;
	unsigned int nfuncs, selector = 0;

	if (!ops || !ops->get_functions_count || !ops->get_function_name)
		return -EINVAL;

	nfuncs = ops->get_functions_count(pctldev);

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
 * pinctrl_add_gpio_range() - register a GPIO range for a controller
 * @pctldev: pin controller device to add the range to
 * @range: the GPIO range to add
 *
 * This adds a range of GPIOs to be handled by a certain pin controller. Call
 * this to register handled ranges after registering your pin controller.
 */
void pinctrl_add_gpio_range(struct pinctrl_dev *pctldev,
			    struct pinctrl_gpio_range *range)
{
	list_add_tail(&range->node, &pctldev->gpio_ranges);
}
EXPORT_SYMBOL_GPL(pinctrl_add_gpio_range);

void pinctrl_add_gpio_ranges(struct pinctrl_dev *pctldev,
			     struct pinctrl_gpio_range *ranges,
			     unsigned int nranges)
{
	int i;

	for (i = 0; i < nranges; i++)
		pinctrl_add_gpio_range(pctldev, &ranges[i]);
}
EXPORT_SYMBOL_GPL(pinctrl_add_gpio_ranges);

/**
 * pinctrl_remove_gpio_range() - remove a range of GPIOs from a pin controller
 * @pctldev: pin controller device to remove the range from
 * @range: the GPIO range to remove
 */
void pinctrl_remove_gpio_range(struct pinctrl_dev *pctldev,
			       struct pinctrl_gpio_range *range)
{
	list_del(&range->node);
}
EXPORT_SYMBOL_GPL(pinctrl_remove_gpio_range);

struct pinctrl_dev *pinctrl_find_and_add_gpio_range(const char *devname,
		struct pinctrl_gpio_range *range)
{
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(devname);

	/*
	 * If we can't find this device, let's assume that is because
	 * it has not probed yet, so the driver trying to register this
	 * range need to defer probing.
	 */
	if (!pctldev)
		return ERR_PTR(-EPROBE_DEFER);

	pinctrl_add_gpio_range(pctldev, range);

	return pctldev;
}
EXPORT_SYMBOL_GPL(pinctrl_find_and_add_gpio_range);

/**
 * pinctrl_find_gpio_range_from_pin_nolock() - locate the GPIO range for a pin
 * @pctldev: the pin controller device to look in
 * @pin: a controller-local number to find the range for
 */
struct pinctrl_gpio_range *
pinctrl_find_gpio_range_from_pin_nolock(struct pinctrl_dev *pctldev,
					unsigned int pin)
{
	struct pinctrl_gpio_range *range;

	/* Loop over the ranges */
	list_for_each_entry(range, &pctldev->gpio_ranges, node) {
		/* Check if we're in the valid range */
		if (range->pins) {
			int a;
			for (a = 0; a < range->npins; a++) {
				if (range->pins[a] == pin)
					return range;
			}
		} else if (pin >= range->pin_base &&
			   pin < range->pin_base + range->npins)
			return range;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(pinctrl_find_gpio_range_from_pin_nolock);

struct pinctrl_gpio_range *
pinctrl_find_gpio_range_from_pin(struct pinctrl_dev *pctldev,
				 unsigned int pin)
{
	return pinctrl_find_gpio_range_from_pin_nolock(pctldev, pin);
}
EXPORT_SYMBOL_GPL(pinctrl_find_gpio_range_from_pin);

/* ranges use global GPIO numbers, chip offsets start at gc->gpio_offset */
static unsigned int gpiochip_gpio_number(struct gpio_chip *gc,
					 unsigned int offset)
{
	return gc->base + offset - gc->gpio_offset;
}

/**
 * pinctrl_match_gpio_range() - check if a certain GPIO pin is in range
 * @pctldev: pin controller device to check
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * Tries to match a GPIO pin number to the ranges handled by a certain pin
 * controller, return the range or NULL
 */
static struct pinctrl_gpio_range *
pinctrl_match_gpio_range(struct pinctrl_dev *pctldev, struct gpio_chip *gc,
			 unsigned int offset)
{
	unsigned int gpio = gpiochip_gpio_number(gc, offset);
	struct pinctrl_gpio_range *range;

	/* Loop over the ranges */
	list_for_each_entry(range, &pctldev->gpio_ranges, node) {
		/* Check if we're in the valid range */
		if (gpio >= range->base && gpio < range->base + range->npins)
			return range;
	}

	return NULL;
}

/**
 * pinctrl_ready_for_gpio_range() - check if other GPIO pins of
 * the same GPIO chip are in range
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * This function is complement of pinctrl_match_gpio_range(). If the return
 * value of pinctrl_match_gpio_range() is NULL, this function could be used
 * to check whether pinctrl device is ready or not. Maybe some GPIO pins
 * of the same GPIO chip don't have back-end pinctrl interface.
 * If the return value is true, it means that pinctrl device is ready & the
 * certain GPIO pin doesn't have back-end pinctrl device. If the return value
 * is false, it means that pinctrl device may not be ready.
 */
static bool pinctrl_ready_for_gpio_range(struct gpio_chip *gc,
					 unsigned int offset)
{
	struct pinctrl_dev *pctldev;
	struct pinctrl_gpio_range *range = NULL;

	/* Loop over the pin controllers */
	list_for_each_entry(pctldev, &pinctrl_list, list) {
		/* Loop over the ranges */
		list_for_each_entry(range, &pctldev->gpio_ranges, node) {
			/* Check if any gpio range overlapped with gpio chip */
			if (range->base + range->npins - 1 < gc->base ||
			    range->base > gc->base + gc->ngpio - 1)
				continue;
			return true;
		}
	}

	return false;
}

/**
 * pinctrl_get_device_gpio_range() - find device for GPIO range
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 * @outdev: the pin control device if found
 * @outrange: the GPIO range if found
 *
 * Find the pin controller handling a certain GPIO pin from the pinspace of
 * the GPIO subsystem, return the device and the matching GPIO range. Returns
 * -EPROBE_DEFER if the GPIO range could not be found in any device since it
 * may still have not been registered.
 */
static int pinctrl_get_device_gpio_range(struct gpio_chip *gc,
					 unsigned int offset,
					 struct pinctrl_dev **outdev,
					 struct pinctrl_gpio_range **outrange)
{
	struct pinctrl_dev *pctldev;

	/* Loop over the pin controllers */
	list_for_each_entry(pctldev, &pinctrl_list, list) {
		struct pinctrl_gpio_range *range;

		range = pinctrl_match_gpio_range(pctldev, gc, offset);
		if (range) {
			*outdev = pctldev;
			*outrange = range;
			return 0;
		}
	}

	return -EPROBE_DEFER;
}

/**
 * gpio_to_pin() - GPIO range GPIO number to pin number translation
 * @range: GPIO range used for the translation
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * Finds the pin number for a given GPIO using the specified GPIO range
 * as a base for translation. The distinction between linear GPIO ranges
 * and pin list based GPIO ranges is managed correctly by this function.
 *
 * This function assumes the gpio is part of the specified GPIO range, use
 * only after making sure this is the case (e.g. by calling it on the
 * result of successful pinctrl_get_device_gpio_range calls)!
 */
static inline int gpio_to_pin(struct pinctrl_gpio_range *range,
			      struct gpio_chip *gc, unsigned int offset)
{
	unsigned int pin = gpiochip_gpio_number(gc, offset) - range->base;
	if (range->pins)
		return range->pins[pin];
	else
		return range->pin_base + pin;
}

/**
 * pinmux_request_gpio() - request pinmuxing for a GPIO pin
 * @pctldev: pin controller device affected
 * @pin: the pin to mux in for GPIO
 * @range: the applicable GPIO range
 * @gpio: number of requested GPIO
 */
int pinmux_request_gpio(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range,
			unsigned int pin, unsigned int gpio)
{
	const struct pinmux_ops *ops = pctldev->pmxops;

	if (!ops)
		return 0;

	/*
	 * If there is no kind of request function for the pin we just assume
	 * we got it by default and proceed.
	 */
	if (range && ops->gpio_request_enable)
		/* This requests and enables a single GPIO pin */
		return ops->gpio_request_enable(pctldev, range, pin);
	else if (ops->request)
		return ops->request(pctldev, pin);
	else
		return 0;
}

/**
 * pinmux_free_gpio() - release a pin from GPIO muxing
 * @pctldev: the pin controller device for the pin
 * @pin: the affected currently GPIO-muxed in pin
 * @range: applicable GPIO range
 */
void pinmux_free_gpio(struct pinctrl_dev *pctldev, unsigned int pin,
		      struct pinctrl_gpio_range *range)
{
	const struct pinmux_ops *ops = pctldev->pmxops;

	if (!ops)
		return;

	if (range && ops->gpio_disable_free)
		ops->gpio_disable_free(pctldev, range, pin);
	else if (ops->free)
		ops->free(pctldev, pin);
}

/**
 * pinmux_gpio_direction() - set the direction of a single muxed-in GPIO pin
 * @pctldev: the pin controller handling this pin
 * @range: applicable GPIO range
 * @pin: the affected GPIO pin in this controller
 * @input: true if we set the pin as input, false for output
 */
int pinmux_gpio_direction(struct pinctrl_dev *pctldev,
			  struct pinctrl_gpio_range *range,
			  unsigned int pin, bool input)
{
	const struct pinmux_ops *ops = pctldev->pmxops;

	if (ops && ops->gpio_set_direction)
		return ops->gpio_set_direction(pctldev, range, pin, input);

	return 0;
}

int pinconf_set_config(struct pinctrl_dev *pctldev, unsigned int pin,
		       unsigned long *configs, size_t nconfigs)
{
	const struct pinconf_ops *ops = pctldev->confops;

	if (!ops || !ops->pin_config_set)
		return -ENOTSUPP;

	return ops->pin_config_set(pctldev, pin, configs, nconfigs);
}

/**
 * pinctrl_gpio_request() - request a single pin to be used as GPIO
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * This function should *ONLY* be used from gpiolib-based GPIO drivers,
 * as part of their gpio_request() semantics, platforms and individual drivers
 * shall *NOT* request GPIO pins to be muxed in.
 */
int pinctrl_gpio_request(struct gpio_chip *gc, unsigned int offset)
{
	struct pinctrl_gpio_range *range;
	struct pinctrl_dev *pctldev;
	int ret, pin;

	ret = pinctrl_get_device_gpio_range(gc, offset, &pctldev, &range);
	if (ret) {
		if (pinctrl_ready_for_gpio_range(gc, offset))
			ret = 0;
		return ret;
	}

	/* Convert to the pin controllers number space */
	pin = gpio_to_pin(range, gc, offset);

	return pinmux_request_gpio(pctldev, range, pin,
				   gpiochip_gpio_number(gc, offset));
}
EXPORT_SYMBOL_GPL(pinctrl_gpio_request);

/**
 * pinctrl_gpio_free() - free control on a single pin, currently used as GPIO
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * This function should *ONLY* be used from gpiolib-based GPIO drivers,
 * as part of their gpio_request() semantics, platforms and individual drivers
 * shall *NOT* request GPIO pins to be muxed in.
 */
void pinctrl_gpio_free(struct gpio_chip *gc, unsigned int offset)
{
	struct pinctrl_gpio_range *range;
	struct pinctrl_dev *pctldev;
	int ret, pin;

	ret = pinctrl_get_device_gpio_range(gc, offset, &pctldev, &range);
	if (ret)
		return;

	/* Convert to the pin controllers number space */
	pin = gpio_to_pin(range, gc, offset);

	pinmux_free_gpio(pctldev, pin, range);
}
EXPORT_SYMBOL_GPL(pinctrl_gpio_free);

static int pinctrl_gpio_direction(struct gpio_chip *gc, unsigned int offset,
				  bool input)
{
	struct pinctrl_dev *pctldev;
	struct pinctrl_gpio_range *range;
	int ret;
	int pin;

	ret = pinctrl_get_device_gpio_range(gc, offset, &pctldev, &range);
	if (ret) {
		return ret;
	}

	/* Convert to the pin controllers number space */
	pin = gpio_to_pin(range, gc, offset);

	return pinmux_gpio_direction(pctldev, range, pin, input);
}

/**
 * pinctrl_gpio_direction_input() - request a GPIO pin to go into input mode
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * This function should *ONLY* be used from gpiolib-based GPIO drivers,
 * as part of their gpio_direction_input() semantics, platforms and individual
 * drivers shall *NOT* touch pin control GPIO calls.
 */
int pinctrl_gpio_direction_input(struct gpio_chip *gc, unsigned int offset)
{
	return pinctrl_gpio_direction(gc, offset, true);
}
EXPORT_SYMBOL_GPL(pinctrl_gpio_direction_input);

/**
 * pinctrl_gpio_direction_output() - request a GPIO pin to go into output mode
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * This function should *ONLY* be used from gpiolib-based GPIO drivers,
 * as part of their gpio_direction_output() semantics, platforms and individual
 * drivers shall *NOT* touch pin control GPIO calls.
 */
int pinctrl_gpio_direction_output(struct gpio_chip *gc, unsigned int offset)
{
	return pinctrl_gpio_direction(gc, offset, false);
}
EXPORT_SYMBOL_GPL(pinctrl_gpio_direction_output);

/**
 * pinctrl_gpio_get_direction() - read back the direction of a GPIO pin
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 *
 * barebox-specific counterpart to pinctrl_gpio_direction_input/output().
 */
int pinctrl_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	struct pinctrl_dev *pctldev;
	struct pinctrl_gpio_range *range;
	int ret, pin;

	ret = pinctrl_get_device_gpio_range(gc, offset, &pctldev, &range);
	if (ret)
		return ret;

	if (!pctldev->pmxops || !pctldev->pmxops->gpio_get_direction)
		return -ENOTSUPP;

	pin = gpio_to_pin(range, gc, offset);

	return pctldev->pmxops->gpio_get_direction(pctldev, range, pin);
}
EXPORT_SYMBOL_GPL(pinctrl_gpio_get_direction);

/**
 * pinctrl_gpio_set_config() - Apply config to given GPIO pin
 * @gc: GPIO chip structure from the GPIO subsystem
 * @offset: hardware offset of the GPIO relative to the controller
 * @config: the configuration to apply to the GPIO
 *
 * This function should *ONLY* be used from gpiolib-based GPIO drivers, if
 * they need to call the underlying pin controller to change GPIO config
 * (for example set debounce time).
 */
int pinctrl_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
				unsigned long config)
{
	unsigned long configs[] = { config };
	struct pinctrl_gpio_range *range;
	struct pinctrl_dev *pctldev;
	int ret, pin;

	ret = pinctrl_get_device_gpio_range(gc, offset, &pctldev, &range);
	if (ret)
		return ret;

	pin = gpio_to_pin(range, gc, offset);

	return pinconf_set_config(pctldev, pin, configs, ARRAY_SIZE(configs));
}
EXPORT_SYMBOL_GPL(pinctrl_gpio_set_config);

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
		return pinctrl_map_set_state(pdev, np);

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
	INIT_LIST_HEAD(&pdev->gpio_ranges);

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
