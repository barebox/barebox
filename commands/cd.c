// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* cd.c - change working directory */

/**
 * @file
 * @brief Change working directory
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <environment.h>

static int do_cd(int argc, char *argv[])
{
	const char *target_dir;
	char *old_pwd;
	int ret;

	if (argc == 1) {
		target_dir = "/";
	} else if (strcmp(argv[1], "-")) {
		target_dir = argv[1];
	} else {
		/* cd - switches to previous directory */
		target_dir = getenv("OLDPWD");
		if (!target_dir) {
			printf("cd: OLDPWD not set\n");
			return 1;
		}
		/* Print the directory we're changing to, like bash does */
		printf("%s\n", target_dir);
	}

	/* Save current directory before changing */
	old_pwd = xstrdup(getcwd());

	ret = chdir(target_dir);
	if (ret) {
		perror("chdir");
		return 1;
	}

	setenv("OLDPWD", old_pwd);
	free(old_pwd);

	return 0;
}

BAREBOX_CMD_HELP_START(cd)
BAREBOX_CMD_HELP_TEXT("If called without an argument, change to the root directory '/'.")
BAREBOX_CMD_HELP_TEXT("Use 'cd -' to change to the previous directory.")
BAREBOX_CMD_HELP_TEXT("Sets OLDPWD environment variables.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(cd)
	.cmd		= do_cd,
	BAREBOX_CMD_DESC("change working directory")
	BAREBOX_CMD_OPTS("DIRECTORY")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_cd_help)
BAREBOX_CMD_END
