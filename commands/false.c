// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2000-2003 Wolfgang Denk <wd@denx.de>, DENX Software Engineering

#include <common.h>
#include <command.h>
#include <complete.h>

static int do_false(int argc, char *argv[])
{
	return 1;
}

BAREBOX_CMD_START(false)
	.cmd		= do_false,
	BAREBOX_CMD_DESC("do nothing, unsuccessfully")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
