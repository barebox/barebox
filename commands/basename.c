// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* basename.c - strip directory and suffix from filenames */

#include <common.h>
#include <command.h>
#include <libgen.h>
#include <environment.h>

static int do_basename(int argc, char *argv[])
{
	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	setenv(argv[2], posix_basename(argv[1]));

	return 0;
}

BAREBOX_CMD_HELP_START(basename)
BAREBOX_CMD_HELP_TEXT("Remove directory part from the PATH and store result into variable VAR.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(basename)
	.cmd		= do_basename,
	BAREBOX_CMD_DESC("strip directory and suffix from filenames")
	BAREBOX_CMD_OPTS("PATH VAR")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_basename_help)
BAREBOX_CMD_END
