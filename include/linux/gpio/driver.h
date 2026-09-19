/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_GPIO_DRIVER_H
#define __LINUX_GPIO_DRIVER_H

#include <linux/gpio/defs.h>
#include <linux/list.h>
#include <linux/pinctrl/pinctrl.h>
#include <gpio.h>

struct pinctrl_dev;

/**
 * struct gpio_pin_range - pin range controlled by a gpio chip
 * @node: list for maintaining set of pin ranges, used internally
 * @pctldev: pinctrl device which handles corresponding pins
 * @range: actual range of pins controlled by a gpio controller
 */
struct gpio_pin_range {
	struct list_head node;
	struct pinctrl_dev *pctldev;
	struct pinctrl_gpio_range range;
};

/* get driver data */
static inline void *gpiochip_get_data(struct gpio_chip *gc)
{
	return gc->gpiodev_data;
}

static inline int gpiochip_add_data(struct gpio_chip *gc, void *data)
{
	int ret;

	gc->gpiodev_data = data;

	ret = gpiochip_add(gc);
	if (ret)
		gc->gpiodev_data = NULL;

	return ret;
}

#endif /* __LINUX_GPIO_DRIVER_H */
