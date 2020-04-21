// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: © 2011 Eric Bénard <eric@eukrea.com>, Eukréa Electromatique

/* usbserial.c - usb serial gadget command */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fs.h>
#include <xfuncs.h>
#include <usb/usbserial.h>

static int do_usbserial(int argc, char *argv[])
{
	int opt;
	struct usb_serial_pdata pdata;
	int acm = 1;

	while ((opt = getopt(argc, argv, "asd")) > 0) {
		switch (opt) {
		case 'a':
			acm = 1;
			break;
		case 's':
			acm = 0;
			break;
		case 'd':
			usb_serial_unregister();
			return 0;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	pdata.acm = acm;

	return usb_serial_register(&pdata);
}

BAREBOX_CMD_HELP_START(usbserial)
BAREBOX_CMD_HELP_TEXT("Enable / disable a serial gadget on the USB device interface.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",   "CDC ACM (default)")
BAREBOX_CMD_HELP_OPT ("-s",   "Generic Serial")
BAREBOX_CMD_HELP_OPT ("-d",   "Disable the serial gadget")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(usbserial)
	.cmd		= do_usbserial,
	BAREBOX_CMD_DESC("serial gadget enable/disable")
	BAREBOX_CMD_OPTS("[-asd] <description>")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_usbserial_help)
BAREBOX_CMD_END
