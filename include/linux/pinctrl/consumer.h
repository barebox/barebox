/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LINUX_PINCTRL_CONSUMER_H
#define LINUX_PINCTRL_CONSUMER_H

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <device.h>

struct device;
struct device_node;

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
#endif

static inline void pinctrl_put(struct pinctrl *pinctrl) {}

static inline struct pinctrl *pinctrl_get(struct device *dev)
{
	return (struct pinctrl *)dev_of_node(dev);
}

#endif /* LINUX_PINCTRL_CONSUMER_H */
