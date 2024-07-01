// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Port of Mini watch implementation from busybox
 *
 * Copyright (C) 2001 by Michael Habermann <mhabermann@gmx.de>
 * Copyrigjt (C) Mar 16, 2003 Manuel Novoa III   (mjn3@codepoet.org)
 */

#include <common.h>
#include <command.h>
#include <clock.h>
#include <linux/math64.h>
#include <malloc.h>
#include <getopt.h>
#include <term.h>
#include <rtc.h>

static int do_watch(int argc , char *argv[])
{
	const char *period_str = "2.0";
	u64 period_ns, start;
	bool print_header = true;
	int opt;
	unsigned width, new_width;
	char *end, *header, *cmd;

	while ((opt = getopt(argc, argv, "+n:t")) > 0) {
		switch (opt) {
		case 'n':
			period_str = optarg;
			break;
		case 't':
			print_header = false;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
		return COMMAND_ERROR_USAGE;

	period_ns = simple_strtofract(period_str, &end, NSEC_PER_SEC);
	if (*end)
		return -EINVAL;

	cmd = strjoin(" ", argv, argc);

	width = (unsigned)-1; // make sure first time new_width != width
	header = NULL;

	while (true) {
		/* home; clear to the end of screen */
		printf("\e[H\e[J");

		if (print_header) {
			term_getsize(&new_width, NULL);
			if (new_width != width) {
				width = new_width;
				free(header);
				header = xasprintf("Every %ss: %-*s",
						   period_str, (int)width, cmd);
			}

			printf("%s\n\n", header);
		}

		run_command(cmd);

		start = get_time_ns();
		while (!is_timeout(start, period_ns)) {
			if (ctrlc())
				goto out;
		}
	}

out:
	free(header);
	free(cmd);

	return 0;
}

BAREBOX_CMD_HELP_START(watch)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-n SEC", "Period (default 2)")
BAREBOX_CMD_HELP_OPT ("-t",	"Don't print header")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(watch)
	.cmd		= do_watch,
	BAREBOX_CMD_DESC("run program periodically")
	BAREBOX_CMD_OPTS("[-n SEC] [-t] PROG ARGS")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_watch_help)
BAREBOX_CMD_END
