// SPDX-License-Identifier: GPL-2.0-only

#include <stdio.h>
#include <command.h>
#include <complete.h>

static int do_resize(int argc, char *argv[])
{
	/* save cursor, reset DECSTBM scroll region, restore cursor */
	printf("\e7" "\e[r" "\e8");

	return 0;
}

BAREBOX_CMD_HELP_START(resize)
BAREBOX_CMD_HELP_TEXT("Reset the terminal scroll region to the full window")
BAREBOX_CMD_HELP_TEXT("and clear the screen.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(resize)
	.cmd		= do_resize,
	BAREBOX_CMD_DESC("reset scroll region and clear screen")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_resize_help)
BAREBOX_CMD_END
