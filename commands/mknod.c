// SPDX-License-Identifier: GPL-2.0-only

#include <command.h>
#include <fs.h>

static int do_mknod(int argc, char *argv[])
{
	const char *filename, *cdevname;
	umode_t mode;

	if (argc != 4)
		return COMMAND_ERROR_USAGE;

	filename = argv[1];
	if (!strcmp(argv[2], "b"))
		mode = S_IFBLK;
	else if (!strcmp(argv[2], "c"))
		mode = S_IFCHR;
	else
		return COMMAND_ERROR_USAGE;

	cdevname = argv[3];

	return mknod(filename, mode, cdevname);
}

BAREBOX_CMD_HELP_START(mknod)
BAREBOX_CMD_HELP_TEXT("make a device special node.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("usage: mknod <name> c|b <cdevname>")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Create a device special node named <name> directing")
BAREBOX_CMD_HELP_TEXT("to cdev <cdevname>. This can either be a block (b) or")
BAREBOX_CMD_HELP_TEXT("character (c) device.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mknod)
        .cmd            = do_mknod,
        BAREBOX_CMD_DESC("make device nodes")
        BAREBOX_CMD_OPTS("NAME TYPE CDEVNAME")
        BAREBOX_CMD_GROUP(CMD_GRP_FILE)
        BAREBOX_CMD_HELP(cmd_mknod_help)
BAREBOX_CMD_END
