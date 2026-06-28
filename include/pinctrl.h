/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PINCTRL_H
#define PINCTRL_H

#include <linux/types.h>
#include <linux/pinctrl/consumer.h>

struct pinctrl_dev;
struct device_node;

struct pinctrl_ops {
	int (*set_state)(struct pinctrl_dev *, struct device_node *);
	int (*set_direction)(struct pinctrl_dev *, unsigned int, bool);
	int (*get_direction)(struct pinctrl_dev *, unsigned int);
};

struct pinctrl_dev {
	struct device *dev;
	const struct pinctrl_ops *ops;
	struct list_head list;
	struct device_node *node;
	unsigned int base, npins;
};

int pinctrl_register(struct pinctrl_dev *pdev);
void pinctrl_unregister(struct pinctrl_dev *pdev);

#ifdef CONFIG_PINCTRL_STATE_PARAM
void of_pinctrl_register_consumer(struct device *dev, struct device_node *np);
void of_pinctrl_unregister_consumer(struct device *dev);
#else
static inline void of_pinctrl_register_consumer(struct device *dev, struct device_node *np)
{
}
static inline void of_pinctrl_unregister_consumer(struct device *dev) {}
#endif

/* Legacy operations that take global pin number */
#ifdef CONFIG_PINCTRL
int pinctrl_gpio_direction_input_pin(unsigned pin);
int pinctrl_gpio_direction_output_pin(unsigned int pin);
int pinctrl_gpio_get_direction_pin(unsigned pin);
#else
static inline int pinctrl_gpio_direction_input_pin(unsigned pin)
{
	return -ENOTSUPP;
}

static inline int pinctrl_gpio_direction_output_pin(unsigned int pin)
{
	return -ENOTSUPP;
}

static inline int pinctrl_gpio_get_direction_pin(unsigned pin)
{
	return -ENOTSUPP;
}
#endif

#endif /* PINCTRL_H */
