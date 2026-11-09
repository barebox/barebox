/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/pinctrl/pinmux.h?id=418a2bbdee2927d543db6913073d0e7ec8b540ee */
/*
 * Interface the pinmux subsystem
 *
 * Copyright (C) 2011 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * Based on bits of regulator core, gpio core and clk core
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */
#ifndef __LINUX_PINCTRL_PINMUX_H
#define __LINUX_PINCTRL_PINMUX_H

#include <linux/types.h>

struct pinctrl_dev;
struct pinctrl_gpio_range;

/**
 * struct pinmux_ops - pinmux operations, to be implemented by pin controller
 * drivers that support pinmuxing
 * @request: called by the core to see if a certain pin can be made
 *	available for muxing. This is called by the core to acquire the pins
 *	before selecting any actual mux setting across a function. The driver
 *	is allowed to answer "no" by returning a negative error code
 * @free: the reverse function of the request() callback, frees a pin after
 *	being requested
 * @get_functions_count: returns number of selectable named functions available
 *	in this pinmux driver
 * @get_function_name: return the function name of the muxing selector,
 *	called by the core to figure out which mux setting it shall map a
 *	certain device to
 * @set_mux: enable a certain muxing function with a certain pin group. The
 *	driver does not need to figure out whether enabling this function
 *	conflicts some other use of the pins in that group, such collisions
 *	are handled by the pinmux subsystem. The @func_selector selects a
 *	certain function whereas @group_selector selects a certain set of pins
 *	to be used. On simple controllers the latter argument may be ignored
 *	(barebox: U-Boot's pinmux_group_set() takes the selectors swapped)
 * @gpio_request_enable: requests and enables GPIO on a certain pin.
 *	Implement this only if you can mux every pin individually as GPIO. The
 *	affected GPIO range is passed along with an offset(pin number) into that
 *	specific GPIO range - function selectors and pin groups are orthogonal
 *	to this, the core will however make sure the pins do not collide.
 * @gpio_disable_free: free up GPIO muxing on a certain pin, the reverse of
 *	@gpio_request_enable
 * @gpio_set_direction: Since controllers may need different configurations
 *	depending on whether the GPIO is configured as input or output,
 *	a direction selector function may be implemented as a backing
 *	to the GPIO controllers that need pin muxing.
 * @gpio_get_direction: barebox-specific: read back the direction of a GPIO
 *	pin, returns GPIO_LINE_DIRECTION_IN or GPIO_LINE_DIRECTION_OUT
 * @pinmux_property_set: barebox-specific, from U-Boot: mux the pin one
 *	pinmux cell describes and return its pin number or a negative
 *	error code
 */
struct pinmux_ops {
	int (*request) (struct pinctrl_dev *pctldev, unsigned int offset);
	int (*free) (struct pinctrl_dev *pctldev, unsigned int offset);
	int (*get_functions_count) (struct pinctrl_dev *pctldev);
	const char *(*get_function_name) (struct pinctrl_dev *pctldev,
					  unsigned int selector);
	int (*set_mux) (struct pinctrl_dev *pctldev, unsigned int func_selector,
			unsigned int group_selector);
	int (*gpio_request_enable) (struct pinctrl_dev *pctldev,
				    struct pinctrl_gpio_range *range,
				    unsigned int offset);
	void (*gpio_disable_free) (struct pinctrl_dev *pctldev,
				   struct pinctrl_gpio_range *range,
				   unsigned int offset);
	int (*gpio_set_direction) (struct pinctrl_dev *pctldev,
				   struct pinctrl_gpio_range *range,
				   unsigned int offset,
				   bool input);
	int (*gpio_get_direction)(struct pinctrl_dev *pctldev,
				  struct pinctrl_gpio_range *range,
				  unsigned int offset);
	int (*pinmux_property_set)(struct pinctrl_dev *pctldev, u32 pinmux_group);
};

#endif /* __LINUX_PINCTRL_PINMUX_H */
