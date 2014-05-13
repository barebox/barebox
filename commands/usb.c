/*
 * usb.c - rescan for USB devices
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <command.h>
#include <complete.h>
#include <usb/usb.h>
#include <getopt.h>

static int do_usb(int argc, char *argv[])
{
	int opt;
	int force = 0;

	while ((opt = getopt(argc, argv, "f")) > 0) {
		switch (opt) {
		case 'f':
			force = 1;
			break;
		}
	}

	usb_rescan(force);

	return 0;
}

BAREBOX_CMD_HELP_START(usb)
BAREBOX_CMD_HELP_TEXT("Scan for USB devices.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-f", "force rescan")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(usb)
	.cmd		= do_usb,
	BAREBOX_CMD_DESC("(re-)detect USB devices")
	BAREBOX_CMD_OPTS("[-f]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_usb_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
