// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* cd.c - change working directory */

/**
 * @file
 * @brief Change working directory
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_cd(int argc, char *argv[])
{
	int ret;

	if (argc == 1)
		ret = chdir("/");
	else
		ret = chdir(argv[1]);

	if (ret) {
		perror("chdir");
		return 1;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(cd)
BAREBOX_CMD_HELP_TEXT("If called without an argument, change to the root directory '/'.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(cd)
	.cmd		= do_cd,
	BAREBOX_CMD_DESC("change working directory")
	BAREBOX_CMD_OPTS("DIRECTORY")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_cd_help)
BAREBOX_CMD_END
