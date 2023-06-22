// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* dmesg.c - barebox logbuffer handling */

#include <common.h>
#include <malloc.h>
#include <command.h>
#include <globalvar.h>
#include <environment.h>
#include <getopt.h>
#include <clock.h>

static int str_to_loglevel(const char *str)
{
	int ret;
	unsigned long level;

	ret = kstrtoul(str, 10, &level);
	if (!ret) {
		if (level > MSG_VDEBUG)
			goto unknown;
		return level;
	}

	if (!strcmp(str, "vdebug"))
		return MSG_VDEBUG;
	if (!strcmp(str, "debug"))
		return MSG_DEBUG;
	if (!strcmp(str, "info"))
		return MSG_INFO;
	if (!strcmp(str, "notice"))
		return MSG_NOTICE;
	if (!strcmp(str, "warn"))
		return MSG_WARNING;
	if (!strcmp(str, "err"))
		return MSG_ERR;
	if (!strcmp(str, "crit"))
		return MSG_CRIT;
	if (!strcmp(str, "alert"))
		return MSG_ALERT;
	if (!strcmp(str, "emerg"))
		return MSG_EMERG;
unknown:
	printf("dmesg: unknown loglevel %s\n", str);

	return -EINVAL;
}

static unsigned dmesg_get_levels(const char *__args)
{
	char *args = xstrdup(__args);
	char *str, *levels = args;
	unsigned flags = 0;

	while (1) {
		int level;

		str = strsep(&levels, ",");
		if (!str)
			break;

		level = str_to_loglevel(str);
		if (level < 0) {
			flags = 0;
			break;
		}

		flags |= BIT(level);
	}

	free(args);

	return flags;
}

static int do_dmesg(int argc, char *argv[])
{
	int opt, i;
	int delete_buf = 0, emit = 0;
	unsigned flags = 0, levels = 0;
	char *set = NULL;

	while ((opt = getopt(argc, argv, "ctderl:n:")) > 0) {
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
				return COMMAND_ERROR;
			break;
		case 'r':
			flags |= BAREBOX_LOG_PRINT_RAW | BAREBOX_LOG_PRINT_TIME;
			break;
		case 'n':
			set = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (set) {
		int level = str_to_loglevel(set);

		if (level < 0)
			return COMMAND_ERROR;

		barebox_loglevel = level;

		return 0;
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
BAREBOX_CMD_HELP_TEXT("print or control the barebox message buffer")
BAREBOX_CMD_HELP_TEXT("Loglevels can be specified as number (0=emerg, 7=vdebug)")
BAREBOX_CMD_HELP_TEXT("Known debug loglevels are: emerg, alert, crit, err, warn, notice, info, debug,")
BAREBOX_CMD_HELP_TEXT("vdebug")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c",		"Delete messages after printing them")
BAREBOX_CMD_HELP_OPT ("-d",		"Show a time delta to the last message")
BAREBOX_CMD_HELP_OPT ("-e <msg>",	"Emit a log message")
BAREBOX_CMD_HELP_OPT ("-l <loglevel>", "Restrict output to the given (comma-separated) list of levels")
BAREBOX_CMD_HELP_OPT ("-n <loglevel>", "Set level at which printing of messages is done to the console")
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
