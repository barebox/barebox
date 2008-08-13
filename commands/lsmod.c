#include <common.h>
#include <command.h>
#include <module.h>

static int do_lsmod (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	struct module *mod;

	for_each_module(mod)
		printf("%s\n", mod->name);

	return 0;
}

U_BOOT_CMD_START(lsmod)
	.maxargs	= 1,
	.cmd		= do_lsmod,
	.usage		= "list modules",
U_BOOT_CMD_END
