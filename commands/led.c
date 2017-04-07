/*
 * LED command support for barebox
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

static int do_led(int argc, char *argv[])
{
	unsigned long value;
	struct led *led;
	int ret;
	int opt;
	int flash = 0, blink = 0;
	int blink_on_ms = 500;
	int blink_off_ms = 500;

	while ((opt = getopt(argc, argv, "fb")) > 0) {
		switch(opt) {
		case 'f':
			flash = 1;
			break;
		case 'b':
			blink = 1;
			break;
		}
	}

	if (flash || blink) {
		int args = argc - optind;

		if (!args || (flash && blink))
			return COMMAND_ERROR_USAGE;

		led = led_by_name_or_number(argv[optind]);
		if (!led) {
			printf("no such LED: %s\n", argv[optind]);
			return 1;
		}

		if (args > 1)
			blink_on_ms = simple_strtoul(argv[optind + 1], NULL, 0);
		if (args > 2)
			blink_off_ms = simple_strtoul(argv[optind + 2], NULL, 0);

		if (flash)
			return led_flash(led, blink_on_ms);
		if (blink)
			return led_blink(led, blink_on_ms, blink_off_ms);
	}

	if (argc == 1) {
		int i = 0;

		led = led_by_number(i);
		if (!led) {
			printf("no registered LEDs\n");
			return COMMAND_ERROR;
		}

		printf("registered LEDs:\n");

		while ((led = led_by_number(i))) {
			printf("%-2d: name: %-10s max_value: %d\n",
					i, led->name ? led->name : "none",
					led->max_value);
			i++;
		}
		return 0;
	}

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	led = led_by_name_or_number(argv[1]);
	if (!led) {
		printf("no such LED: %s\n", argv[1]);
		return 1;
	}

	value = simple_strtoul(argv[optind + 1], NULL, 0);

	ret = led_set(led, value);
	if (ret < 0) {
		perror("led");
		return 1;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(led)
BAREBOX_CMD_HELP_TEXT("Control the value of a LED. The exact meaning of VALUE is unspecified,")
BAREBOX_CMD_HELP_TEXT("it can be a brightness, or a color. Most often a value of '1' means on")
BAREBOX_CMD_HELP_TEXT("and '0' means off. Basic usage is 'led <led> <value>'. LEDs can be given")
BAREBOX_CMD_HELP_TEXT("by name or number.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Without arguments the available LEDs are listed.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b <led> [duration-on-ms] [duration-off-ms]",  "blink a LED")
BAREBOX_CMD_HELP_OPT ("-f <led> [duration-ms]",  "flash a LED")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(led)
	.cmd		= do_led,
	BAREBOX_CMD_DESC("control LEDs")
	BAREBOX_CMD_OPTS("LED VALUE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_led_help)
BAREBOX_CMD_END
