// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2014 Sascha Hauer, Pengutronix

#include <common.h>
#include <command.h>
#include <complete.h>
#include <environment.h>
#include <image-metadata.h>

int imd_command_setenv(const char *variable_name, const char *value)
{
	return setenv(variable_name, value);
}

static int do_imd(int argc, char *argv[])
{
	int ret;

	ret = imd_command(argc, argv);

	if (ret == -ENOSYS)
		return COMMAND_ERROR_USAGE;

	return ret;
}

BAREBOX_CMD_HELP_START(imd)
BAREBOX_CMD_HELP_TEXT("extract metadata from barebox binary")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-t <type>", "only show information of <type>")
BAREBOX_CMD_HELP_OPT ("-n <no>", "for tags with multiple strings only show string <no>")
BAREBOX_CMD_HELP_OPT ("-s VARNAME",  "set variable VARNAME instead of showing information")
BAREBOX_CMD_HELP_OPT ("-v", "Be verbose")
BAREBOX_CMD_HELP_OPT ("-V",  "Verify checksum of FILE")
BAREBOX_CMD_HELP_OPT ("-c", "Create checksum for FILE and write it to the crc32 tag.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Without options all information available is printed. Valid types are:")
BAREBOX_CMD_HELP_TEXT("release, build, model, of_compatible")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(imd)
	.cmd = do_imd,
	BAREBOX_CMD_DESC("extract metadata from barebox binary")
	BAREBOX_CMD_OPTS("[nst] FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_imd_help)
BAREBOX_CMD_END
