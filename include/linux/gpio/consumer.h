/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_GPIO_CONSUMER_H
#define __LINUX_GPIO_CONSUMER_H

#include <gpio.h>
#include <of_gpio.h>
#include <driver.h>
#include <linux/err.h>
#include <errno.h>
#include <linux/iopoll.h>

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

#define GPIO_DESC_OK		BIT(BITS_PER_LONG - 1)

#define gpiod_not_found(desc)	(IS_ERR(desc) && PTR_ERR(desc) == -ENOENT)

struct gpio_desc;

static inline int __tmp_desc_to_gpio(struct gpio_desc *desc)
{
	if (!desc)
		return -ENOENT;
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	return ((ulong)desc & ~GPIO_DESC_OK);
}

static inline struct gpio_desc *__tmp_gpio_to_desc(int gpio)
{
	if (gpio == -ENOENT)
		return NULL;
	if (gpio < 0)
		return ERR_PTR(gpio);

	return (struct gpio_desc *)(gpio | GPIO_DESC_OK);
}

#ifdef CONFIG_OFDEVICE

/* returned gpio descriptor can be passed to any normal gpio_* function */
struct gpio_desc *dev_gpiod_get_index(struct device *dev,
			struct device_node *np,
			const char *_con_id, int index,
			enum gpiod_flags flags,
			const char *label);

#else
static inline struct gpio_desc *dev_gpiod_get_index(struct device *dev,
		struct device_node *np,
		const char *_con_id, int index,
		enum gpiod_flags flags,
		const char *label)
{
	return ERR_PTR(-ENODEV);
}
#endif

static inline struct gpio_desc *dev_gpiod_get(struct device *dev,
				struct device_node *np,
				const char *con_id,
				enum gpiod_flags flags,
				const char *label)
{
	return dev_gpiod_get_index(dev, np, con_id, 0, flags, label);
}

static inline struct gpio_desc *gpiod_get(struct device *dev,
			    const char *_con_id,
			    enum gpiod_flags flags)
{
	return dev_gpiod_get(dev, dev->of_node, _con_id, flags, NULL);
}

static inline struct gpio_desc *__must_check
gpiod_get_optional(struct device *dev, const char *con_id,
		   enum gpiod_flags flags)
{
	struct gpio_desc *desc;

	desc = gpiod_get(dev, con_id, flags);
	if (gpiod_not_found(desc))
		return NULL;

	return desc;
}

static inline int gpiod_direction_input(struct gpio_desc *gpiod)
{
	if (gpiod_not_found(gpiod))
		return 0;

	return gpio_direction_input(__tmp_desc_to_gpio(gpiod));
}

static inline int gpiod_direction_output(struct gpio_desc *gpiod, bool value)
{
	if (gpiod_not_found(gpiod))
		return 0;

	return gpio_direction_active(__tmp_desc_to_gpio(gpiod), value);
}

static inline int gpiod_set_value(struct gpio_desc *gpiod, bool value)
{
	return gpiod_direction_output(gpiod, value);
}

static inline int gpiod_get_value(struct gpio_desc *gpiod)
{
	if (gpiod_not_found(gpiod))
		return 0;
	if (IS_ERR(gpiod))
		return PTR_ERR(gpiod);

	return gpio_is_active(__tmp_desc_to_gpio(gpiod));
}

/**
 * gpiod_poll_timeout_us - poll till gpio descriptor reaches requested active state
 * @gpiod: gpio descriptor to poll
 * @active: wait till gpio is active if true, wait till it's inactive if false
 * @timeout_us: timeout in microseconds
 *
 * during the wait barebox pollers are called, if any.
 */
#define gpiod_poll_timeout_us(gpiod, active, timeout_us)		\
	({								\
		int __state;						\
		readx_poll_timeout(gpiod_get_value, gpiod, __state,	\
				   __state == (active), timeout_us);	\
	})

#endif
