/*
 * sleep.c - delay execution
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <clock.h>

static int do_sleep(int argc, char *argv[])
{
	uint64_t start;
	ulong delay;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	delay = simple_strtoul(argv[1], NULL, 10);

	start = get_time_ns();
	while (!is_timeout(start, delay * SECOND)) {
		if (ctrlc())
			return 1;
	}

	return 0;
}

BAREBOX_CMD_START(sleep)
	.cmd		= do_sleep,
	.usage		= "delay execution for n seconds",
	BAREBOX_CMD_COMPLETE(command_var_complete)
BAREBOX_CMD_END
