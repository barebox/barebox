#include <common.h>
#include <command.h>
#include <magicvar.h>

static int do_magicvar(int argc, char *argv[])
{
	struct magicvar *m;

	for (m = &__barebox_magicvar_start;
			m != &__barebox_magicvar_end;
			m++)
		printf("%-32s %s\n", m->name, m->description);

	return 0;
}

BAREBOX_CMD_START(magicvar)
	.cmd		= do_magicvar,
	.usage		= "List information about magic variables",
BAREBOX_CMD_END
