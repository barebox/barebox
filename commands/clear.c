// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* clear.c - Clear the screen */

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
BAREBOX_CMD_HELP_TEXT("Send ANSI ESC sequence to clear the screen.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clear)
	.cmd		= do_clear,
	BAREBOX_CMD_DESC("clear screen")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_clear_help)
BAREBOX_CMD_END
