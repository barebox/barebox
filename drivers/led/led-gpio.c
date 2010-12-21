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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <led.h>
#include <asm/gpio.h>

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
	led->led.set = led_gpio_set;
	led->led.max_value = 1;

	return led_register(&led->led);
}

/**
 * led_gpio_unregister - remove a gpio controlled LED from the framework
 * @param led	The gpio LED
 */
void led_gpio_unregister(struct gpio_led *led)
{
	led_unregister(&led->led);
}

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
	led->led.set = led_gpio_rgb_set;
	led->led.max_value = 7;

	return led_register(&led->led);
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
