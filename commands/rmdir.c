#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_rmdir(int argc, char *argv[])
{
	int i = 1;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while (i < argc) {
		if (rmdir(argv[i])) {
			printf("could not remove %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}


BAREBOX_CMD_HELP_START(rmdir)
BAREBOX_CMD_HELP_TEXT("Remove directories. The directories have to be empty.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(rmdir)
	.cmd		= do_rmdir,
	BAREBOX_CMD_DESC("remove empty directory(s)")
	BAREBOX_CMD_OPTS("DIRECTORY...")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_rmdir_help)
BAREBOX_CMD_END
