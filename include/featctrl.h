/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __FEATCTRL_H_
#define __FEATCTRL_H_

#include <linux/list.h>

struct feature_controller;
struct device_node;

struct feature_controller {
	struct device_d *dev;
	int (*check)(struct feature_controller *, int idx);
	struct list_head list;
};

enum { FEATCTRL_GATED = 0, FEATCTRL_OKAY = 1 };

int feature_controller_register(struct feature_controller *);

#ifdef CONFIG_FEATURE_CONTROLLER
int of_feature_controller_check(struct device_node *np);
#else
static inline int of_feature_controller_check(struct device_node *np)
{
	return FEATCTRL_OKAY;
}
#endif

#endif /* PINCTRL_H */
