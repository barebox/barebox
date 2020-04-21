// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* pwd.c - print working directory */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <fs.h>

static int do_pwd(int argc, char *argv[])
{
	printf("%s\n", getcwd());
	return 0;
}

BAREBOX_CMD_START(pwd)
	.cmd		= do_pwd,
	BAREBOX_CMD_DESC("print working directory")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
