// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015 by Ari Sundholm <ari@tuxera.com>
 */

#include <common.h>
#include <command.h>
#include <fcntl.h>
#include <getopt.h>
#include <fs.h>

static int do_truncate(int argc, char *argv[])
{
	int flags = O_CREAT | O_WRONLY;
	int opt, modify = 0, ret = 0;
	const char *size_str;
	off_t size = -1;

	while((opt = getopt(argc, argv, "cs:")) > 0) {
		switch(opt) {
		case 'c':
			flags &= ~O_CREAT;
			break;
		case 's':
			size_str = optarg;
			if (!size_str)
				return COMMAND_ERROR_USAGE;
			switch (*size_str) {
			case '+':
				modify = 1;
				size_str++;
				break;
			case '-':
				return COMMAND_ERROR_USAGE;
			}
			size = strtoull_suffix(size_str, NULL, 10);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if (size == -1 || argc == 0)
		return COMMAND_ERROR_USAGE;

	for (int i = 0; i < argc; i++) {
		int fd = open(argv[i], flags, 0666);
		if (fd < 0) {
			if (errno != ENOENT || !(flags & O_CREAT)) {
				perror("open");
				ret = 1;
			}
			/* else: ENOENT && OPT_NOCREATE:
			 * do not report error, exitcode is also 0.
			 */
			continue;
		}

		if (modify) {
			struct stat st;

			if (fstat(fd, &st) == -1) {
				perror("fstat");
				ret = 1;
				goto close;
			}

			size = st.st_size + modify * size;
		}

		if (ftruncate(fd, size) == -1) {
			perror("truncate");
			ret = 1;
		}
close:
		close(fd);
	}

	return ret;
}

BAREBOX_CMD_HELP_START(truncate)
BAREBOX_CMD_HELP_TEXT("Set files to specified size. Missing files are created")
BAREBOX_CMD_HELP_TEXT("unless -c option is used. With -s +SIZE, increase size")
BAREBOX_CMD_HELP_TEXT("by SIZE bytes; with -s SIZE, set absolute size.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c",     "Do not create files")
BAREBOX_CMD_HELP_OPT ("-s [+]SIZE", "truncate file to SIZE, or increase by +SIZE")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(truncate)
	.cmd		= do_truncate,
	BAREBOX_CMD_DESC("truncate files to size")
	BAREBOX_CMD_OPTS("[-c] -s [+]SIZE FILE...")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_truncate_help)
BAREBOX_CMD_END
