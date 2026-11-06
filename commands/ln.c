// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>

/* ln.c - make links between files */

#include <common.h>
#include <command.h>
#include <libgen.h>
#include <environment.h>
#include <fs.h>
#include <errno.h>
#include <getopt.h>
#include <libbb.h>

static int do_ln(int argc, char *argv[])
{
	const char *target, *dest;
	char *linkname = NULL;
	struct stat s;
	int opt, ret;

	while ((opt = getopt(argc, argv, "s")) > 0) {
		switch (opt) {
		case 's':
			/* all links are symbolic, accept -s for compatibility */
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	target = argv[0];
	dest = argv[1];

	/* like Linux, create the link inside DEST when it is a directory */
	if (!stat(dest, &s) && S_ISDIR(s.st_mode)) {
		char *tmp = xstrdup(target);

		linkname = concat_path_file(dest, posix_basename(tmp));
		free(tmp);
		dest = linkname;
	}

	ret = symlink(target, dest);
	free(linkname);

	if (ret < 0) {
		perror("ln");
		return 1;
	}
	return 0;
}

BAREBOX_CMD_HELP_START(ln)
BAREBOX_CMD_HELP_TEXT("Create a symbolic link DEST pointing to SRC. If DEST is an existing")
BAREBOX_CMD_HELP_TEXT("directory, the link is created in it, named after the last component")
BAREBOX_CMD_HELP_TEXT("of SRC.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-s", "ignored, all links are symbolic links")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ln)
	.cmd		= do_ln,
	BAREBOX_CMD_DESC("create symlink (make a new name for a file)")
	BAREBOX_CMD_OPTS("[-s] SRC DEST")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_ln_help)
BAREBOX_CMD_END
