#include <common.h>
#include <command.h>
#include <module.h>

static int do_lsmod(struct command *cmdtp, int argc, char *argv[])
{
	struct module *mod;

	for_each_module(mod)
		printf("%s\n", mod->name);

	return 0;
}

BAREBOX_CMD_START(lsmod)
	.cmd		= do_lsmod,
	.usage		= "list modules",
BAREBOX_CMD_END
