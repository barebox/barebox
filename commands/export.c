// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/**
 * @file
 * @brief export: Export an environment variable
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <environment.h>

static int do_export(int argc, char *argv[])
{
	int i = 1;
	char *ptr;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while (i < argc) {
		if ((ptr = parse_assignment(argv[i])))
			setenv(argv[i], ptr);
		if (export(argv[i])) {
			printf("could not export: %s\n", argv[i]);
			return 1;
		}
		i++;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(export)
BAREBOX_CMD_HELP_TEXT("Export an environment variable to subsequently executed scripts.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(export)
	.cmd		= do_export,
	BAREBOX_CMD_DESC("export environment variables")
	BAREBOX_CMD_OPTS("VAR[=VALUE]")
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
	BAREBOX_CMD_HELP(cmd_export_help)
BAREBOX_CMD_END
