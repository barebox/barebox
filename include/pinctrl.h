/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PINCTRL_H
#define PINCTRL_H

#include <linux/types.h>
#include <linux/pinctrl/consumer.h>

struct pinctrl_device;
struct device_node;

struct pinctrl_ops {
	int (*set_state)(struct pinctrl_device *, struct device_node *);
	int (*set_direction)(struct pinctrl_device *, unsigned int, bool);
	int (*get_direction)(struct pinctrl_device *, unsigned int);
};

struct pinctrl_device {
	struct device *dev;
	struct pinctrl_ops *ops;
	struct list_head list;
	struct device_node *node;
	unsigned int base, npins;
};

int pinctrl_register(struct pinctrl_device *pdev);
void pinctrl_unregister(struct pinctrl_device *pdev);

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
