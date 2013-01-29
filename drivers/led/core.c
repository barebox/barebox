/*
 * core LED support for barebox
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
#include <command.h>
#include <linux/list.h>
#include <errno.h>
#include <led.h>
#include <poller.h>
#include <clock.h>
#include <linux/ctype.h>

/**
 * @file
 * @brief LED framework
 *
 * This file contains the core LED framework for barebox.
 *
 * Each LED can be set to a value where 0 means disabled and values
 * > 0 mean enabled. LEDs can have different enable values where the
 * exact meaning depends on the LED, for example a gpio controlled rgb
 * LED can have enable values from 1 to 7 which correspond to different
 * colors. value could also mean a brightness.
 * Each LED is assigned a number. numbers start with 0 and are increased
 * with each registered LED. The number stays the same during lifecycle,
 * gaps because of unregistered LEDs are not filled up.
 */

static LIST_HEAD(leds);
static int num_leds;

/**
 * led_by_number - get the number of a LED
 * @param num number of the LED to return
 */
struct led *led_by_number(int num)
{
	struct led *led;

	list_for_each_entry(led, &leds, list) {
		if (led->num == num)
			return led;
	}

	return NULL;
}

/**
 * led_by_name - get a LED with its name
 * @param name	name of the LED
 */
struct led *led_by_name(const char *name)
{
	struct led *led;

	list_for_each_entry(led, &leds, list) {
		if (led->name && !strcmp(led->name, name))
			return led;
	}

	return NULL;
}

/**
 * led_by_name_or_number - get a LED with its name or number
 * @param str	if first character of str is a digit led_by_number
 *		is returned, led_by_name otherwise.
 */
struct led *led_by_name_or_number(const char *str)
{
	if (isdigit(*str)) {
		int l;

		l = simple_strtoul(str, NULL, 0);

		return led_by_number(l);
	} else {
		return led_by_name(str);
	}
}

/**
 * led_set - set the value of a LED
 * @param led	the led
 * @param value	the value of the LED (0 is disabled)
 */
int led_set(struct led *led, unsigned int value)
{
	if (value > led->max_value)
		value = led->max_value;

	if (!led->set)
		return -ENODEV;

	led->set(led, value);

	return 0;
}

/**
 * led_set_num - set the value of a LED
 * @param num	the number of the LED
 * @param value	the value of the LED (0 is disabled)
 */
int led_set_num(int num, unsigned int value)
{
	struct led *led = led_by_number(num);

	if (!led)
		return -ENODEV;

	return led_set(led, value);
}

/**
 * led_register - Register a LED
 * @param led	the led
 */
int led_register(struct led *led)
{
	if (led->name && led_by_name(led->name))
		return -EBUSY;

	led->num = num_leds++;

	list_add_tail(&led->list, &leds);

	return 0;
}

/**
 * led_unregister - Unegister a LED
 * @param led	the led
 */
void led_unregister(struct led *led)
{
	list_del(&led->list);
}
