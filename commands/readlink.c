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

enum can_mode { CAN_NONE, CAN_ALL_BUT_LAST, CAN_EXISTING };

static void output_result(const char *var, const char *val)
{
	if (var)
		setenv(var, val);
	else
		printf("%s\n", val);

}

static int canonicalize_filename_mode(const char *var, char *path, enum can_mode can_mode)
{
	char *buf = NULL, *file, *dir;
	struct stat s;
	int ret = -1;

	buf = canonicalize_path(AT_FDCWD, path);
	if (buf)
		goto out;

	switch (can_mode) {
	case CAN_ALL_BUT_LAST:
		file = basename(path);
		dir = dirname(path);

		buf = canonicalize_path(AT_FDCWD, dir);
		if (!buf || stat(dir, &s) || !S_ISDIR(s.st_mode))
			goto err;

		buf = xrasprintf(buf, "/%s", file);
		break;
	case CAN_EXISTING:
	case CAN_NONE:
		goto err;
	}

out:
	output_result(var, buf);
	ret = 0;
err:
	free(buf);
	return ret;
}

static int do_readlink(int argc, char *argv[])
{
	const char *var;
	char *path, realname[PATH_MAX];
	enum can_mode can_mode = CAN_NONE;
	int opt;

	memset(realname, 0, PATH_MAX);

	while ((opt = getopt(argc, argv, "fe")) > 0) {
		switch (opt) {
		case 'f':
			can_mode = CAN_ALL_BUT_LAST;
			break;
		case 'e':
			can_mode = CAN_EXISTING;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc == 0 || argc > 2)
		return COMMAND_ERROR_USAGE;

	path = argv[0];
	var = argv[1];

	if (can_mode > CAN_NONE) {
		if (canonicalize_filename_mode(var, path, can_mode))
			goto err;
	} else {
		if (readlink(path, realname, PATH_MAX - 1) < 0)
			goto err;
		output_result(var, realname);
	}

	return 0;
err:
	if (var)
		unsetenv(var);
	return 1;
}

BAREBOX_CMD_HELP_START(readlink)
BAREBOX_CMD_HELP_TEXT("Read value of a symbolic link or canonical file name")
BAREBOX_CMD_HELP_TEXT("The value is either stored it into the specified VARIABLE")
BAREBOX_CMD_HELP_TEXT("or printed.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f", "canonicalize by following symlinks; final component need not exist")
BAREBOX_CMD_HELP_OPT("-e", "canonicalize by following symlinks; all components must exist")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(readlink)
	.cmd		= do_readlink,
	BAREBOX_CMD_DESC("read value of a symbolic link or canonical file name")
	BAREBOX_CMD_OPTS("[-fe] FILE [VARIABLE]")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_readlink_help)
BAREBOX_CMD_END
