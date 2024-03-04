// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>

/* readlink.c - read value of a symbolic link */

#include <common.h>
#include <command.h>
#include <libgen.h>
#include <environment.h>
#include <fs.h>
#include <malloc.h>
#include <getopt.h>

static int do_readlink(int argc, char *argv[])
{
	char realname[PATH_MAX];
	int canonicalize = 0;
	int opt;

	memset(realname, 0, PATH_MAX);

	while ((opt = getopt(argc, argv, "f")) > 0) {
		switch (opt) {
		case 'f':
			canonicalize = 1;
			break;
		}
	}

	if (argc < optind + 2)
		return COMMAND_ERROR_USAGE;

	if (canonicalize) {
		char *buf = canonicalize_path(AT_FDCWD, argv[optind]);
		struct stat s;

		if (!buf)
			goto err;
		if (stat(dirname(argv[optind]), &s) || !S_ISDIR(s.st_mode)) {
			free(buf);
			goto err;
		}
		setenv(argv[optind + 1], buf);
		free(buf);
	} else {
		if (readlink(argv[optind], realname, PATH_MAX - 1) < 0)
			goto err;
		setenv(argv[optind + 1], realname);
	}

	return 0;
err:
	unsetenv(argv[optind + 1]);
	return 1;
}

BAREBOX_CMD_HELP_START(readlink)
BAREBOX_CMD_HELP_TEXT("Read value of a symbolic link or canonical file name and store it into VARIABLE.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f", "canonicalize by following symlinks;")
BAREBOX_CMD_HELP_OPT("",   "final component need not exist");
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(readlink)
	.cmd		= do_readlink,
	BAREBOX_CMD_DESC("read value of a symbolic link or canonical file name")
	BAREBOX_CMD_OPTS("[-f] FILE VARIABLE")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_readlink_help)
BAREBOX_CMD_END
