// SPDX-License-Identifier: GPL-2.0-only

#include <command.h>
#include <malloc.h>
#include <abort.h>
#include <getopt.h>
#include <linux/kstrtox.h>

static int do_checkleak(int argc, char *argv[])
{
	unsigned int count;
	int opt;

	while ((opt = getopt(argc, argv, "l:")) > 0) {
		switch(opt) {
		case 'l':
			if (kstrtouint(optarg, 0, &count))
				return COMMAND_ERROR;
			(void)malloc(count);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc)
		return COMMAND_ERROR_USAGE;

	memleak_check();

	return 0;
}

BAREBOX_CMD_HELP_START(checkleak)
BAREBOX_CMD_HELP_TEXT("list memory leaks encountered since the last time")
BAREBOX_CMD_HELP_TEXT("the command ran.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l COUNT",  "force leak of COUNT bytes")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(checkleak)
	.cmd		= do_checkleak,
	BAREBOX_CMD_DESC("check for memory leaks")
	BAREBOX_CMD_OPTS("[-l]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_checkleak_help)
BAREBOX_CMD_END

