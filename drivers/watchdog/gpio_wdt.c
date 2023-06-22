// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for watchdog device controlled through GPIO-line
 *
 * Author: 2013, Alexander Shiyan <shc_work@mail.ru>
 */

#include <common.h>
#include <driver.h>
#include <watchdog.h>
#include <superio.h>
#include <linux/gpio/consumer.h>

enum {
	HW_ALGO_TOGGLE,
	HW_ALGO_LEVEL,
};

struct gpio_wdt_priv {
	struct gpio_desc *gpiod;
	bool		state;
	bool		started;
	unsigned int	hw_algo;
	struct watchdog	wdd;
};

static inline struct gpio_wdt_priv *to_gpio_wdt_priv(struct watchdog *wdd)
{
	return container_of(wdd, struct gpio_wdt_priv, wdd);
}

static void gpio_wdt_disable(struct gpio_wdt_priv *priv)
{
	/* Eternal ping */
	gpiod_set_value(priv->gpiod, 1);

	/* Put GPIO back to tristate */
	if (priv->hw_algo == HW_ALGO_TOGGLE)
		gpiod_direction_input(priv->gpiod);

	priv->started = false;
}

static void gpio_wdt_ping(struct gpio_wdt_priv *priv)
{
	switch (priv->hw_algo) {
	case HW_ALGO_TOGGLE:
		/* Toggle output pin */
		priv->state = !priv->state;
		gpiod_set_value(priv->gpiod, priv->state);
		break;
	case HW_ALGO_LEVEL:
		/* Pulse */
		gpiod_set_value(priv->gpiod, true);
		udelay(1);
		gpiod_set_value(priv->gpiod, false);
		break;
	}
}

static void gpio_wdt_start(struct gpio_wdt_priv *priv)
{
	priv->state = false;
	gpiod_direction_output(priv->gpiod, priv->state);
	priv->started = true;
}

static int gpio_wdt_set_timeout(struct watchdog *wdd, unsigned int new_timeout)
{
	struct gpio_wdt_priv *priv = to_gpio_wdt_priv(wdd);

	if (!new_timeout) {
		gpio_wdt_disable(priv);
		return 0;
	}

	if (!priv->started)
		gpio_wdt_start(priv);

	gpio_wdt_ping(priv);
	return 0;
}

static int gpio_wdt_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct gpio_wdt_priv *priv;
	enum gpiod_flags gflags;
	unsigned int hw_margin;
	const char *algo;
	int ret;

	priv = xzalloc(sizeof(*priv));

	ret = of_property_read_u32(np, "hw_margin_ms", &hw_margin);
	if (ret)
		return ret;

	/* Autoping is fixed at one ping every 500 ms. Round it up to a second */
	if (hw_margin < 1000)
		return -EINVAL;

	ret = of_property_read_string(np, "hw_algo", &algo);
	if (ret)
		return ret;
	if (!strcmp(algo, "toggle")) {
		priv->hw_algo = HW_ALGO_TOGGLE;
		gflags = GPIOD_IN;
	} else if (!strcmp(algo, "level")) {
		priv->hw_algo = HW_ALGO_LEVEL;
		gflags = GPIOD_OUT_LOW;
	} else {
		return -EINVAL;
	}

	priv->gpiod = gpiod_get(dev, NULL, gflags);
	if (IS_ERR(priv->gpiod))
		return PTR_ERR(priv->gpiod);

	priv->wdd.hwdev		= dev;
	priv->wdd.timeout_max	= hw_margin / 1000;
	priv->wdd.priority	= 129;
	priv->wdd.set_timeout	= gpio_wdt_set_timeout;

	return watchdog_register(&priv->wdd);
}

static const struct of_device_id gpio_wdt_dt_ids[] = {
	{ .compatible = "linux,wdt-gpio", },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_wdt_dt_ids);

static struct driver gpio_wdt_driver = {
	.name		= "gpio-wdt",
	.of_compatible	= gpio_wdt_dt_ids,
	.probe	= gpio_wdt_probe,
};
device_platform_driver(gpio_wdt_driver);

MODULE_AUTHOR("Alexander Shiyan <shc_work@mail.ru>");
MODULE_DESCRIPTION("GPIO Watchdog");
MODULE_LICENSE("GPL");
