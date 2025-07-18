// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2013 - 2014 Texas Instruments Incorporated - https://www.ti.com
 *
 * Authors:
 *    Jyri Sarha <jsarha@ti.com>
 *    Sergej Sawazki <ce3a@gmx.de>
 *
 * Gpio controlled clock implementation
 */

#include <common.h>
#include <regulator.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>

/**
 * struct clk_gpio - gpio gated clock
 *
 * @hw:		handle between common and hardware-specific interfaces
 * @gpiod:	gpio descriptor
 *
 * Clock with a gpio control for enabling and disabling the parent clock
 * or switching between two parents by asserting or deasserting the gpio.
 *
 * Implements .enable, .disable and .is_enabled or
 * .get_parent, .set_parent and .determine_rate depending on which clk_ops
 * is used.
 */
struct clk_gpio {
	struct clk_hw	hw;
	struct gpio_desc *gpiod;
};

#define to_clk_gpio(_hw) container_of(_hw, struct clk_gpio, hw)

static int clk_gpio_gate_enable(struct clk_hw *hw)
{
	struct clk_gpio *clk = to_clk_gpio(hw);

	gpiod_set_value(clk->gpiod, 1);

	return 0;
}

static void clk_gpio_gate_disable(struct clk_hw *hw)
{
	struct clk_gpio *clk = to_clk_gpio(hw);

	gpiod_set_value(clk->gpiod, 0);
}

static int clk_gpio_gate_is_enabled(struct clk_hw *hw)
{
	struct clk_gpio *clk = to_clk_gpio(hw);

	return gpiod_get_value(clk->gpiod);
}

static const struct clk_ops clk_gpio_gate_ops = {
	.enable = clk_gpio_gate_enable,
	.disable = clk_gpio_gate_disable,
	.is_enabled = clk_gpio_gate_is_enabled,
};

static int clk_gpio_mux_get_parent(struct clk_hw *hw)
{
	struct clk_gpio *clk = to_clk_gpio(hw);

	return gpiod_get_value(clk->gpiod);
}

static int clk_gpio_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_gpio *clk = to_clk_gpio(hw);

	gpiod_set_value(clk->gpiod, index);

	return 0;
}

static const struct clk_ops clk_gpio_mux_ops = {
	.get_parent = clk_gpio_mux_get_parent,
	.set_parent = clk_gpio_mux_set_parent,
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
};

static struct clk_hw *clk_register_gpio(struct device *dev, u8 num_parents,
					struct gpio_desc *gpiod,
					const struct clk_ops *clk_gpio_ops)
{
	struct clk_gpio *clk_gpio;
	struct clk_hw *hw;
	struct clk_init_data init = {};
	const char *parent_names[2];

	clk_gpio = xzalloc(sizeof(*clk_gpio));
	if (!clk_gpio)
		return ERR_PTR(-ENOMEM);

	if (of_clk_parent_fill(dev->of_node, parent_names, num_parents) != num_parents)
		return ERR_PTR(-EINVAL);

	init.name = dev->of_node->name;
	of_property_read_string(dev->of_node, "clock-output-names", &init.name);
	init.ops = clk_gpio_ops;
	init.parent_names = parent_names;
	init.num_parents = num_parents;
	init.flags = CLK_SET_RATE_PARENT;

	clk_gpio->gpiod = gpiod;
	clk_gpio->hw.init = &init;

	hw = &clk_gpio->hw;
	return clk_to_clk_hw(clk_register(dev, hw));
}

static struct clk_hw *clk_hw_register_gpio_gate(struct device *dev,
						int num_parents,
						struct gpio_desc *gpiod)
{
	return clk_register_gpio(dev, num_parents, gpiod, &clk_gpio_gate_ops);
}

static struct clk_hw *clk_hw_register_gpio_mux(struct device *dev,
					       struct gpio_desc *gpiod)
{
	return clk_register_gpio(dev, 2, gpiod, &clk_gpio_mux_ops);
}

static int gpio_clk_driver_probe(struct device *dev)
{
	struct device_node *node = dev->of_node;
	const char *gpio_name;
	unsigned int num_parents;
	struct gpio_desc *gpiod;
	struct clk_hw *hw;
	bool is_mux;

	is_mux = of_device_is_compatible(node, "gpio-mux-clock");

	num_parents = of_clk_get_parent_count(node);
	if (is_mux && num_parents != 2)
		return dev_err_probe(dev, -EINVAL,
				     "mux-clock must have 2 parents\n");

	gpio_name = is_mux ? "select" : "enable";
	gpiod = gpiod_get(dev, gpio_name, GPIOD_OUT_LOW);
	if (IS_ERR(gpiod))
		return dev_err_probe(dev, PTR_ERR(gpiod),
				     "Can't get '%s' named GPIO property\n", gpio_name);

	if (is_mux)
		hw = clk_hw_register_gpio_mux(dev, gpiod);
	else
		hw = clk_hw_register_gpio_gate(dev, num_parents, gpiod);
	if (IS_ERR(hw))
		return PTR_ERR(hw);

	return of_clk_add_provider(node, of_clk_src_simple_get, &hw->clk);
}

