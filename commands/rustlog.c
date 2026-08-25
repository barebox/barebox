// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <linux/printk.h>

static int do_rustlog(int argc, char *argv[])
{
	int opt, level = LOGLEVEL_INFO;

	while ((opt = getopt(argc, argv, "l:")) > 0) {
		switch (opt) {
		case 'l':
			if (kstrtoint(optarg, 0, &level))
				return COMMAND_ERROR_USAGE;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind != argc - 1)
		return COMMAND_ERROR_USAGE;

	return rust_log(level, argv[optind]);
}

BAREBOX_CMD_HELP_START(rustlog)
BAREBOX_CMD_HELP_TEXT("Emit MESSAGE through the Rust log crate.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-l LEVEL", "barebox loglevel (0..8, default 6 = info)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(rustlog)
	.cmd		= do_rustlog,
	BAREBOX_CMD_DESC("log a message via Rust")
	BAREBOX_CMD_OPTS("[-l LEVEL] MESSAGE")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_rustlog_help)
BAREBOX_CMD_END
