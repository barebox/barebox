// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-gpio.c - clock that can be enabled and disabled via GPIO output
 * Based on Linux clk support
 *
 * Copyright (c) 2018 Nikita Yushchenko <nikita.yoush@cogentembedded.com>
 */
#include <common.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <gpio.h>
#include <of_gpio.h>
#include <init.h>

struct clk_gpio {
	struct clk_hw hw;
	const char *parent;
	int gpio;
};
#define to_clk_gpio(_hw) container_of(_hw, struct clk_gpio, hw)

static int clk_gpio_enable(struct clk_hw *hw)
{
	struct clk_gpio *clk_gpio = to_clk_gpio(hw);

	gpio_set_active(clk_gpio->gpio, true);
	return 0;
}

static void clk_gpio_disable(struct clk_hw *hw)
{
	struct clk_gpio *clk_gpio = to_clk_gpio(hw);

	gpio_set_active(clk_gpio->gpio, false);
}

static int clk_gpio_is_enabled(struct clk_hw *hw)
{
	struct clk_gpio *clk_gpio = to_clk_gpio(hw);

	return gpio_is_active(clk_gpio->gpio);
}

static struct clk_ops clk_gpio_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = clk_gpio_enable,
	.disable = clk_gpio_disable,
	.is_enabled = clk_gpio_is_enabled,
};

static int of_gpio_clk_probe(struct device *dev)
{
	struct device_node *node = dev->device_node;
	struct clk_gpio *clk_gpio;
	enum of_gpio_flags of_flags;
	unsigned long flags;
	int ret;

	clk_gpio = xzalloc(sizeof(*clk_gpio));
	if (!clk_gpio)
		return -ENOMEM;

	clk_gpio->parent = of_clk_get_parent_name(node, 0);
	if (!clk_gpio->parent) {
		ret = -EINVAL;
		goto no_parent;
	}

	clk_gpio->hw.clk.ops = &clk_gpio_ops;
	clk_gpio->hw.clk.parent_names = &clk_gpio->parent;
	clk_gpio->hw.clk.num_parents = 1;

	clk_gpio->hw.clk.name = node->name;
	of_property_read_string(node, "clock-output-names",
			&clk_gpio->hw.clk.name);

	ret = of_get_named_gpio_flags(node, "enable-gpios", 0,
			&of_flags);
	if (ret >= 0 && !gpio_is_valid(ret))
		ret = -EINVAL;
	if (ret < 0)
		goto no_gpio;
	clk_gpio->gpio = ret;

	flags = GPIOF_OUT_INIT_ACTIVE;
	if (of_flags & OF_GPIO_ACTIVE_LOW)
		flags |= GPIOF_ACTIVE_LOW;
	ret = gpio_request_one(clk_gpio->gpio, flags, clk_gpio->hw.clk.name);
	if (ret)
		goto no_request;

	ret = bclk_register(&clk_gpio->hw.clk);
	if (ret)
		goto no_register;

	return of_clk_add_provider(node, of_clk_src_simple_get, &clk_gpio->hw.clk);

no_register:
	gpio_free(clk_gpio->gpio);
no_request:
no_gpio:
no_parent:
	free(clk_gpio);
	return ret;
}

static const struct of_device_id clk_gpio_device_id[] = {
	{ .compatible = "gpio-gate-clock", },
	{}
};
MODULE_DEVICE_TABLE(of, clk_gpio_device_id);

static struct driver gpio_gate_clock_driver = {
	.probe = of_gpio_clk_probe,
	.name = "gpio-gate-clock",
	.of_compatible = DRV_OF_COMPAT(clk_gpio_device_id),
};

core_platform_driver(gpio_gate_clock_driver);
