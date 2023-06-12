// SPDX-License-Identifier: GPL-2.0
/*
 * GPIO latch driver
 *
 *  Copyright (C) 2022 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This driver implements a GPIO (or better GPO as there is no input)
 * multiplexer based on latches like this:
 *
 * CLK0 ----------------------.        ,--------.
 * CLK1 -------------------.  `--------|>    #0 |
 *                         |           |        |
 * OUT0 ----------------+--|-----------|D0    Q0|-----|<
 * OUT1 --------------+-|--|-----------|D1    Q1|-----|<
 * OUT2 ------------+-|-|--|-----------|D2    Q2|-----|<
 * OUT3 ----------+-|-|-|--|-----------|D3    Q3|-----|<
 * OUT4 --------+-|-|-|-|--|-----------|D4    Q4|-----|<
 * OUT5 ------+-|-|-|-|-|--|-----------|D5    Q5|-----|<
 * OUT6 ----+-|-|-|-|-|-|--|-----------|D6    Q6|-----|<
 * OUT7 --+-|-|-|-|-|-|-|--|-----------|D7    Q7|-----|<
 *        | | | | | | | |  |           `--------'
 *        | | | | | | | |  |
 *        | | | | | | | |  |           ,--------.
 *        | | | | | | | |  `-----------|>    #1 |
 *        | | | | | | | |              |        |
 *        | | | | | | | `--------------|D0    Q0|-----|<
 *        | | | | | | `----------------|D1    Q1|-----|<
 *        | | | | | `------------------|D2    Q2|-----|<
 *        | | | | `--------------------|D3    Q3|-----|<
 *        | | | `----------------------|D4    Q4|-----|<
 *        | | `------------------------|D5    Q5|-----|<
 *        | `--------------------------|D6    Q6|-----|<
 *        `----------------------------|D7    Q7|-----|<
 *                                     `--------'
 *
 * The above is just an example. The actual number of number of latches and
 * the number of inputs per latch is derived from the number of GPIOs given
 * in the corresponding device tree properties.
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <gpio.h>
#include <init.h>
#include <of_gpio.h>
#include <linux/bitmap.h>

struct gpio_latch_priv {
	struct gpio_chip gc;
	int *clk_gpios;
	int *latched_gpios;
	int n_latched_gpios;
	unsigned int setup_duration_ns;
	unsigned int clock_duration_ns;
	unsigned long *shadow;
};

static int gpio_latch_get_direction(struct gpio_chip *gc, unsigned int offset)
{
	return 0;
}

static void gpio_latch_set(struct gpio_chip *gc, unsigned int offset, int val)
{
	struct gpio_latch_priv *priv = container_of(gc, struct gpio_latch_priv, gc);
	int latch = offset / priv->n_latched_gpios;
	int i;

	assign_bit(offset, priv->shadow, val);

	for (i = 0; i < priv->n_latched_gpios; i++)
		gpio_set_value(priv->latched_gpios[i],
			       test_bit(latch * priv->n_latched_gpios + i, priv->shadow));

	ndelay(priv->setup_duration_ns);
	gpio_set_value(priv->clk_gpios[latch], 1);
	ndelay(priv->clock_duration_ns);
	gpio_set_value(priv->clk_gpios[latch], 0);
}
static int gpio_latch_direction_output(struct gpio_chip *gc, unsigned gpio, int val)
{
	gpio_latch_set(gc, gpio, val);

	return 0;
}

#define DURATION_NS_MAX 5000

static struct gpio_ops gpio_latch_gpio_ops = {
	.direction_output = gpio_latch_direction_output,
	.set = gpio_latch_set,
	.get_direction = gpio_latch_get_direction,
};

static int gpio_latch_probe(struct device *dev)
{
	struct gpio_latch_priv *priv;
	int n_latches, i, ret;
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;

	priv = xzalloc(sizeof(*priv));

	n_latches = of_gpio_named_count(np, "clk-gpios");
	if (n_latches < 0) {
		dev_err(dev, "invalid or missing clk-gpios");
		ret = -EINVAL;
		goto err_gpio;
	}

	priv->n_latched_gpios = of_gpio_named_count(np, "latched-gpios");
	if (priv->n_latched_gpios < 0) {
		dev_err(dev, "invalid or missing latched-gpios");
		ret = -EINVAL;
		goto err_gpio;
	}

	priv->clk_gpios = xzalloc(sizeof(int) * n_latches);
	priv->latched_gpios = xzalloc(sizeof(int) * priv->n_latched_gpios);

	for (i = 0; i < n_latches; i++) {
		priv->clk_gpios[i] = of_get_named_gpio_flags(np, "clk-gpios",
								i, &flags);
		ret = gpio_request_one(priv->clk_gpios[i],
				       flags & OF_GPIO_ACTIVE_LOW ? GPIOF_ACTIVE_LOW : 0,
				       dev_name(dev));
		if (ret) {
			dev_err(dev, "Cannot request gpio %d: %s\n", priv->clk_gpios[i],
				strerror(-ret));
			goto err_gpio;
		}

		gpio_direction_output(priv->clk_gpios[i], 0);
	}

	for (i = 0; i < priv->n_latched_gpios; i++) {
		priv->latched_gpios[i] = of_get_named_gpio_flags(np, "latched-gpios",
							      i, &flags);
		ret = gpio_request_one(priv->latched_gpios[i],
				       flags & OF_GPIO_ACTIVE_LOW ? GPIOF_ACTIVE_LOW : 0,
				       dev_name(dev));
		if (ret) {
			dev_err(dev, "Cannot request gpio %d: %s\n", priv->latched_gpios[i],
				strerror(-ret));
			goto err_gpio;
		}
	}

	priv->shadow = bitmap_zalloc(n_latches * priv->n_latched_gpios);

	of_property_read_u32(np, "setup-duration-ns", &priv->setup_duration_ns);
	if (priv->setup_duration_ns > DURATION_NS_MAX) {
		dev_warn(dev, "setup-duration-ns too high, limit to %d\n",
			 DURATION_NS_MAX);
		priv->setup_duration_ns = DURATION_NS_MAX;
	}

	of_property_read_u32(np, "clock-duration-ns", &priv->clock_duration_ns);
	if (priv->clock_duration_ns > DURATION_NS_MAX) {
		dev_warn(dev, "clock-duration-ns too high, limit to %d\n",
			 DURATION_NS_MAX);
		priv->clock_duration_ns = DURATION_NS_MAX;
	}

	priv->gc.ops = &gpio_latch_gpio_ops;
	priv->gc.ngpio = n_latches * priv->n_latched_gpios;
	priv->gc.base = -1;
	priv->gc.dev = dev;

	return gpiochip_add(&priv->gc);

err_gpio:
	return ret;
}

static const struct of_device_id gpio_latch_ids[] = {
	{
		.compatible	= "gpio-latch",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, gpio_latch_ids);

static struct driver gpio_latch_driver = {
	.name = "gpio-latch",
	.probe = gpio_latch_probe,
	.of_compatible = DRV_OF_COMPAT(gpio_latch_ids),
};
device_platform_driver(gpio_latch_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sascha Hauer <s.hauer@pengutronix.de>");
MODULE_DESCRIPTION("GPIO latch driver");
