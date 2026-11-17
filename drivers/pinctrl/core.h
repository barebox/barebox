/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/pinctrl/core.h?id=9724e6f1953644cc9a5d102605d624bc79609038 */
/*
 * Core private header for the pin control subsystem
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */

#include <linux/list.h>
#include <linux/types.h>

#include <linux/pinctrl/machine.h>

#include <pinctrl.h>

struct device;
struct device_node;

struct pinctrl_desc;
struct pinctrl_gpio_range;

#ifdef CONFIG_GENERIC_PINCTRL_GROUPS

#include <linux/pinctrl/pinctrl.h>

/**
 * struct group_desc - generic pin group descriptor
 * @grp: generic data of the pin group (name and pins)
 * @data: pin controller driver specific data
 */
struct group_desc {
	struct pingroup grp;
	void *data;
};

/* Convenient macro to define a generic pin group descriptor */
#define PINCTRL_GROUP_DESC(_name, _pins, _num_pins, _data)	\
(struct group_desc) {						\
	.grp = PINCTRL_PINGROUP(_name, _pins, _num_pins),	\
	.data = _data,						\
}

int pinctrl_generic_get_group_count(struct pinctrl_dev *pctldev);

const char *pinctrl_generic_get_group_name(struct pinctrl_dev *pctldev,
					   unsigned int group_selector);

int pinctrl_generic_get_group_pins(struct pinctrl_dev *pctldev,
				   unsigned int group_selector,
				   const unsigned int **pins,
				   unsigned int *npins);

struct group_desc *pinctrl_generic_get_group(struct pinctrl_dev *pctldev,
					     unsigned int group_selector);

int pinctrl_generic_add_group(struct pinctrl_dev *pctldev, const char *name,
			      const unsigned int *pins, int num_pins, void *data);

#endif	/* CONFIG_GENERIC_PINCTRL_GROUPS */

struct pinctrl_dev *get_pinctrl_dev_from_devname(const char *dev_name);
struct pinctrl_dev *get_pinctrl_dev_from_of_node(struct device_node *np);
int pin_get_from_name(struct pinctrl_dev *pctldev, const char *name);
int pinctrl_get_group_selector(struct pinctrl_dev *pctldev,
			       const char *pin_group);

extern struct pinctrl_gpio_range *
pinctrl_find_gpio_range_from_pin_nolock(struct pinctrl_dev *pctldev,
					unsigned int pin);

