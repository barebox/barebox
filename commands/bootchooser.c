/*
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
 * Copyright (C) 2015 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <bootchooser.h>
#include <globalvar.h>
#include <command.h>
#include <common.h>
#include <getopt.h>
#include <malloc.h>
#include <stdio.h>

#define DONTTOUCH	-2
#define DEFAULT		-1

static void target_reset(struct bootchooser_target *target, int priority, int attempts)
{
	printf("Resetting target %s to ", bootchooser_target_name(target));

	if (priority >= 0)
		printf("priority %d", priority);
	else if (priority == DEFAULT)
		printf("default priority");

	if (priority > DONTTOUCH && attempts > DONTTOUCH)
		printf(", ");

	if (attempts >= 0)
		printf("%d attempts", attempts);
	else if (attempts == DEFAULT)
		printf("default attempts");

	printf("\n");

	if (priority > DONTTOUCH)
		bootchooser_target_set_priority(target, priority);
	if (attempts > DONTTOUCH)
		bootchooser_target_set_attempts(target, attempts);
}

static int do_bootchooser(int argc, char *argv[])
{
	int opt, ret = 0, i;
	struct bootchooser *bootchooser;
	struct bootchooser_target *target;
	int attempts = DONTTOUCH;
	int priority = DONTTOUCH;
	int info = 0;
	bool done_something = false;
	bool last_boot_successful = false;

	while ((opt = getopt(argc, argv, "a:p:is")) > 0) {
		switch (opt) {
		case 'a':
			if (!strcmp(optarg, "default"))
				attempts = DEFAULT;
			else
				attempts = simple_strtoul(optarg, NULL, 0);
			break;
		case 'p':
			if (!strcmp(optarg, "default"))
				priority = DEFAULT;
			else
				priority = simple_strtoul(optarg, NULL, 0);
			break;
		case 'i':
			info = 1;
			break;
		case 's':
			last_boot_successful = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	bootchooser = bootchooser_get();
	if (IS_ERR(bootchooser)) {
		printf("No bootchooser found\n");
		return COMMAND_ERROR;
	}

	if (last_boot_successful) {
		bootchooser_last_boot_successful();
		done_something = true;
	}

	if (attempts != DONTTOUCH || priority != DONTTOUCH) {
		if (optind < argc) {
			for (i = optind; i < argc; i++) {
				target = bootchooser_target_by_name(bootchooser, argv[i]);
				if (!target) {
					printf("No such target: %s\n", argv[i]);
					ret = COMMAND_ERROR;
					goto out;
				}

				target_reset(target, priority, attempts);
			}
		} else {
			bootchooser_for_each_target(bootchooser, target)
				target_reset(target, priority, attempts);
		}
		done_something = true;
	}

	if (info) {
		bootchooser_info(bootchooser);
		done_something = true;
	}

	if (!done_something) {
		printf("Nothing to do\n");
		ret = COMMAND_ERROR_USAGE;
	}
out:
	bootchooser_put(bootchooser);

	return ret;
}

BAREBOX_CMD_HELP_START(bootchooser)
BAREBOX_CMD_HELP_TEXT("Control misc behaviour of the bootchooser")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a <n|default> [TARGETS]",  "set priority of given targets to 'n' or the default priority")
BAREBOX_CMD_HELP_OPT ("-p <n|default> [TARGETS]",  "set remaining attempts of given targets to 'n' or the default attempts")
BAREBOX_CMD_HELP_OPT ("-i",  "Show information about the bootchooser")
BAREBOX_CMD_HELP_OPT ("-s",  "Mark the last boot successful")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootchooser)
	.cmd = do_bootchooser,
	BAREBOX_CMD_DESC("bootchooser control")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_bootchooser_help)
BAREBOX_CMD_END
