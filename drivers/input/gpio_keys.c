// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <clock.h>
#include <gpio_keys.h>
#include <poller.h>
#include <gpio.h>
#include <of_gpio.h>
#include <input/input.h>

struct gpio_key {
	int code;

	int gpio;
	int active_low;

	int previous_state;

	int debounce_interval;
	u64 debounce_start;
};

struct gpio_keys {
	struct gpio_key *buttons;
	int nbuttons;

	struct poller_struct poller;
	struct input_device input;
	struct device *dev;
};

static inline struct gpio_keys *
poller_to_gk_pdata(struct poller_struct *poller)
{
	return container_of(poller, struct gpio_keys, poller);
}

static void gpio_key_poller(struct poller_struct *poller)
{
	struct gpio_keys *gk = poller_to_gk_pdata(poller);
	struct gpio_key *gb;
	int i, val;

	for (i = 0; i < gk->nbuttons; i++) {

		gb = &gk->buttons[i];
		val = gpio_get_value(gb->gpio);

		if (!is_timeout(gb->debounce_start, gb->debounce_interval * MSECOND))
			continue;

		if (val != gb->previous_state) {
			int pressed = val != gb->active_low;

			gb->debounce_start = get_time_ns();
			input_report_key_event(&gk->input, gb->code, pressed);
			dev_dbg(gk->dev, "%s gpio(%d) as %d\n",
				pressed ? "pressed" : "released", gb->gpio, gb->code);
			gb->previous_state = val;
		}
	}
}

static int gpio_keys_probe_pdata(struct gpio_keys *gk, struct device *dev)
{
	struct gpio_keys_platform_data *pdata;
	int i;

	pdata = dev->platform_data;

	if (!pdata) {
		/* small (so we copy it) but critical! */
		dev_err(dev, "missing platform_data\n");
		return -ENODEV;
	}

	gk->buttons = xzalloc(pdata->nbuttons * sizeof(*gk->buttons));
	gk->nbuttons = pdata->nbuttons;

	for (i = 0; i < gk->nbuttons; i++) {
		gk->buttons[i].gpio = pdata->buttons[i].gpio;
		gk->buttons[i].code = pdata->buttons[i].code;
		gk->buttons[i].active_low = pdata->buttons[i].active_low;
		gk->buttons[i].debounce_interval = 20;
	}

	return 0;
}

static int gpio_keys_probe_dt(struct gpio_keys *gk, struct device *dev)
{
	struct device_node *npkey, *np = dev->of_node;
	int i = 0, ret;

	if (!IS_ENABLED(CONFIG_OFDEVICE) || !IS_ENABLED(CONFIG_OF_GPIO))
		return -ENODEV;

	gk->nbuttons = of_get_child_count(np);
	gk->buttons = xzalloc(gk->nbuttons * sizeof(*gk->buttons));

	for_each_child_of_node(np, npkey) {
		enum of_gpio_flags gpioflags;
		uint32_t keycode;

		gk->buttons[i].gpio = of_get_named_gpio_flags(npkey, "gpios", 0, &gpioflags);
		if (gk->buttons[i].gpio < 0)
			return gk->buttons[i].gpio;

		if (gpioflags & OF_GPIO_ACTIVE_LOW)
			gk->buttons[i].active_low = 1;

		ret = of_property_read_u32(npkey, "linux,code", &keycode);
		if (ret)
			return ret;

		gk->buttons[i].debounce_interval = 20;

		of_property_read_u32(npkey, "debounce-interval",
				     &gk->buttons[i].debounce_interval);

		gk->buttons[i].code = keycode;

		i++;
	}

	return 0;
}

static int __init gpio_keys_probe(struct device *dev)
{
	int ret, i, gpio;
	struct gpio_keys *gk;

	gk = xzalloc(sizeof(*gk));

	gk->dev = dev;

	if (dev->of_node)
		ret = gpio_keys_probe_dt(gk, dev);
	else
		ret = gpio_keys_probe_pdata(gk, dev);

	if (ret)
		return ret;

	for (i = 0; i < gk->nbuttons; i++) {
		gpio = gk->buttons[i].gpio;
		ret = gpio_request(gpio, "gpio_keys");
		if (ret) {
			pr_err("gpio_keys: (%d) can not be requested\n", gpio);
			return ret;
		}
		gpio_direction_input(gpio);
		gk->buttons[i].previous_state = gk->buttons[i].active_low;
	}

	gk->poller.func = gpio_key_poller;

	ret = input_device_register(&gk->input);
	if (ret)
		return ret;

	ret = poller_register(&gk->poller, dev_name(dev));
	if (ret)
		return ret;

	return 0;
}

static struct of_device_id key_gpio_of_ids[] = {
	{ .compatible = "gpio-keys", },
	{ }
};
MODULE_DEVICE_TABLE(of, key_gpio_of_ids);

static struct driver gpio_keys_driver = {
	.name	= "gpio_keys",
	.probe	= gpio_keys_probe,
	.of_compatible = DRV_OF_COMPAT(key_gpio_of_ids),
};
device_platform_driver(gpio_keys_driver);
