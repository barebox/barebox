/*
 * echo.c - echo arguments to stdout or into a file
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
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <libbb.h>

static int do_echo(int argc, char *argv[])
{
	int i, optind = 1;
	int fd = stdout, opt, newline = 1;
	char *file = NULL;
	int oflags = O_WRONLY | O_CREAT;
	char str[CONFIG_CBSIZE];
	int process_escape = 0;

	/* We can't use getopt() here because we want to
	 * echo all things we don't understand.
	 */
	while (optind < argc && *argv[optind] == '-') {
		if (!*(argv[optind] + 1) || *(argv[optind] + 2))
			break;

		opt = *(argv[optind] + 1);
		switch (opt) {
		case 'n':
			newline = 0;
			break;
		case 'a':
			oflags |= O_APPEND;
			if (optind + 1 < argc)
				file = argv[optind + 1];
			else
				goto no_optarg_out;
			optind++;
			break;
		case 'o':
			oflags |= O_TRUNC;
			if (optind + 1 < argc)
				file = argv[optind + 1];
			else
				goto no_optarg_out;
			optind++;
			break;
		case 'e':
			process_escape = IS_ENABLED(CONFIG_CMD_ECHO_E);
			break;
		default:
			goto exit_parse;
		}
		optind++;
	}

exit_parse:
	if (file) {
		fd = open(file, oflags);
		if (fd < 0) {
			perror("open");
			return 1;
		}
	}

	for (i = optind; i < argc; i++) {
		if (i > optind)
			fputc(fd, ' ');
		if (process_escape) {
			process_escape_sequence(argv[i], str, CONFIG_CBSIZE);
			fputs(fd, str);
		} else {
			fputs(fd, argv[i]);
		}
	}

	if (newline)
		fputc(fd, '\n');

	if (file)
		close(fd);

	return 0;

no_optarg_out:
	printf("option requires an argument -- %c\n", opt);
	return 1;
}

BAREBOX_CMD_HELP_START(echo)
BAREBOX_CMD_HELP_USAGE("echo [OPTIONS] [STRING]\n")
BAREBOX_CMD_HELP_SHORT("Display a line of text.\n")
BAREBOX_CMD_HELP_OPT  ("-n",  "do not output the trailing newline\n")
BAREBOX_CMD_HELP_OPT  ("-a <file>",  "instead of outputting to stdout append to <file>\n")
BAREBOX_CMD_HELP_OPT  ("-o <file>",  "instead of outputting to stdout overwrite <file>\n")
BAREBOX_CMD_HELP_OPT  ("-e",  "recognize escape sequences\n")
BAREBOX_CMD_HELP_END

/**
 * @page echo_command

\todo Add documentation for -a, -o and -e.

 */

BAREBOX_CMD_START(echo)
	.cmd		= do_echo,
	.usage		= "echo args to console",
	BAREBOX_CMD_HELP(cmd_echo_help)
BAREBOX_CMD_END

