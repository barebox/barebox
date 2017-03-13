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


static int do_trigger(int argc, char *argv[])
{
	struct led *led = NULL;
	int opt, ret = 0;
	int cmd = LED_COMMAND_SHOW_INFO;
	enum led_trigger trigger;
	const char *led_name = NULL;
	const char *trigger_name = NULL;

	while((opt = getopt(argc, argv, "t:d:")) > 0) {
		switch(opt) {
		case 't':
			trigger_name = optarg;
			cmd = LED_COMMAND_SET_TRIGGER;
			break;
		case 'd':
			led_name = optarg;
			cmd = LED_COMMAND_DISABLE_TRIGGER;
		}
	}

	if (optind < argc)
		led = led_by_name_or_number(argv[optind]);

	switch (cmd) {
	case LED_COMMAND_SHOW_INFO:
		led_triggers_show_info();
		break;

	case LED_COMMAND_DISABLE_TRIGGER:
		led = led_by_name_or_number(led_name);
		if (!led) {
			printf("no such led: %s\n", led_name);
			return 1;
		}

		led_trigger_disable(led);
		break;

	case LED_COMMAND_SET_TRIGGER:
		if (argc - optind != 1)
			return COMMAND_ERROR_USAGE;

		led = led_by_name_or_number(argv[optind]);
		if (!led) {
			printf("no such led: %s\n", argv[optind]);
			return 1;
		}

		trigger = trigger_by_name(trigger_name);
		if (trigger == LED_TRIGGER_INVALID) {
			printf("no such trigger: %s\n", trigger_name);
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
BAREBOX_CMD_HELP_TEXT("Control a LED trigger. Without options assigned triggers are shown.")
BAREBOX_CMD_HELP_TEXT("triggers are given with their name, LEDs are given with their name or number")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-t <trigger> <led>",  "set a trigger")
BAREBOX_CMD_HELP_OPT ("-d <led>",  "disable a trigger")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(trigger)
	.cmd		= do_trigger,
	BAREBOX_CMD_DESC("handle LED triggers")
	BAREBOX_CMD_OPTS("[-td] [LED]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_trigger_help)
BAREBOX_CMD_END
