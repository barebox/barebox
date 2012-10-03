/*
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

/**
 * @file
 * @brief setenv: Set an environment variables
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <environment.h>

static int do_setenv(int argc, char *argv[])
{
	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	setenv(argv[1], argv[2]);

	return 0;
}

BAREBOX_CMD_HELP_START(setenv)
BAREBOX_CMD_HELP_USAGE("setenv <name> [<value>]\n")
BAREBOX_CMD_HELP_SHORT("Set environment variable to a value or delete if value is avoided.\n")
BAREBOX_CMD_HELP_END

/**
 * @page setenv_command

<p> This command is only available if the simple command line parser is
in use. Within the hush shell, \c setenv is not required.</p>

\todo Check if kconfig does this correctly.

 */

BAREBOX_CMD_START(setenv)
	.cmd		= do_setenv,
	.usage		= "set environment variables",
	BAREBOX_CMD_HELP(cmd_setenv_help)
BAREBOX_CMD_END
