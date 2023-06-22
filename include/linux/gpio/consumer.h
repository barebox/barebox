/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_GPIO_CONSUMER_H
#define __LINUX_GPIO_CONSUMER_H

#include <gpio.h>
#include <of_gpio.h>
#include <driver.h>
#include <linux/bug.h>
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

#define gpiod_not_found(desc)	(IS_ERR(desc) && PTR_ERR(desc) == -ENOENT)

struct gpio_desc;
struct gpio_array;

/**
 * struct gpio_descs - Struct containing an array of descriptors that can be
 *                     obtained using gpiod_get_array()
 *
 * @info:	Pointer to the opaque gpio_array structure
 * @ndescs:	Number of held descriptors
 * @desc:	Array of pointers to GPIO descriptors
 */
struct gpio_descs {
	unsigned int ndescs;
	/* info is used for fastpath, which we don't have in barebox.
	 * We define the member anyway, as not to change API
	 */
	struct gpio_array *info;
	DECLARE_FLEX_ARRAY(struct gpio_desc *, desc);
};

#if defined(CONFIG_OFDEVICE) && defined(CONFIG_GPIOLIB)

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

#ifdef CONFIG_GPIOLIB

int gpiod_direction_input(struct gpio_desc *desc);

int gpiod_direction_output_raw(struct gpio_desc *desc, int value);
int gpiod_direction_output(struct gpio_desc *desc, int value);

void gpiod_set_raw_value(struct gpio_desc *desc, int value);
void gpiod_set_value(struct gpio_desc *desc, int value);

int gpiod_get_raw_value(const struct gpio_desc *desc);
int gpiod_get_value(const struct gpio_desc *desc);

void gpiod_put(struct gpio_desc *desc);

int gpiod_count(struct device *dev, const char *con_id);

struct gpio_descs *__must_check gpiod_get_array(struct device *dev,
						const char *con_id,
						enum gpiod_flags flags);

void gpiod_put_array(struct gpio_descs *descs);

int gpiod_set_array_value(unsigned int array_size,
			  struct gpio_desc **desc_array,
			  struct gpio_array *array_info,
			  unsigned long *value_bitmap);

#else

static inline int gpiod_direction_input(struct gpio_desc *desc)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
	return 0;
}

static inline int gpiod_direction_output_raw(struct gpio_desc *desc, int value)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
	return 0;
}

static inline int gpiod_direction_output(struct gpio_desc *desc, int value)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
	return 0;
}

static inline void gpiod_set_raw_value(struct gpio_desc *desc, int value)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
}

static inline void gpiod_set_value(struct gpio_desc *desc, int value)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
}

static inline int gpiod_get_raw_value(const struct gpio_desc *desc)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
	return 0;
}

static inline int gpiod_get_value(const struct gpio_desc *desc)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
	return 0;
}

static inline void gpiod_put(struct gpio_desc *desc)
{
	/* GPIO can never have been requested */
	WARN_ON(desc);
}

static inline int gpiod_count(struct device *dev, const char *con_id)
{
	return 0;
}

static inline struct gpio_descs *__must_check
gpiod_get_array(struct device *dev, const char *con_id, enum gpiod_flags flags)
{
	return ERR_PTR(-ENOSYS);
}

static inline void gpiod_put_array(struct gpio_descs *descs)
{
	/* GPIO can never have been requested */
	WARN_ON(descs);
}

static inline int gpiod_set_array_value(unsigned int array_size,
			  struct gpio_desc **desc_array,
			  struct gpio_array *array_info,
			  unsigned long *value_bitmap)
{
	/* GPIO can never have been requested */
	WARN_ON(desc_array);
	return 0;
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
