/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/pinctrl/pinconf.h?id=7cc4e6b0e4ddf610477fcec8e3d2a9caae7e8a6c */
/*
 * Interface the pinconfig portions of the pinctrl subsystem
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * This interface is used in the core to keep track of pins.
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */
#ifndef __LINUX_PINCTRL_PINCONF_H
#define __LINUX_PINCTRL_PINCONF_H

#include <linux/types.h>

struct pinctrl_dev;

/**
 * struct pinconf_ops - pin config operations, to be implemented by
 * pin configuration capable drivers.
 * @is_generic: for pin controllers that want to use the generic interface,
 *	this flag tells the framework that it's generic.
 * @pin_config_set: configure an individual pin
 * @pin_config_group_set: configure all pins in a group
 */
struct pinconf_ops {
#ifdef CONFIG_GENERIC_PINCONF
	bool is_generic;
#endif
	int (*pin_config_set) (struct pinctrl_dev *pctldev,
			       unsigned int pin,
			       unsigned long *configs,
			       unsigned int num_configs);
	int (*pin_config_group_set) (struct pinctrl_dev *pctldev,
				     unsigned int selector,
				     unsigned long *configs,
				     unsigned int num_configs);
};

#endif /* __LINUX_PINCTRL_PINCONF_H */
