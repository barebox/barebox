#include <common.h>
#include <command.h>

static int cmd_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	do_reset();

	/* Not reached */
	return 1;
}

U_BOOT_CMD_START(reset)
	.maxargs	= 1,
	.cmd		= cmd_reset,
	.usage		= "Perform RESET of the CPU",
U_BOOT_CMD_END
