/*
 * gpio LED support for barebox
 *
 * (C) Copyright 2010 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <init.h>
#include <led.h>
#include <gpio.h>
#include <of_gpio.h>

static void led_gpio_set(struct led *led, unsigned int value)
{
	struct gpio_led *gpio_led = container_of(led, struct gpio_led, led);

	gpio_direction_output(gpio_led->gpio, !!value ^ gpio_led->active_low);
}

/**
 * led_gpio_register - register a gpio controlled LED
 * @param led	The gpio LED
 *
 * This function registers a single gpio as a LED. led->gpio
 * should be initialized to the gpio to control.
 */
int led_gpio_register(struct gpio_led *led)
{
	int ret;
	char *name = led->led.name;

	ret = gpio_request(led->gpio, name ? name : "led");
	if (ret)
		return ret;

	led->led.set = led_gpio_set;
	led->led.max_value = 1;

	ret = led_register(&led->led);
	if (ret)
		gpio_free(led->gpio);

	return ret;
}

/**
 * led_gpio_unregister - remove a gpio controlled LED from the framework
 * @param led	The gpio LED
 */
void led_gpio_unregister(struct gpio_led *led)
{
	led_unregister(&led->led);
}

#ifdef CONFIG_LED_GPIO_BICOLOR
static void led_gpio_bicolor_set(struct led *led, unsigned int value)
{
	struct gpio_bicolor_led *bi = container_of(led, struct gpio_bicolor_led, led);
	int al = bi->active_low;

	switch (value) {
	case 0:
		gpio_direction_output(bi->gpio_c0, al);
		gpio_direction_output(bi->gpio_c1, al);
		break;
	case 1:
		gpio_direction_output(bi->gpio_c0, !al);
		gpio_direction_output(bi->gpio_c1, al);
		break;
	case 2:
		gpio_direction_output(bi->gpio_c0, al);
		gpio_direction_output(bi->gpio_c1, !al);
		break;
	}
}

/**
 * led_gpio_bicolor_register - register three gpios as a bicolor LED
 * @param led	The gpio bicolor LED
 *
 * This function registers three gpios as a bicolor LED. led->gpio[rg]
 * should be initialized to the gpios to control.
 */
int led_gpio_bicolor_register(struct gpio_bicolor_led *led)
{
	int ret;
	char *name = led->led.name;

	ret = gpio_request(led->gpio_c0, name ? name : "led_c0");
	if (ret)
		return ret;

	ret = gpio_request(led->gpio_c1, name ? name : "led_c1");
	if (ret)
		goto err_gpio_c0;

	led->led.set = led_gpio_bicolor_set;
	led->led.max_value = 2;

	ret = led_register(&led->led);
	if (ret)
		goto err_gpio_c1;

	return 0;

err_gpio_c1:
	gpio_free(led->gpio_c1);
err_gpio_c0:
	gpio_free(led->gpio_c0);
	return ret;
}

/**
 * led_gpio_bicolor_unregister - remove a gpio controlled bicolor LED from the framework
 * @param led	The gpio LED
 */
void led_gpio_bicolor_unregister(struct gpio_bicolor_led *led)
{
	led_unregister(&led->led);
}
#endif

#ifdef CONFIG_LED_GPIO_RGB

static void led_gpio_rgb_set(struct led *led, unsigned int value)
{
	struct gpio_rgb_led *rgb = container_of(led, struct gpio_rgb_led, led);
	int al = rgb->active_low;

	gpio_direction_output(rgb->gpio_r, !!(value & 4) ^ al);
	gpio_direction_output(rgb->gpio_g, !!(value & 2) ^ al);
	gpio_direction_output(rgb->gpio_b, !!(value & 1) ^ al);
}

/**
 * led_gpio_rgb_register - register three gpios as a rgb LED
 * @param led	The gpio rg LED
 *
 * This function registers three gpios as a rgb LED. led->gpio[rgb]
 * should be initialized to the gpios to control.
 */
int led_gpio_rgb_register(struct gpio_rgb_led *led)
{
	int ret;
	char *name = led->led.name;

	ret = gpio_request(led->gpio_r, name ? name : "led_r");
	if (ret)
		return ret;

	ret = gpio_request(led->gpio_g, name ? name : "led_g");
	if (ret)
		goto err_gpio_r;

	ret = gpio_request(led->gpio_b, name ? name : "led_b");
	if (ret)
		goto err_gpio_g;

	led->led.set = led_gpio_rgb_set;
	led->led.max_value = 7;

	ret = led_register(&led->led);
	if (ret)
		goto err_gpio_b;

	return 0;

err_gpio_b:
	gpio_free(led->gpio_b);
err_gpio_g:
	gpio_free(led->gpio_g);
err_gpio_r:
	gpio_free(led->gpio_r);
	return ret;
}

/**
 * led_gpio_rgb_unregister - remove a gpio controlled rgb LED from the framework
 * @param led	The gpio LED
 */
void led_gpio_rgb_unregister(struct gpio_led *led)
{
	led_unregister(&led->led);
}
#endif /* CONFIG_LED_GPIO_RGB */

#ifdef CONFIG_LED_GPIO_OF

struct led_trg {
	const char *str;
	enum led_trigger trg;
};

static struct led_trg triggers[] = {
	{ .str = "heartbeat", LED_TRIGGER_HEARTBEAT, },
	{ .str = "panic", LED_TRIGGER_PANIC, },
	{ .str = "net", LED_TRIGGER_NET_TXRX, },
};

static void led_of_parse_trigger(struct led *led, struct device_node *np)
{
	const char *trigger;
	int i;

	trigger = of_get_property(np, "linux,default-trigger", NULL);
	if (!trigger)
		trigger = of_get_property(np, "barebox,default-trigger", NULL);

	if (!trigger)
		return;

	for (i = 0; i < ARRAY_SIZE(triggers); i++) {
		struct led_trg *trg = &triggers[i];
		if (!strcmp(trg->str, trigger)) {
			led_set_trigger(trg->trg, led);
			return;
		}
	}
}

static int led_gpio_of_probe(struct device_d *dev)
{
	struct device_node *child;

	for_each_child_of_node(dev->device_node, child) {
		struct gpio_led *gled;
		enum of_gpio_flags flags;
		int gpio;

		gpio = of_get_named_gpio_flags(child, "gpios", 0, &flags);
		if (gpio < 0)
			continue;

		gled = xzalloc(sizeof(*gled));
		gled->led.name = xstrdup(child->name);
		gled->gpio = gpio;
		gled->active_low = (flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0;

		dev_dbg(dev, "register led %s on gpio%d, active_low = %d\n",
			gled->led.name, gled->gpio, gled->active_low);

		led_of_parse_trigger(&gled->led, child);
		led_gpio_register(gled);
	}

	return 0;
}

static struct of_device_id led_gpio_of_ids[] = {
	{ .compatible = "gpio-leds", },
	{ }
};

static struct driver_d led_gpio_of_driver = {
	.name  = "gpio-leds",
	.probe = led_gpio_of_probe,
	.of_compatible = DRV_OF_COMPAT(led_gpio_of_ids),
};
device_platform_driver(led_gpio_of_driver);

#endif /* CONFIG LED_GPIO_OF */
