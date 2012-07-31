/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <command.h>
#include <malloc.h>
#include <xfuncs.h>
#include <environment.h>

static int do_readline(int argc, char *argv[])
{
	char *buf = xzalloc(CONFIG_CBSIZE);

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	if (readline(argv[1], buf, CONFIG_CBSIZE) < 0) {
		free(buf);
		return 1;
	}

	setenv(argv[2], buf);
	free(buf);

	return 0;
}

static const __maybe_unused char cmd_readline_help[] =
"Usage: readline <prompt> VAR\n"
"readline reads a line of user input into variable VAR.\n";

BAREBOX_CMD_START(readline)
	.cmd		= do_readline,
	.usage		= "prompt for user input",
	BAREBOX_CMD_HELP(cmd_readline_help)
BAREBOX_CMD_END

