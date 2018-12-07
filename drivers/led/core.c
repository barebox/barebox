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
#include <init.h>
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
static int __led_set(struct led *led, unsigned int value)
{
	if (value > led->max_value)
		value = led->max_value;

	if (!led->set)
		return -ENODEV;

	led->set(led, value);

	return 0;
}

int led_set(struct led *led, unsigned int value)
{
	led->blink = 0;
	led->flash = 0;
	return __led_set(led, value);
}

static void led_blink_func(struct poller_struct *poller)
{
	struct led *led;

	list_for_each_entry(led, &leds, list) {
		const uint64_t now = get_time_ns();
		int on;

		if (!led->blink && !led->flash)
			continue;

		if (led->blink_next_event > now) {
			continue;
		}

		on = !(led->blink_next_state % 2);
		if (on)
			on = led->max_value;

		led->blink_next_event = now +
			(led->blink_states[led->blink_next_state] * MSECOND);
		led->blink_next_state = (led->blink_next_state + 1) %
					led->blink_nr_states;

		if (led->flash && !on)
			led->flash = 0;

		__led_set(led, on);
	}
}

/**
 * led_blink_pattern - Blink a led with flexible timings.
 * @led LED used
 * @pattern Array of millisecond intervals describing the on and off periods of
 * the pattern. At the end of the array/pattern it is repeated. The array
 * starts with an on-period. In general every array item with even index
 * describes an on-period, every item with odd index an off-period.
 * @pattern_len Length of the pattern array.
 *
 * Returns 0 on success.
 *
 * Example:
 * 	pattern = {500, 1000};
 * 	This will enable the LED for 500ms and disable it for 1000ms after
 * 	that. This is repeated forever.
 */
int led_blink_pattern(struct led *led, const unsigned int *pattern,
		      unsigned int pattern_len)
{
	free(led->blink_states);
	led->blink_states = xmemdup(pattern,
				    pattern_len * sizeof(*led->blink_states));
	led->blink_nr_states = pattern_len;
	led->blink_next_state = 0;
	led->blink_next_event = 0;
	led->blink = 1;
	led->flash = 0;

	return 0;
}

int led_blink(struct led *led, unsigned int on_ms, unsigned int off_ms)
{
	unsigned int pattern[] = {on_ms, off_ms};

	return led_blink_pattern(led, ARRAY_AND_SIZE(pattern));
}

int led_flash(struct led *led, unsigned int duration_ms)
{
	unsigned int pattern[] = {duration_ms, 0};
	int ret;

	ret = led_blink_pattern(led, ARRAY_AND_SIZE(pattern));
	if (ret)
		return ret;

	led->flash = 1;
	led->blink = 0;

	return 0;
}

static struct poller_struct led_poller = {
	.func = led_blink_func,
};

static int led_blink_init(void)
{
	return poller_register(&led_poller);
}
late_initcall(led_blink_init);

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

struct led_trg {
	const char *str;
	enum led_trigger trg;
};

static struct led_trg triggers[] = {
	{ .str = "heartbeat", LED_TRIGGER_HEARTBEAT, },
	{ .str = "panic", LED_TRIGGER_PANIC, },
	{ .str = "net", LED_TRIGGER_NET_TXRX, },
	{ .str = "default-on", LED_TRIGGER_DEFAULT_ON, },
};

void led_of_parse_trigger(struct led *led, struct device_node *np)
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
			/* disable LED before installing trigger */
			led_set(led, 0);
			led_set_trigger(trg->trg, led);
			return;
		}
	}
}
