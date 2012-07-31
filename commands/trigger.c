/*
 * LED trigger command support for barebox
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
#include <led.h>
#include <command.h>
#include <getopt.h>
#include <errno.h>

#define LED_COMMAND_SET_TRIGGER		1
#define LED_COMMAND_SHOW_INFO		2
#define	LED_COMMAND_DISABLE_TRIGGER	3

static char *trigger_names[] = {
	[LED_TRIGGER_PANIC] = "panic",
	[LED_TRIGGER_HEARTBEAT] = "heartbeat",
	[LED_TRIGGER_NET_RX] = "net rx",
	[LED_TRIGGER_NET_TX] = "net tx",
	[LED_TRIGGER_NET_TXRX] = "net",
};

static int do_trigger(int argc, char *argv[])
{
	struct led *led;
	int i, opt, ret = 0;
	int cmd = LED_COMMAND_SHOW_INFO;
	unsigned long trigger = 0;

	while((opt = getopt(argc, argv, "t:d:")) > 0) {
		switch(opt) {
		case 't':
			trigger = simple_strtoul(optarg, NULL, 0);
			cmd = LED_COMMAND_SET_TRIGGER;
			break;
		case 'd':
			trigger = simple_strtoul(optarg, NULL, 0);
			cmd = LED_COMMAND_DISABLE_TRIGGER;
		}
	}

	switch (cmd) {
	case LED_COMMAND_SHOW_INFO:
		for (i = 0; i < LED_TRIGGER_MAX; i++) {
			int led = led_get_trigger(i);
			printf("%d: %s", i, trigger_names[i]);
			if (led >= 0)
				printf(" (led %d)", led);
			printf("\n");
		}
		break;

	case LED_COMMAND_DISABLE_TRIGGER:
		led_set_trigger(trigger, NULL);
		return 0;
	case LED_COMMAND_SET_TRIGGER:
		if (argc - optind != 1)
			return COMMAND_ERROR_USAGE;
		led = led_by_name_or_number(argv[optind]);

		if (!led) {
			printf("no such led: %s\n", argv[optind]);
			return 1;
		}

		ret = led_set_trigger(trigger, led);
		break;
	}

	if (ret)
		printf("trigger failed: %s\n", strerror(-ret));
	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(trigger)
BAREBOX_CMD_HELP_USAGE("trigger [OPTIONS]\n")
BAREBOX_CMD_HELP_SHORT("control a LED trigger. Without options the currently assigned triggers are shown.\n")
BAREBOX_CMD_HELP_OPT  ("-t <trigger> <led>",  "set a trigger for a led\n")
BAREBOX_CMD_HELP_OPT  ("-d <trigger>",  "disable a trigger\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(trigger)
	.cmd		= do_trigger,
	.usage		= "handle LED triggers",
	BAREBOX_CMD_HELP(cmd_trigger_help)
BAREBOX_CMD_END

