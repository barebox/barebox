/*
 * Copyright (c) 2017 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * Based on analogous code from Linux kernel
 *
 * Copyright (C) 2011, 2012 Cavium, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/mdio-mux.h>
#include <gpio.h>
#include <of_gpio.h>


struct mdio_mux_gpio_state {
	struct gpio *gpios;
	int gpios_num;
};

static void mdio_mux_gpio_set_active(struct mdio_mux_gpio_state *s,
				     unsigned int mask, int value)
{
	while (mask) {
		const int n = __ffs(mask);
		mask >>= n + 1;

		if (WARN_ON(n >= s->gpios_num))
			break;

		gpio_set_active(s->gpios[n].gpio, value);
	}
}

static int mdio_mux_gpio_switch_fn(int current_child, int desired_child,
				   void *data)
{
	if (current_child < 0)
		current_child = 0;

	if (current_child != desired_child) {
		struct mdio_mux_gpio_state *s = data;
		/*
		 * GPIOs that are set in both current and desired
		 * states can be left untouched. So we calculate them
		 * and clear corresponding bits in both states.
		 */
		const int unchanged = current_child & desired_child;

		current_child &= ~unchanged;
		desired_child &= ~unchanged;

		/*
		 * First step: disable all of the currently selected
		 * bits that are no loger needed
		 */
		mdio_mux_gpio_set_active(s, current_child, 0);
		/*
		 * Second step: enable all of the GPIOs that are
		 * desired to be selected
		 */
		mdio_mux_gpio_set_active(s, desired_child, 1);
	}

	return 0;
}

static int mdio_mux_gpio_probe(struct device_d *dev)
{
	struct mdio_mux_gpio_state *s;
	int i, r;

	s = xzalloc(sizeof(*s));

	s->gpios_num = of_gpio_count(dev->device_node);
	if (s->gpios_num <= 0) {
		dev_err(dev, "No GPIOs specified\n");
		r = -EINVAL;
		goto free_mem;
	}

	s->gpios = xzalloc(s->gpios_num * sizeof(s->gpios[0]));

	for (i = 0; i < s->gpios_num; i++) {
		enum of_gpio_flags flags;

		r = of_get_gpio_flags(dev->device_node, i, &flags);
		if (!gpio_is_valid(r)) {
			r = (r < 0) ? r : -EINVAL;
			goto free_mem;
		}

		s->gpios[i].gpio  = r;
		s->gpios[i].label = dev_name(dev);
		s->gpios[i].flags = GPIOF_OUT_INIT_INACTIVE;

		if (flags & OF_GPIO_ACTIVE_LOW)
			s->gpios[i].flags |= GPIOF_ACTIVE_LOW;
	}

	r = gpio_request_array(s->gpios, s->gpios_num);
	if (r < 0)
		goto free_gpios;


	r = mdio_mux_init(dev, dev->device_node,
			  mdio_mux_gpio_switch_fn, s, NULL);
	if (r < 0)
		goto free_gpios;

	return 0;

free_gpios:
	gpio_free_array(s->gpios, s->gpios_num);
free_mem:
	free(s->gpios);
	free(s);
	return r;
}

static const struct of_device_id mdio_mux_gpio_match[] = {
	{
		.compatible = "mdio-mux-gpio",
	},
	{},
};

static struct driver_d mdio_mux_gpio_driver = {
	.name	       = "mdio-mux-gpio",
	.probe         = mdio_mux_gpio_probe,
	.of_compatible = mdio_mux_gpio_match,
};
device_platform_driver(mdio_mux_gpio_driver);
