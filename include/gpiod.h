/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GPIOD_H_
#define __GPIOD_H_

#include <gpio.h>
#include <of_gpio.h>

/**
 * Optional flags that can be passed to one of gpiod_* to configure direction
 * and output value. These values cannot be OR'd.
 */
enum gpiod_flags {
	GPIOD_ASIS	= 0,
	GPIOD_IN	= GPIOF_IN,
	/*
	 * To change this later to a different logic level (i.e. taking
	 * active low into account), use gpiod_set_value()
	 */
	GPIOD_OUT_LOW	= GPIOF_OUT_INIT_INACTIVE,
	GPIOD_OUT_HIGH	= GPIOF_OUT_INIT_ACTIVE,
};

/* returned gpio descriptor can be passed to any normal gpio_* function */
int gpiod_get(struct device_d *dev, const char *_con_id, enum gpiod_flags flags);

static inline void gpiod_set_value(unsigned gpio, bool value)
{
	if (gpio != -ENOENT)
		gpio_direction_active(gpio, value);
}

#endif
