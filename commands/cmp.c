// SPDX-License-Identifier: GPL-2.0-or-later

/* cmp - determine if two files differ */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <libfile.h>

static int do_cmp(int argc, char *argv[])
{
	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	return compare_file(argv[1], argv[2]);
}

BAREBOX_CMD_HELP_START(cmp)
BAREBOX_CMD_HELP_TEXT("Returns successfully if the two files are the same, return with an error if not")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(cmp)
	.cmd		= do_cmp,
	BAREBOX_CMD_DESC("compare two files")
	BAREBOX_CMD_OPTS("FILE1 FILE2")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_cmp_help)
BAREBOX_CMD_END
