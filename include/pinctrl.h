/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PINCTRL_H
#define PINCTRL_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>

struct device_node;

/**
 * struct pinctrl_dev - pin controller device
 * @list: node in the global list of pin controllers
 * @desc: the pin controller descriptor, optional
 * @dev: the device entry for this pin controller
 * @driver_data: driver data for the pin controller driver
 * @ops: pin control operations
 * @pmxops: pinmux operations, optional
 * @confops: pin configuration operations, optional
 * @node: device tree node of the pin controller
 * @base: first pin handled by this controller in the global GPIO number
 *	space, only used for pinctrl_gpio_direction_*_pin()
 * @npins: number of pins starting at @base
 *
 * Unlike in Linux, this is public, so drivers can embed it and call
 * pinctrl_register().
 */
struct pinctrl_dev {
	struct list_head list;
	const struct pinctrl_desc *desc;
	struct device *dev;
	void *driver_data;
	const struct pinctrl_ops *ops;
	const struct pinmux_ops *pmxops;
	const struct pinconf_ops *confops;
	struct device_node *node;
	unsigned int base, npins;
};

int pinctrl_register(struct pinctrl_dev *pdev);

/* Legacy operations that take a global pin number */
#ifdef CONFIG_PINCTRL
int pinctrl_gpio_direction_input_pin(unsigned int pin);
int pinctrl_gpio_direction_output_pin(unsigned int pin);
int pinctrl_gpio_get_direction_pin(unsigned int pin);
#else
static inline int pinctrl_gpio_direction_input_pin(unsigned int pin)
{
	return -ENOTSUPP;
}

static inline int pinctrl_gpio_direction_output_pin(unsigned int pin)
{
	return -ENOTSUPP;
}

static inline int pinctrl_gpio_get_direction_pin(unsigned int pin)
{
	return -ENOTSUPP;
}
#endif

#ifdef CONFIG_PINCTRL_STATE_PARAM
void of_pinctrl_register_consumer(struct device *dev, struct device_node *np);
void of_pinctrl_unregister_consumer(struct device *dev);
#else
static inline void of_pinctrl_register_consumer(struct device *dev, struct device_node *np)
{
}
static inline void of_pinctrl_unregister_consumer(struct device *dev) {}
#endif

#endif /* PINCTRL_H */
