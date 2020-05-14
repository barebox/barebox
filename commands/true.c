// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2000-2003 Wolfgang Denk <wd@denx.de>, DENX Software Engineering

#include <common.h>
#include <command.h>
#include <complete.h>

static int do_true(int argc, char *argv[])
{
	return 0;
}

static const char * const true_aliases[] = { ":", NULL};

BAREBOX_CMD_START(true)
	.aliases	= true_aliases,
	.cmd		= do_true,
	BAREBOX_CMD_DESC("do nothing, successfully")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
