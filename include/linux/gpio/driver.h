/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_GPIO_DRIVER_H
#define __LINUX_GPIO_DRIVER_H

#include <linux/gpio/defs.h>
#include <gpio.h>

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