static __maybe_unused const struct of_device_id clk_gpio_device_id[] = {
	{ .compatible = "gpio-mux-clock" },
	{ .compatible = "gpio-gate-clock" },
	{ }
};
MODULE_DEVICE_TABLE(of, clk_gpio_device_id);

static struct driver gpio_gate_clock_driver = {
	.probe = gpio_clk_driver_probe,
	.name = "gpio-gate-clock",
	.of_compatible = DRV_OF_COMPAT(clk_gpio_device_id),
};
core_platform_driver(gpio_gate_clock_driver);

/**
 * DOC: gated fixed clock, controlled with a gpio output and a regulator
 * Traits of this clock:
 * prepare - clk_prepare and clk_unprepare are function & control regulator
 *           optionally a gpio that can sleep
 * enable - clk_enable and clk_disable are functional & control gpio
 * rate - rate is fixed and set on clock registration
 * parent - fixed clock is a root clock and has no parent
 */

/**
 * struct clk_gated_fixed - Gateable fixed rate clock
 * @clk_gpio:	instance of clk_gpio for gate-gpio
 * @supply:	supply regulator
 * @rate:	fixed rate
 */
struct clk_gated_fixed {
	struct clk_gpio clk_gpio;
	struct regulator *supply;
	unsigned long rate;
};

#define to_clk_gated_fixed(_clk_gpio) container_of(_clk_gpio, struct clk_gated_fixed, clk_gpio)

static unsigned long clk_gated_fixed_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	return to_clk_gated_fixed(to_clk_gpio(hw))->rate;
}

static int clk_gated_fixed_enable(struct clk_hw *hw)
{
	struct clk_gated_fixed *clk = to_clk_gated_fixed(to_clk_gpio(hw));
	int ret;

	ret = regulator_enable(clk->supply);
	if (!ret)
		gpiod_set_value(clk->clk_gpio.gpiod, 1);

	return ret;
}

static void clk_gated_fixed_disable(struct clk_hw *hw)
{
	struct clk_gated_fixed *clk = to_clk_gated_fixed(to_clk_gpio(hw));

	gpiod_set_value(clk->clk_gpio.gpiod, 0);

	regulator_disable(clk->supply);
}

static int clk_gated_fixed_is_enabled(struct clk_hw *hw)
{
	struct clk_gated_fixed *clk = to_clk_gated_fixed(to_clk_gpio(hw));

	if (!regulator_is_enabled(clk->supply))
		return 0;

	return clk->clk_gpio.gpiod ? gpiod_get_value(clk->clk_gpio.gpiod) : 1;
}

static const struct clk_ops clk_gated_fixed_ops = {
	.enable = clk_gated_fixed_enable,
	.disable = clk_gated_fixed_disable,
	.is_enabled = clk_gated_fixed_is_enabled,
	.recalc_rate = clk_gated_fixed_recalc_rate,
};

static int clk_gated_fixed_probe(struct device *dev)
{
	struct clk_gated_fixed *clk;
	const char *clk_name;
	u32 rate;
	int ret;

	clk = xzalloc(sizeof(*clk));
	if (!clk)
		return -ENOMEM;

	ret = of_property_read_u32(dev->of_node, "clock-frequency", &rate);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get clock-frequency\n");
	clk->rate = rate;

	clk_name = dev->of_node->name;
	of_property_read_string(dev->of_node, "clock-output-names", &clk_name);

	clk->supply = regulator_get_optional(dev, "vdd");
	if (IS_ERR(clk->supply)) {
		if (PTR_ERR(clk->supply) != -ENODEV)
			return dev_err_probe(dev, PTR_ERR(clk->supply),
					     "Failed to get regulator\n");
		clk->supply = NULL;
	}

	clk->clk_gpio.gpiod = gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(clk->clk_gpio.gpiod))
		return dev_err_probe(dev, PTR_ERR(clk->clk_gpio.gpiod),
				     "Failed to get gpio\n");


	clk->clk_gpio.hw.init = CLK_HW_INIT_NO_PARENT(clk_name, &clk_gated_fixed_ops, 0);

	ret = clk_hw_register(dev, &clk->clk_gpio.hw);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register clock\n");

	ret = of_clk_add_hw_provider(dev->of_node, of_clk_hw_simple_get,
				     &clk->clk_gpio.hw);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to register clock provider\n");

	return 0;
}

static __maybe_unused const struct of_device_id gated_fixed_clk_match_table[] = {
	{ .compatible = "gated-fixed-clock" },
	{ }
};
MODULE_DEVICE_TABLE(of, gated_fixed_clk_match_table);

static struct driver gated_fixed_clk_driver = {
	.probe = clk_gated_fixed_probe,
	.name = "gated-fixed-clk",
	.of_compatible = DRV_OF_COMPAT(gated_fixed_clk_match_table),
};
core_platform_driver(gated_fixed_clk_driver);
