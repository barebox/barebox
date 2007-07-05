#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_cd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	if (argc == 1)
		ret = chdir("/");
	else
		ret = chdir(argv[1]);

	if (ret) {
		perror("chdir");
		return 1;
	}

	return 0;
}

static __maybe_unused char cmd_cd_help[] =
"Usage: cd [directory]\n"
"change to directory. If called without argument, change to /\n";

U_BOOT_CMD_START(cd)
	.maxargs	= 2,
	.cmd		= do_cd,
	.usage		= "change working directory",
	U_BOOT_CMD_HELP(cmd_cd_help)
U_BOOT_CMD_END
