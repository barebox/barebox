#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 1;

	if (argc < 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	while (i < argc) {
		if (mkdir(argv[i])) {
			printf("could not create %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_mkdir_help[] =
"Usage: mkdir [directories]\n"
"Create new directories\n";

U_BOOT_CMD_START(mkdir)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mkdir,
	.usage		= "make directories",
	U_BOOT_CMD_HELP(cmd_mkdir_help)
U_BOOT_CMD_END
