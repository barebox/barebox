/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __GPIO_H
#define __GPIO_H

#include <slice.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/iopoll.h>
#include <linux/bitops.h>
#include <linux/bug.h>

#ifdef CONFIG_GENERIC_GPIO
void gpio_set_value(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_direction_input(unsigned gpio);
#else
static inline void gpio_set_value(unsigned gpio, int value)
{
}
static inline int gpio_get_value(unsigned gpio)
{
	return 0;
}
static inline int gpio_direction_output(unsigned gpio, int value)
{
	return -EINVAL;
}
static inline int gpio_direction_input(unsigned gpio)
{
	return -EINVAL;
}
#endif

struct device;

#ifdef CONFIG_GPIOLIB
void gpio_set_active(unsigned gpio, bool state);
int gpio_is_active(unsigned gpio);
int gpio_direction_active(unsigned gpio, bool state);
struct gpio_chip *gpio_get_chip_by_dev(struct device *);
struct gpio_chip *of_gpio_get_chip_by_alias(const char *alias);

/**
 * gpio_poll_timeout_us - Poll till GPIO reaches requested active state
 * @gpio: gpio to poll
 * @active: wait till GPIO is active if true, wait till it's inactive if false
 * @timeout_us: timeout in microseconds
 *
 * During the wait barebox pollers are called, if any.
 */
#define gpio_poll_timeout_us(gpio, active, timeout_us)			\
	({								\
		int __state;						\
		readx_poll_timeout(gpio_is_active, gpio, __state,	\
				   __state == (active), timeout_us);	\
	})
#else
static inline void gpio_set_active(unsigned gpio, int value)
{
}
static inline int gpio_is_active(unsigned gpio)
{
	return 0;
}
static inline int gpio_direction_active(unsigned gpio, int value)
{
	return -EINVAL;
}

static inline struct gpio_chip *gpio_get_chip_by_dev(struct device *dev)
{
	return NULL;
}

static inline struct gpio_chip *of_gpio_get_chip_by_alias(const char *alias)
{
	return NULL;
}

#define gpio_poll_timeout_us(gpio, val, timeout_us) (-ENOSYS)
#endif

#if defined(CONFIG_ARCH_NR_GPIO) && CONFIG_ARCH_NR_GPIO > 0
#define ARCH_NR_GPIOS CONFIG_ARCH_NR_GPIO
#else
#define ARCH_NR_GPIOS 256
#endif

static inline int gpio_is_valid(int gpio)
{
	if (gpio < 0)
		return 0;
	if (gpio < ARCH_NR_GPIOS)
		return 1;
	return 0;
}

#define GPIOF_DIR_OUT	(0 << 0)
#define GPIOF_DIR_IN	(1 << 0)

#define GPIOF_INIT_LOW	(0 << 1)
#define GPIOF_INIT_HIGH	(1 << 1)

#define GPIOF_IN		(GPIOF_DIR_IN)
#define GPIOF_OUT_INIT_LOW	(GPIOF_DIR_OUT | GPIOF_INIT_LOW)
#define GPIOF_OUT_INIT_HIGH	(GPIOF_DIR_OUT | GPIOF_INIT_HIGH)

#define GPIOF_LOGICAL		BIT(2)
#define GPIOF_ACTIVE_HIGH	GPIOF_LOGICAL
#define GPIOF_ACTIVE_LOW	(BIT(3) | GPIOF_LOGICAL)
#define GPIOF_INIT_INACTIVE	GPIOF_LOGICAL
#define GPIOF_INIT_ACTIVE	(GPIOF_LOGICAL | GPIOF_INIT_HIGH)
#define GPIOF_OUT_INIT_ACTIVE	(GPIOF_DIR_OUT | GPIOF_INIT_ACTIVE)
#define GPIOF_OUT_INIT_INACTIVE	(GPIOF_DIR_OUT | GPIOF_INIT_INACTIVE)

/**
 * struct gpio - a structure describing a GPIO with configuration
 * @gpio:	the GPIO number
 * @flags:	GPIO configuration as specified by GPIOF_*
 * @label:	a literal description string of this GPIO
 */
struct gpio {
	unsigned	gpio;
	unsigned long	flags;
	const char	*label;
};

#ifndef CONFIG_GPIOLIB
static inline int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

static inline int gpio_find_by_name(const char *name)
{
	return -ENOSYS;
}

static inline int gpio_find_by_label(const char *label)
{
	return -ENOSYS;
}

static inline void gpio_free(unsigned gpio)
{
}

static inline int gpio_request_one(unsigned gpio,
					unsigned long flags, const char *label)
{
	return -ENOSYS;
}

static inline int gpio_request_array(const struct gpio *array, size_t num)
{
	return -ENOSYS;
}

static inline void gpio_free_array(const struct gpio *array, size_t num)
{
	/* GPIO can never have been requested */
	WARN_ON(1);
}
static inline int gpio_array_to_id(const struct gpio *array, size_t num, u32 *val)
{
	return -EINVAL;
}

static inline bool gpio_slice_acquired(unsigned gpio)
{
	return false;
}

#else
int gpio_request(unsigned gpio, const char *label);
int gpio_find_by_name(const char *name);
int gpio_find_by_label(const char *label);
void gpio_free(unsigned gpio);
int gpio_request_one(unsigned gpio, unsigned long flags, const char *label);
int gpio_request_array(const struct gpio *array, size_t num);
void gpio_free_array(const struct gpio *array, size_t num);
int gpio_array_to_id(const struct gpio *array, size_t num, u32 *val);
bool gpio_slice_acquired(unsigned gpio);
#endif

struct gpio_chip;
struct of_phandle_args;

struct gpio_ops {
	int (*request)(struct gpio_chip *chip, unsigned offset);
	void (*free)(struct gpio_chip *chip, unsigned offset);
	int (*direction_input)(struct gpio_chip *chip, unsigned offset);
	int (*direction_output)(struct gpio_chip *chip, unsigned offset, int value);
	int (*get_direction)(struct gpio_chip *chip, unsigned offset);
	int (*get)(struct gpio_chip *chip, unsigned offset);
	void (*set)(struct gpio_chip *chip, unsigned offset, int value);

#if defined(CONFIG_OF_GPIO)
	/*
	 * If CONFIG_OF_GPIO is enabled, then all GPIO controllers described in
	 * the device tree automatically may have an OF translation
	 */

	/**
	 * @of_xlate:
	 *
	 * Callback to translate a device tree GPIO specifier into a chip-
	 * relative GPIO number and flags.
	 */
	int (*of_xlate)(struct gpio_chip *gc,
			const struct of_phandle_args *gpiospec, u32 *flags);
#endif
};

struct gpio_chip {
	struct device *dev;

	int base;
	/* GPIO controller specific GPIO offset */
	int gpio_offset;
	int ngpio;

#if defined(CONFIG_OF_GPIO)
	/**
	 * @of_gpio_n_cells:
	 *
	 * Number of cells used to form the GPIO specifier.
	 */
	unsigned int of_gpio_n_cells;
#endif

	struct gpio_ops *ops;

	struct slice slice;

	struct list_head list;
};

static inline struct slice *gpiochip_slice(struct gpio_chip *chip)
{
	return &chip->slice;
}

int gpiochip_add(struct gpio_chip *chip);
void gpiochip_remove(struct gpio_chip *chip);

int gpio_get_num(struct device *dev, int gpio);
struct gpio_chip *gpio_get_chip(int gpio);

#endif /* __GPIO_H */
