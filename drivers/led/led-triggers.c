/*
 * LED trigger support for barebox
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
#include <poller.h>
#include <errno.h>
#include <clock.h>
#include <led.h>
#include <init.h>

/**
 * @file
 * @brief LED trigger framework
 *
 * This file contains triggers which can be associated to LEDs.
 *
 * With this framework LEDs can be associated to different events.
 * An event can be a heartbeat, network activity or panic.
 * led_trigger() is the central function which is called in the
 * different barebox frameworks to trigger an event.
 *
 * currently there are the following triggers are defined:
 *
 * led_trigger_panic:		triggered in panic()
 * led_trigger_heartbeat:	shows the heartbeat of barebox. Blinks as long
 *				barebox is up and running.
 * led_trigger_net_rx:		Triggered during network packet reception
 * led_trigger_net_tx:		Triggered during network packet transmission
 * led_trigger_net_txrx:	combination of the two above
 */

struct led_trigger_struct {
	struct led *led;
	struct list_head list;
	enum led_trigger trigger;
};

static struct led_trigger_struct triggers[LED_TRIGGER_MAX];

/**
 * led_trigger - triggers a trigger
 * @param trigger	The trigger to enable/disable
 * @param enable	true if enable
 *
 * Enable/disable a LED for a given trigger.
 */
void led_trigger(enum led_trigger trigger, enum trigger_type type)
{
	if (trigger >= LED_TRIGGER_MAX)
		return;
	if (!triggers[trigger].led)
		return;

	if (type == TRIGGER_FLASH) {
		led_flash(triggers[trigger].led, 200);
		return;
	}

	led_set(triggers[trigger].led, type == TRIGGER_ENABLE ? triggers[trigger].led->max_value : 0);
}

/**
 * led_set_trigger - set the LED for a trigger
 * @param trigger	The trigger to set a LED for
 * @param led		The LED
 *
 * This function associates a trigger with a LED. Pass led = NULL
 * to disable a trigger
 */
int led_set_trigger(enum led_trigger trigger, struct led *led)
{
	int i;

	if (trigger >= LED_TRIGGER_MAX)
		return -EINVAL;

	if (led)
		for (i = 0; i < LED_TRIGGER_MAX; i++)
			if (triggers[i].led == led)
				return -EBUSY;

	if (triggers[trigger].led && !led)
		led_set(triggers[trigger].led, 0);

	triggers[trigger].led = led;

	if (led && trigger == LED_TRIGGER_DEFAULT_ON)
		led_set(triggers[trigger].led, triggers[trigger].led->max_value);
	if (led && trigger == LED_TRIGGER_HEARTBEAT)
		led_blink(led, 200, 1000);

	return 0;
}

/**
 * led_get_trigger - get the LED for a trigger
 * @param trigger	The trigger to set a LED for
 *
 * return the LED number of a trigger.
 */
int led_get_trigger(enum led_trigger trigger)
{
	if (trigger >= LED_TRIGGER_MAX)
		return -EINVAL;
	if (!triggers[trigger].led)
		return -ENODEV;
	return led_get_number(triggers[trigger].led);
}
