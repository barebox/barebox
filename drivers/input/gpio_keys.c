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
#include <linux/gpio/consumer.h>
#include <input/input.h>

struct gpio_key {
	struct gpio_desc *gpio;
	int code;

	int previous_state;

	int debounce_interval;
	u64 debounce_start;
};

struct gpio_keys {
	struct gpio_key *buttons;
	int nbuttons;

	struct poller_async poller;
	struct input_device input;
};

static void gpio_key_poller(void *data)
{
	struct gpio_keys *gk = data;
	struct gpio_key *gb;
	int i, pressed;

	for (i = 0; i < gk->nbuttons; i++) {
		gb = &gk->buttons[i];

		if (gpiod_slice_acquired(gb->gpio))
			goto out;
	}

	for (i = 0; i < gk->nbuttons; i++) {

		gb = &gk->buttons[i];
		pressed = gpiod_get_value(gb->gpio);

		if (!is_timeout(gb->debounce_start, gb->debounce_interval * MSECOND))
			continue;

		if (pressed != gb->previous_state) {
			gb->debounce_start = get_time_ns();
			input_report_key_event(&gk->input, gb->code, pressed);
			dev_dbg(gk->input.parent, "%s gpio #%d as %d\n",
				pressed ? "pressed" : "released", i, gb->code);
			gb->previous_state = pressed;
		}
	}
out:
	poller_call_async(&gk->poller, 10 * MSECOND, gpio_key_poller, gk);
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
		struct gpio_desc *gpio;
		ulong flags = GPIOF_DIR_IN;

		if (pdata->buttons[i].active_low)
			flags |= GPIOF_ACTIVE_LOW;

		gpio = gpiod_request_one(pdata->buttons[i].gpio, flags, "gpio_keys");
		if (IS_ERR(gpio)) {
			pr_err("gpio_keys: gpio #%d can not be requested\n", i);
			return PTR_ERR(gpio);
		}

		gk->buttons[i].gpio = gpio;
		gk->buttons[i].code = pdata->buttons[i].code;
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
		const char *label = NULL;
		struct gpio_desc *gpio;
		uint32_t keycode;

		of_property_read_string(npkey, "label", &label);

		gpio = dev_gpiod_get(dev, npkey, NULL, GPIOD_IN, label);
		if (IS_ERR(gpio)) {
			pr_err("gpio_keys: gpio #%d can not be requested\n", i);
			return PTR_ERR(gpio);
		}

		gk->buttons[i].gpio = gpio;

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
	int ret;
	struct gpio_keys *gk;

	gk = xzalloc(sizeof(*gk));

	if (dev_of_node(dev))
		ret = gpio_keys_probe_dt(gk, dev);
	else
		ret = gpio_keys_probe_pdata(gk, dev);

	if (ret)
		return ret;

	gk->input.parent = dev;
	ret = input_device_register(&gk->input);
	if (ret)
		return ret;

	ret = poller_async_register(&gk->poller, dev_name(dev));
	if (ret)
		return ret;

	poller_call_async(&gk->poller, 10 * MSECOND, gpio_key_poller, gk);

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
