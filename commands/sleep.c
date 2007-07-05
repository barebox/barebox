#include <common.h>
#include <command.h>
#include <clock.h>

int do_sleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uint64_t start;
	ulong delay;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	delay = simple_strtoul(argv[1], NULL, 10);

	start = get_time_ns();
	while (!is_timeout(start, delay * SECOND)) {
		if (ctrlc())
			return 1;
	}

	return 0;
}

U_BOOT_CMD_START(sleep)
	.maxargs	= 2,
	.cmd		= do_sleep,
	.usage		= "delay execution for n seconds",
U_BOOT_CMD_END
