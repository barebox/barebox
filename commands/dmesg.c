/*
 * dmesg.c - barebox logbuffer handling
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <malloc.h>
#include <command.h>
#include <globalvar.h>
#include <environment.h>
#include <getopt.h>
#include <clock.h>

static int do_dmesg(int argc, char *argv[])
{
	int opt, i;
	int delete_buf = 0, emit = 0;
	unsigned flags = 0;

	while ((opt = getopt(argc, argv, "ctde")) > 0) {
		switch (opt) {
		case 'c':
			delete_buf = 1;
			break;
		case 't':
			flags |= BAREBOX_LOG_PRINT_TIME;
			break;
		case 'd':
			flags |= BAREBOX_LOG_DIFF_TIME;
			break;
		case 'e':
			emit = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (emit) {
		char *buf;
		int len = 0;

		for (i = optind; i < argc; i++)
			len += strlen(argv[i]) + 1;

		buf = malloc(len + 2);
		if (!buf)
			return -ENOMEM;

		len = 0;

		for (i = optind; i < argc; i++)
			len += sprintf(buf + len, "%s ", argv[i]);

		*(buf + len) = '\n';
		*(buf + len + 1) = 0;

		pr_info("%s", buf);

		free(buf);

		return 0;
	}

	log_print(flags);

	if (delete_buf)
		log_clean(10);

	return 0;
}

BAREBOX_CMD_HELP_START(dmesg)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c",		"Delete messages after printing them")
BAREBOX_CMD_HELP_OPT ("-d",		"Show a time delta to the last message")
BAREBOX_CMD_HELP_OPT ("-e <msg>",	"Emit a log message")
BAREBOX_CMD_HELP_OPT ("-t",		"Show timestamp informations")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dmesg)
	.cmd	= do_dmesg,
	BAREBOX_CMD_DESC("Print or control log messages")
	BAREBOX_CMD_OPTS("[-cdet]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_dmesg_help)
BAREBOX_CMD_END
