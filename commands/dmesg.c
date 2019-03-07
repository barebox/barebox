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

static unsigned dmesg_get_levels(const char *__args)
{
	char *args = xstrdup(__args);
	char *str, *levels = args;
	unsigned flags = 0;

	while (1) {
		str = strsep(&levels, ",");
		if (!str)
			break;

		if(!strcmp(str, "vdebug"))
			flags |= BAREBOX_LOG_PRINT_VDEBUG;
		else if(!strcmp(str, "debug"))
			flags |= BAREBOX_LOG_PRINT_DEBUG;
		else if(!strcmp(str, "info"))
			flags |= BAREBOX_LOG_PRINT_INFO;
		else if(!strcmp(str, "notice"))
			flags |= BAREBOX_LOG_PRINT_NOTICE;
		else if(!strcmp(str, "warn"))
			flags |= BAREBOX_LOG_PRINT_WARNING;
		else if(!strcmp(str, "err"))
			flags |= BAREBOX_LOG_PRINT_ERR;
		else if(!strcmp(str, "crit"))
			flags |= BAREBOX_LOG_PRINT_CRIT;
		else if(!strcmp(str, "alert"))
			flags |= BAREBOX_LOG_PRINT_ALERT;
		else if(!strcmp(str, "emerg"))
			flags |= BAREBOX_LOG_PRINT_EMERG;
	}

	free(args);

	return flags;
}

static int do_dmesg(int argc, char *argv[])
{
	int opt, i;
	int delete_buf = 0, emit = 0;
	unsigned flags = 0, levels = 0;

	while ((opt = getopt(argc, argv, "ctderl:")) > 0) {
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
		case 'l':
			levels = dmesg_get_levels(optarg);
			if (!levels)
				return COMMAND_ERROR_USAGE;
			break;
		case 'r':
			flags |= BAREBOX_LOG_PRINT_RAW | BAREBOX_LOG_PRINT_TIME;
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

	log_print(flags, levels);

	if (delete_buf)
		log_clean(10);

	return 0;
}

BAREBOX_CMD_HELP_START(dmesg)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c",		"Delete messages after printing them")
BAREBOX_CMD_HELP_OPT ("-d",		"Show a time delta to the last message")
BAREBOX_CMD_HELP_OPT ("-e <msg>",	"Emit a log message")
BAREBOX_CMD_HELP_OPT ("-l <vdebug|debug|info|notice|warn|err|crit|alert|emerg>", "Restrict output to the given (comma-separated) list of levels")
BAREBOX_CMD_HELP_OPT ("-r",		"Print timestamp and log-level prefixes.")
BAREBOX_CMD_HELP_OPT ("-t",		"Show timestamp informations")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dmesg)
	.cmd	= do_dmesg,
	BAREBOX_CMD_DESC("Print or control log messages")
	BAREBOX_CMD_OPTS("[-cdert]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_dmesg_help)
BAREBOX_CMD_END
