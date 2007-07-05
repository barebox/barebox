#include <common.h>
#include <command.h>
#include <readkey.h>

int do_clear (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf(ANSI_CLEAR_SCREEN);

	return 0;
}

U_BOOT_CMD_START(clear)
	.maxargs	= 1,
	.cmd		= do_clear,
	.usage		= "clear screen",
U_BOOT_CMD_END
