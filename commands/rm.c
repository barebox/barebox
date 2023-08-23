// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* rm.c - remove files */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <libfile.h>
#include <getopt.h>
#include <errno.h>

static int do_rm(int argc, char *argv[])
{
	int i, opt, recursive = 0, force = 0;

	while ((opt = getopt(argc, argv, "rf")) > 0) {
		switch (opt) {
		case 'r':
			recursive = 1;
			break;
		case 'f':
			force = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	i = optind;

	while (i < argc) {
		int ret;

		if (recursive)
			ret = unlink_recursive(argv[i], NULL);
		else
			ret = unlink(argv[i]);
		if (ret) {
			if (!force || ret != -ENOENT)
				printf("could not remove %s: %m\n", argv[i]);
			if (!force)
				return 1;
		}
		i++;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(rm)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-r", "remove directories and their contents recursively")
BAREBOX_CMD_HELP_OPT ("-f", "ignore nonexistent files")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(rm)
	.cmd		= do_rm,
	BAREBOX_CMD_DESC("remove files")
	BAREBOX_CMD_OPTS("[-rf] FILES...")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_rm_help)
BAREBOX_CMD_END
