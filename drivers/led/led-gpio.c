// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * gpio LED support for barebox
 *
 * (C) Copyright 2010 Sascha Hauer, Pengutronix
 */
#include <common.h>
#include <init.h>
#include <led.h>
#include <malloc.h>
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
	gpio_free(led->gpio);
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
	gpio_free(led->gpio_c1);
	gpio_free(led->gpio_c0);
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
void led_gpio_rgb_unregister(struct gpio_rgb_led *led)
{
	led_unregister(&led->led);
	gpio_free(led->gpio_b);
	gpio_free(led->gpio_g);
	gpio_free(led->gpio_r);
}
#endif /* CONFIG_LED_GPIO_RGB */

#ifdef CONFIG_LED_GPIO_OF
static int led_gpio_of_probe(struct device *dev)
{
	struct device_node *child;
	struct gpio_led *leds;
	int num_leds;
	int ret = 0, n = 0;

	num_leds = of_get_child_count(dev->of_node);
	if (num_leds <= 0)
		return num_leds;

	leds = xzalloc(num_leds * sizeof(struct gpio_led));

	for_each_child_of_node(dev->of_node, child) {
		struct gpio_led *gled = &leds[n];
		const char *default_state;
		enum of_gpio_flags flags;
		int gpio;
		const char *label;

		gpio = of_get_named_gpio_flags(child, "gpios", 0, &flags);
		if (gpio < 0) {
			if (gpio != -EPROBE_DEFER)
				dev_err(dev, "failed to get gpio for %s: %d\n",
					child->full_name, gpio);
			ret = gpio;
			goto err;
		}

		if (of_property_read_string(child, "label", &label))
			label = child->name;
		gled->led.name = xstrdup(label);

		gled->gpio = gpio;
		gled->active_low = (flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0;

		dev_dbg(dev, "register led %s on gpio%d, active_low = %d\n",
			gled->led.name, gled->gpio, gled->active_low);

		led_gpio_register(gled);
		led_of_parse_trigger(&gled->led, child);

		if (!of_property_read_string(child, "default-state", &default_state)) {
			if (!strcmp(default_state, "on"))
				led_gpio_set(&gled->led, 1);
			else if (!strcmp(default_state, "off"))
				led_gpio_set(&gled->led, 0);
		}

		n++;
	}

	return 0;

err:
	for (n = n - 1; n >= 0; n--)
		led_gpio_unregister(&leds[n]);
	free(leds);
	return ret;
}

static struct of_device_id led_gpio_of_ids[] = {
	{ .compatible = "gpio-leds", },
	{ }
};
MODULE_DEVICE_TABLE(of, led_gpio_of_ids);

static struct driver led_gpio_of_driver = {
	.name  = "gpio-leds",
	.probe = led_gpio_of_probe,
	.of_compatible = DRV_OF_COMPAT(led_gpio_of_ids),
};
device_platform_driver(led_gpio_of_driver);

#endif /* CONFIG LED_GPIO_OF */
