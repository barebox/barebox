#include <common.h>
#include <command.h>

int do_meminfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	malloc_stats();

	return 0;
}

U_BOOT_CMD_START(meminfo)
	.maxargs	= 1,
	.cmd		= do_meminfo,
	.usage		= "print info about memory usage",
U_BOOT_CMD_END
