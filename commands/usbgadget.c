/*
 * usbserial.c - usb serial gadget command
 *
 * Copyright (c) 2011 Eric Bénard <eric@eukrea.com>, Eukréa Electromatique
 * based on dfu.c which is :
 * Copyright (c) 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <environment.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fs.h>
#include <xfuncs.h>
#include <usb/usbserial.h>
#include <usb/dfu.h>
#include <usb/gadget-multi.h>

static int do_usbgadget(int argc, char *argv[])
{
	int opt;
	bool acm = false, dfu = false, fastboot = false, export_bbu = false;
	const char *fastboot_opts = NULL, *dfu_opts = NULL;

	while ((opt = getopt(argc, argv, "asdA::D::b")) > 0) {
		switch (opt) {
		case 'a':
		case 's':
			acm = true;
			break;
		case 'D':
			dfu = true;
			dfu_opts = optarg;
			break;
		case 'A':
			fastboot = true;
			fastboot_opts = optarg;
			break;
		case 'b':
			export_bbu = true;
			break;
		case 'd':
			usb_multi_unregister();
			return 0;
		default:
			return -EINVAL;
		}
	}

	return usbgadget_register(dfu, dfu_opts, fastboot, fastboot_opts, acm,
				  export_bbu);
}

BAREBOX_CMD_HELP_START(usbgadget)
BAREBOX_CMD_HELP_TEXT("Enable / disable a USB composite gadget on the USB device interface.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a\t", "Create CDC ACM function")
BAREBOX_CMD_HELP_OPT ("-A <desc>", "Create Android Fastboot function. If 'desc' is not provided, "
				   "try to use 'global.usbgadget.fastboot_function' variable.")
BAREBOX_CMD_HELP_OPT ("-b\t", "include registered barebox update handlers (fastboot specific)")
BAREBOX_CMD_HELP_OPT ("-D <desc>", "Create DFU function. If 'desc' is not provided, "
				   "try to use 'global.usbgadget.dfu_function' variable.")
BAREBOX_CMD_HELP_OPT ("-d\t", "Disable the currently running gadget")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(usbgadget)
	.cmd		= do_usbgadget,
	BAREBOX_CMD_DESC("Create USB Gadget multifunction device")
	BAREBOX_CMD_OPTS("[-asdAD]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_usbgadget_help)
BAREBOX_CMD_END
