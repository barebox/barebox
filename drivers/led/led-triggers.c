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
	uint64_t flash_start;
};

static struct led_trigger_struct triggers[LED_TRIGGER_MAX];

static void trigger_func(struct poller_struct *poller)
{
	int i;

	for (i = 0; i < LED_TRIGGER_MAX; i++) {
		if (triggers[i].led &&
		    triggers[i].flash_start &&
		    is_timeout(triggers[i].flash_start, 200 * MSECOND)) {
			led_set(triggers[i].led, 0);
		}
	}

	if (triggers[LED_TRIGGER_HEARTBEAT].led &&
			is_timeout(triggers[LED_TRIGGER_HEARTBEAT].flash_start, SECOND))
		led_trigger(LED_TRIGGER_HEARTBEAT, TRIGGER_FLASH);
}

static struct poller_struct trigger_poller = {
	.func = trigger_func,
};

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
		if (is_timeout(triggers[trigger].flash_start, 400 * MSECOND)) {
			led_set(triggers[trigger].led, 1);
			triggers[trigger].flash_start = get_time_ns();
		}
		return;
	}

	led_set(triggers[trigger].led, type == TRIGGER_ENABLE ? 1 : 0);
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

int trigger_init(void)
{
	return poller_register(&trigger_poller);
}
late_initcall(trigger_init);
