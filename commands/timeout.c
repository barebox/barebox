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
#include <common.h>
#include <command.h>
#include <linux/stat.h>
#include <errno.h>
#include <getopt.h>
#include <clock.h>
#include <environment.h>

#define TIMEOUT_RETURN	(1 << 0)
#define TIMEOUT_CTRLC	(1 << 1)
#define TIMEOUT_ANYKEY	(1 << 2)
#define TIMEOUT_SILENT	(1 << 3)

static int do_timeout(int argc, char *argv[])
{
	int timeout = 3, ret = 1;
	int flags = 0, opt, countdown;
	int key = 0;
	uint64_t start, second;
	const char *varname = NULL;

	while((opt = getopt(argc, argv, "t:crsav:")) > 0) {
		switch(opt) {
		case 'r':
			flags |= TIMEOUT_RETURN;
			break;
		case 'c':
			flags |= TIMEOUT_CTRLC;
			break;
		case 'a':
			flags |= TIMEOUT_ANYKEY;
			break;
		case 's':
			flags |= TIMEOUT_SILENT;
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

	start = get_time_ns();
	second = start;

	countdown = timeout;

	if (!(flags & TIMEOUT_SILENT))
		printf("%2d", countdown--);

	do {
		if (tstc()) {
			key = getc();
			if (flags & TIMEOUT_CTRLC && key == 3)
				goto  out;
			if (flags & TIMEOUT_ANYKEY)
				goto out;
			if (flags & TIMEOUT_RETURN && key == '\n')
				goto out;
			key = 0;
		}
		if (!(flags & TIMEOUT_SILENT) && is_timeout(second, SECOND)) {
			printf("\b\b%2d", countdown--);
			second += SECOND;
		}
	} while (!is_timeout(start, timeout * SECOND));

	ret = 0;
out:
	if (varname && key) {
		char str[2] = { };
		str[0] = key;
		setenv(varname, str);
	}
	if (!(flags & TIMEOUT_SILENT))
		printf("\n");

	return ret;
}

static const __maybe_unused char cmd_timeout_help[] =
"Usage: timeout [OPTION]... <timeout>\n"
"Wait <timeout> seconds for a timeout. Return 1 if the user intervented\n"
"or 0 if a timeout occured\n"
"  -a  interrupt on any key\n"
"  -c  interrupt on ctrl-c\n"
"  -r  interrupt on return\n"
"  -s  silent mode\n";

BAREBOX_CMD_START(timeout)
	.cmd		= do_timeout,
	.usage		= "wait for a specified timeout",
	BAREBOX_CMD_HELP(cmd_timeout_help)
BAREBOX_CMD_END

