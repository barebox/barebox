/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LINUX_PINCTRL_CONSUMER_H
#define LINUX_PINCTRL_CONSUMER_H

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <device.h>

struct device;
struct device_node;
struct gpio_chip;

struct pinctrl;

extern struct list_head pinctrl_list;

#ifdef CONFIG_PINCTRL
struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *pinctrl,
					   const char *name);
int pinctrl_select_state(struct pinctrl *pinctrl, struct pinctrl_state *state);
struct pinctrl *pinctrl_get_select(struct device *dev, const char *state);
int pinctrl_select_state_default(struct device *dev);
int of_pinctrl_select_state(struct device_node *np, const char *state);
int of_pinctrl_select_state_default(struct device_node *np);
int pinctrl_single_probe(struct device *dev);

/* External interface to pin control */
int pinctrl_gpio_request(struct gpio_chip *gc, unsigned int offset);
void pinctrl_gpio_free(struct gpio_chip *gc, unsigned int offset);
int pinctrl_gpio_direction_input(struct gpio_chip *gc,
				 unsigned int offset);
int pinctrl_gpio_direction_output(struct gpio_chip *gc,
				  unsigned int offset);
int pinctrl_gpio_get_direction(struct gpio_chip *gc, unsigned int offset);
int pinctrl_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
				unsigned long config);
#else
static inline struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *pinctrl,
							 const char *name)
{
        return NULL;
}
static inline int pinctrl_select_state(struct pinctrl *pinctrl, struct pinctrl_state *state)
{
	return 0;
}
static inline struct pinctrl *pinctrl_get_select(struct device *dev, const char *state)
{
	return ERR_PTR(-ENODEV);
}

static inline int pinctrl_select_state_default(struct device *dev)
{
	return -ENODEV;
}

static inline int of_pinctrl_select_state(struct device_node *np, const char *state)
{
	return -ENODEV;
}

static inline int of_pinctrl_select_state_default(struct device_node *np)
{
	return -ENODEV;
}

static inline int pinctrl_single_probe(struct device *dev)
{
	return -ENOSYS;
}

static inline int
pinctrl_gpio_request(struct gpio_chip *gc, unsigned int offset)
{
	return 0;
}

static inline void
pinctrl_gpio_free(struct gpio_chip *gc, unsigned int offset)
{
}

static inline int
pinctrl_gpio_direction_input(struct gpio_chip *gc, unsigned int offset)
{
	return 0;
}

static inline int
pinctrl_gpio_direction_output(struct gpio_chip *gc, unsigned int offset)
{
	return 0;
}

static inline int
pinctrl_gpio_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	return -ENOTSUPP;
}

static inline int
pinctrl_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
			    unsigned long config)
{
	return 0;
}
#endif

static inline void pinctrl_put(struct pinctrl *pinctrl) {}

static inline struct pinctrl *pinctrl_get(struct device *dev)
{
	return (struct pinctrl *)dev_of_node(dev);
}

#endif /* LINUX_PINCTRL_CONSUMER_H */
