/*
 * clear.c - Clear the screen
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
#include <readkey.h>

static int do_clear(int argc, char *argv[])
{
	printf(ANSI_CLEAR_SCREEN);

	return 0;
}

BAREBOX_CMD_HELP_START(clear)
BAREBOX_CMD_HELP_USAGE("clear\n")
BAREBOX_CMD_HELP_SHORT("Clear the screen.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clear)
	.cmd		= do_clear,
	.usage		= "clear screen",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
