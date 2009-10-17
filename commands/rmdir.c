#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_rmdir (cmd_tbl_t *cmdtp, int argc, char *argv[])
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

static const __maybe_unused char cmd_rmdir_help[] =
"Usage: rmdir [directories]\n"
"Remove directories. The directories have to be empty.\n";

U_BOOT_CMD_START(rmdir)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_rmdir,
	.usage		= "remove directorie(s)",
	U_BOOT_CMD_HELP(cmd_rmdir_help)
U_BOOT_CMD_END
