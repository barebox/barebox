// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2022 Ahmad Fatoum, Pengutronix

#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <stringlist.h>

static int do_stat(int argc, char *argv[])
{
	int (*statfn)(int dirfd, const char *, struct stat *) = lstatat;
	int ret, opt, dirfd = AT_FDCWD, extra_flags = 0, exitcode = 0;
	char **filename;
	struct stat st;

	while((opt = getopt(argc, argv, "Lc:C:")) > 0) {
		switch(opt) {
		case 'L':
			statfn = statat;
			break;
		case 'C':
			extra_flags |= O_CHROOT;
			fallthrough;
		case 'c':
			dirfd = open(optarg, O_PATH | extra_flags);
			if (dirfd < 0)
				return dirfd;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	for (filename = &argv[optind]; *filename; filename++) {
		ret = statfn(dirfd, *filename, &st);

		if (ret) {
			printf("%s: %s: %m\n", argv[0], *filename);
			exitcode = COMMAND_ERROR;
			continue;
		}

		stat_print(dirfd, *filename, &st);
	}

	close(dirfd);

	return exitcode;
}

BAREBOX_CMD_HELP_START(stat)
BAREBOX_CMD_HELP_TEXT("Display status information about the specified files")
BAREBOX_CMD_HELP_TEXT("or directories.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-L",  "follow links")
BAREBOX_CMD_HELP_OPT ("-c DIR",  "lookup file relative to directory DIR")
BAREBOX_CMD_HELP_OPT ("-C DIR",  "change root to DIR before file lookup")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(stat)
	.cmd		= do_stat,
	BAREBOX_CMD_DESC("display file status")
	BAREBOX_CMD_OPTS("[-LcC] [FILEDIR...]")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_stat_help)
BAREBOX_CMD_END
