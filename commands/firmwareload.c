/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <firmware.h>

static int do_firmwareload(int argc, char *argv[])
{
	int ret, opt;
	const char *name = NULL, *firmware;
	struct firmware_mgr *mgr;

	while ((opt = getopt(argc, argv, "t:l")) > 0) {
		switch (opt) {
		case 't':
			name = optarg;
			break;
		case 'l':
			firmwaremgr_list_handlers();
			return 0;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!(argc - optind))
		return COMMAND_ERROR_USAGE;

	firmware = argv[optind];

	mgr = firmwaremgr_find(name);

	if (!mgr) {
		printf("No such programming handler found: %s\n",
				name ? name : "default");
		return 1;
	}

	ret = firmwaremgr_load_file(mgr, firmware);

	return ret;
}

BAREBOX_CMD_HELP_START(firmwareload)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-t <target>", "define the firmware handler by name\n")
BAREBOX_CMD_HELP_OPT("-l\t", "list devices capable of firmware loading\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(firmwareload)
	.cmd = do_firmwareload,
	BAREBOX_CMD_DESC("Program a firmware file into a device")
	BAREBOX_CMD_HELP(cmd_firmwareload_help)
BAREBOX_CMD_END
