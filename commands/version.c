// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © Wolfgang Denk <wd@denx.de>, DENX Software Engineering

#include <common.h>
#include <command.h>
#include <complete.h>
#include <generated/compile.h>

static int do_version(int argc, char *argv[])
{
	printf ("\n%s", version_string);
	if (*CONFIG_NAME)
		printf (" (%s)", CONFIG_NAME);
	printf ("\nCompiled by: %s\n\n", BAREBOX_COMPILER);
	return 0;
}

BAREBOX_CMD_START(version)
	.cmd		= do_version,
	BAREBOX_CMD_DESC("print barebox version")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
