/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __GPIOD_H_
#define __GPIOD_H_

#include <gpio.h>
#include <of_gpio.h>
#include <driver.h>

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

#ifdef CONFIG_OFDEVICE

/* returned gpio descriptor can be passed to any normal gpio_* function */
int dev_gpiod_get_index(struct device *dev,
			struct device_node *np,
			const char *_con_id, int index,
			enum gpiod_flags flags,
			const char *label);

#else
static inline int dev_gpiod_get_index(struct device *dev,
		struct device_node *np,
		const char *_con_id, int index,
		enum gpiod_flags flags,
		const char *label)
{
	return -ENODEV;
}
#endif

static inline int dev_gpiod_get(struct device *dev,
				struct device_node *np,
				const char *con_id,
				enum gpiod_flags flags,
				const char *label)
{
	return dev_gpiod_get_index(dev, np, con_id, 0, flags, label);
}

static inline int gpiod_get(struct device *dev,
			    const char *_con_id,
			    enum gpiod_flags flags)
{
	return dev_gpiod_get(dev, dev->of_node, _con_id, flags, NULL);
}

static inline void gpiod_direction_input(int gpio)
{
	gpio_direction_input(gpio);
}

static inline void gpiod_direction_output(int gpio, bool value)
{
	if (gpio != -ENOENT)
		gpio_direction_active(gpio, value);
}

static inline void gpiod_set_value(int gpio, bool value)
{
	gpiod_direction_output(gpio, value);
}

static inline int gpiod_get_value(int gpio)
{
	if (gpio < 0)
		return gpio;

	return gpio_is_active(gpio);
}

#endif
