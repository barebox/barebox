// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* sleep.c - delay execution */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <clock.h>
#include <getopt.h>

static int do_sleep(int argc, char *argv[])
{
	uint64_t start, delay;
	bool blocking = false;
	int opt;

	while((opt = getopt(argc, argv, "b")) > 0) {
		switch(opt) {
		case 'b':
			blocking = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc - optind != 1)
		return COMMAND_ERROR_USAGE;

	delay = simple_strtoul(argv[optind], NULL, 10);

	if (!strcmp(argv[0], "msleep"))
		delay *= MSECOND;
	else
		delay *= SECOND;

	start = get_time_ns();

	if (blocking) {
		while (!is_timeout_non_interruptible(start, delay)) {
			if (ctrlc_non_interruptible())
				return 1;
		}
	} else {
		while (!is_timeout(start, delay)) {
			if (ctrlc())
				return 1;
		}
	}

	return 0;
}

static const char * const sleep_aliases[] = { "msleep", NULL};

BAREBOX_CMD_HELP_START(sleep)
BAREBOX_CMD_HELP_TEXT("delay execution for n seconds or n mseconds if invoked as msleep")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b",  "For development: busy loop while inhibiting pollers from running")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sleep)
	.cmd		= do_sleep,
	.aliases	= sleep_aliases,
	BAREBOX_CMD_DESC("delay execution for specified time")
	BAREBOX_CMD_OPTS("[-b] SECONDS")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_COMPLETE(command_var_complete)
BAREBOX_CMD_END
