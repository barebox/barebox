/*
 * timeout - wait for timeout
 *
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

#include <command.h>
#include <errno.h>
#include <getopt.h>
#include <environment.h>
#include <console_countdown.h>

#include <linux/kernel.h>

static int do_timeout(int argc, char *argv[])
{
	int timeout, ret, opt;
	unsigned flags = 0;
	char str[2] = { };
	const char *varname = NULL;

	while ((opt = getopt(argc, argv, "crsav:e")) > 0) {
		switch(opt) {
		case 'r':
			flags |= CONSOLE_COUNTDOWN_RETURN;
			break;
		case 'c':
			flags |= CONSOLE_COUNTDOWN_CTRLC;
			break;
		case 'a':
			flags |= CONSOLE_COUNTDOWN_ANYKEY;
			break;
		case 's':
			flags |= CONSOLE_COUNTDOWN_SILENT;
			break;
		case 'e':
			flags |= CONSOLE_COUNTDOWN_EXTERN;
			break;
		case 'v':
			varname = optarg;
			break;
		default:
			return 1;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	timeout = simple_strtoul(argv[optind], NULL, 0);
	ret = console_countdown(timeout, flags, str);

	if (varname && str[0])
		setenv(varname, str);

	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(timeout)
BAREBOX_CMD_HELP_TEXT("Wait SECONDS for a timeout. Return 1 if the user intervented.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-a", "interrupt on any key")
BAREBOX_CMD_HELP_OPT("-c", "interrupt on Ctrl-C")
BAREBOX_CMD_HELP_OPT("-r", "interrupt on RETURN")
BAREBOX_CMD_HELP_OPT("-e", "interrupt on external commands (i.e. fastboot")
BAREBOX_CMD_HELP_OPT("-s", "silent mode")
BAREBOX_CMD_HELP_OPT("-v <VARIABLE>", "export pressed key to environment")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(timeout)
	.cmd		= do_timeout,
	BAREBOX_CMD_DESC("wait for a specified timeout")
	BAREBOX_CMD_OPTS("[-acrsev] SECONDS")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_timeout_help)
BAREBOX_CMD_END
