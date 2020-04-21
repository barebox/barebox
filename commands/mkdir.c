// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* mkdir.c - create directories */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>
#include <libfile.h>

static int do_mkdir(int argc, char *argv[])
{
	int opt, parent = 0, ret;

	while((opt = getopt(argc, argv, "p")) > 0) {
		switch(opt) {
		case 'p':
			parent = 1;
			break;
		default:
			return 1;

		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	while (optind < argc) {
		if (parent) {
			ret = make_directory(argv[optind]);
			if (ret == -EEXIST)
				ret = 0;
		} else {
			ret = mkdir(argv[optind], 0);
		}
		if (ret) {
			printf("could not create %s: %s\n", argv[optind], errno_str());
			return 1;
		}
		optind++;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(mkdir)
BAREBOX_CMD_HELP_TEXT("Create new directories")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-p", "make parent directories as needed")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mkdir)
	.cmd		= do_mkdir,
	BAREBOX_CMD_DESC("make directories")
	BAREBOX_CMD_OPTS("[DIRECTORY ...]")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_mkdir_help)
BAREBOX_CMD_END
