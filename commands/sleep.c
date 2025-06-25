// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* sleep.c - delay execution */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <clock.h>

static int do_sleep(int argc, char *argv[])
{
	uint64_t start, delay;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	delay = simple_strtoul(argv[1], NULL, 10);

	if (!strcmp(argv[0], "msleep"))
		delay *= MSECOND;
	else
		delay *= SECOND;

	start = get_time_ns();
	while (!is_timeout(start, delay)) {
		if (ctrlc())
			return 1;
	}

	return 0;
}

static const char * const sleep_aliases[] = { "msleep", NULL};

BAREBOX_CMD_START(sleep)
	.cmd		= do_sleep,
	.aliases	= sleep_aliases,
	BAREBOX_CMD_DESC("delay execution for n seconds or n mseconds if invoked as msleep")
	BAREBOX_CMD_OPTS("SECONDS")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_COMPLETE(command_var_complete)
BAREBOX_CMD_END
