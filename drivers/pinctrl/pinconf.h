/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/pinctrl/pinconf.h?id=6a350ccc4dc3f46ed2ee2592e3050956e415ef64 */
/*
 * Internal interface between the core pin control system and the
 * pin config portions
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * Based on bits of regulator core, gpio core and clk core
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */

#include <linux/errno.h>

struct device_node;

struct pinctrl_dev;

int pinconf_set_config(struct pinctrl_dev *pctldev, unsigned int pin,
		       unsigned long *configs, size_t nconfigs);

#if defined(CONFIG_GENERIC_PINCONF) && defined(CONFIG_OF)
int pinconf_generic_parse_dt_config(struct device_node *np,
				    struct pinctrl_dev *pctldev,
				    unsigned long **configs,
				    unsigned int *nconfigs);

int pinconf_generic_parse_dt_pinmux(struct device_node *np, struct device *dev,
				    unsigned int **pid, unsigned int **pmux,
				    unsigned int *npins);
#else
static inline int
pinconf_generic_parse_dt_config(struct device_node *np,
				struct pinctrl_dev *pctldev,
				unsigned long **configs,
				unsigned int *nconfigs)
{
	return -ENOTSUPP;
}

static inline int
pinconf_generic_parse_dt_pinmux(struct device_node *np, struct device *dev,
				unsigned int **pid, unsigned int **pmux,
				unsigned int *npins)
{
	return -ENOTSUPP;
}
#endif

