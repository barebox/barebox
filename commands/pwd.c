#include <common.h>
#include <command.h>
#include <fs.h>

static int do_pwd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("%s\n", getcwd());
	return 0;
}

U_BOOT_CMD_START(pwd)
	.maxargs	= 2,
	.cmd		= do_pwd,
	.usage		= "print working directory",
U_BOOT_CMD_END
