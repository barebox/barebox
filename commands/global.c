/*
 * global.c - global shell variables
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <malloc.h>
#include <command.h>
#include <globalvar.h>
#include <environment.h>
#include <getopt.h>

static int do_global(int argc, char *argv[])
{
	int opt;
	int do_set_match = 0;
	char *value;

	while ((opt = getopt(argc, argv, "r")) > 0) {
		switch (opt) {
		case 'r':
			do_set_match = 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	value = strchr(argv[0], '=');
	if (value) {
		*value = 0;
		value++;
	}

	if (do_set_match) {
		if (!value)
			value = "";

		globalvar_set_match(argv[0], value);
		return 0;
	}

	return globalvar_add_simple(argv[0], value);
}

BAREBOX_CMD_HELP_START(global)
BAREBOX_CMD_HELP_USAGE("global [-r] <var>[=<value]\n")
BAREBOX_CMD_HELP_SHORT("add a new global variable named <var>, optionally set to <value>\n")
BAREBOX_CMD_HELP_SHORT("-r to set a value to of all globalvars beginning with 'match'")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(global)
	.cmd		= do_global,
	.usage		= "create or set global variables",
	BAREBOX_CMD_HELP(cmd_global_help)
BAREBOX_CMD_END
