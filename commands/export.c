/*
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

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
		if ((ptr = strchr(argv[i], '='))) {
			*ptr++ = 0;
			setenv(argv[i], ptr);
		}
		if (export(argv[i])) {
			printf("could not export: %s\n", argv[i]);
			return 1;
		}
		i++;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(export)
BAREBOX_CMD_HELP_USAGE("export <var>[=value]\n")
BAREBOX_CMD_HELP_SHORT("export an environment variable to subsequently executed scripts\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(export)
	.cmd		= do_export,
	.usage		= "export environment variables",
	BAREBOX_CMD_HELP(cmd_export_help)
BAREBOX_CMD_END

