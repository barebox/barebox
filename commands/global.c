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

static int do_global(int argc, char *argv[])
{
	int ret;
	char *value;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	value = strchr(argv[1], '=');
	if (value) {
		*value = 0;
		value++;
	}

	ret = globalvar_add_simple(argv[1]);

	if (value) {
		char *name = asprintf("global.%s", argv[1]);
		ret = setenv(name, value);
		free(name);
	}

	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(global)
BAREBOX_CMD_HELP_USAGE("global <var>[=<value]\n")
BAREBOX_CMD_HELP_SHORT("add a new global variable named <var>, optionally set to <value>\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(global)
	.cmd		= do_global,
	.usage		= "create global variables",
	BAREBOX_CMD_HELP(cmd_global_help)
BAREBOX_CMD_END
