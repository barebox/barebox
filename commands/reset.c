// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* reset.c - reset the cpu */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <getopt.h>
#include <restart.h>

static int cmd_reset(int argc, char *argv[])
{
	struct restart_handler *rst;
	int opt, shutdown_flag, flags = 0;
	const char *name = NULL;

	shutdown_flag = 1;

	while ((opt = getopt(argc, argv, "flwr:")) > 0) {
		switch (opt) {
		case 'f':
			shutdown_flag = 0;
			break;
		case 'l':
			restart_handlers_print();
			return 0;
		case 'w':
			flags |= RESTART_FLAG_WARM_BOOTROM;
			break;
		case 'r':
			name = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	rst = restart_handler_get_by_name(name, flags);
	if (!rst && (name || flags)) {
		printf("No matching restart handler found\n");
		return COMMAND_ERROR;
	}

	if (shutdown_flag)
		shutdown_barebox();

	if (rst) {
		console_flush();
		rst->restart(rst, 0);
	}

	hang();

	/* Not reached */
	return 1;
}

BAREBOX_CMD_HELP_START(reset)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f",  "force RESET, don't call shutdown")
BAREBOX_CMD_HELP_OPT("-l",  "list reset handlers")
BAREBOX_CMD_HELP_OPT("-w",  "only consider warm BootROM reboot-mode-preserving resets")
BAREBOX_CMD_HELP_OPT("-r RESET",  "use reset handler named RESET")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(reset)
	.cmd		= cmd_reset,
	BAREBOX_CMD_DESC("perform RESET of the CPU")
	BAREBOX_CMD_OPTS("[-flrw]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_reset_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
