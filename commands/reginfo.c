#include <common.h>
#include <command.h>

int do_reginfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	reginfo();
	return 0;
}

U_BOOT_CMD_START(reginfo)
	.maxargs	= 1,
	.cmd		= do_reginfo,
	.usage		= "print register information",
U_BOOT_CMD_END
