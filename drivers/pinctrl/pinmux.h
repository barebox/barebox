/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/pinctrl/pinmux.h?id=11aa02d6a9c222260490f952d041dec6d7f16a92 */
/*
 * Internal interface between the core pin control system and the
 * pinmux portions
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * Based on bits of regulator core, gpio core and clk core
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */

#include <linux/types.h>

struct pinctrl_dev;
struct pinctrl_gpio_range;

int pinmux_request_gpio(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range,
			unsigned int pin, unsigned int gpio);
void pinmux_free_gpio(struct pinctrl_dev *pctldev, unsigned int pin,
		      struct pinctrl_gpio_range *range);
int pinmux_gpio_direction(struct pinctrl_dev *pctldev,
			  struct pinctrl_gpio_range *range,
			  unsigned int pin, bool input);

#ifdef CONFIG_GENERIC_PINMUX_FUNCTIONS

/**
 * struct function_desc - generic function descriptor
 * @func: generic data of the pin function (name and groups of pins)
 * @data: pin controller driver specific data
 */
struct function_desc {
	const struct pinfunction *func;
	void *data;
};

int pinmux_generic_get_function_count(struct pinctrl_dev *pctldev);

const char *
pinmux_generic_get_function_name(struct pinctrl_dev *pctldev,
				 unsigned int selector);

const struct function_desc *
pinmux_generic_get_function(struct pinctrl_dev *pctldev, unsigned int selector);

int pinmux_generic_add_pinfunction(struct pinctrl_dev *pctldev,
				   const struct pinfunction *func, void *data);

#endif /* CONFIG_GENERIC_PINMUX_FUNCTIONS */
