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
	int acm = 1, create_serial = 0;
	char *fastboot_opts = NULL, *dfu_opts = NULL;
	struct f_multi_opts opts = {};

	while ((opt = getopt(argc, argv, "asdA:D:")) > 0) {
		switch (opt) {
		case 'a':
			acm = 1;
			create_serial = 1;
			break;
		case 's':
			acm = 0;
			create_serial = 1;
			break;
		case 'D':
			dfu_opts = optarg;
			break;
		case 'A':
			fastboot_opts = optarg;
			break;
		case 'd':
			usb_multi_unregister();
			return 0;
		default:
			return -EINVAL;
		}
	}

	if (!dfu_opts && !fastboot_opts && !create_serial)
		return COMMAND_ERROR_USAGE;

	/*
	 * Creating a gadget with both DFU and Fastboot doesn't work.
	 * Both client tools seem to assume that the device only has
	 * a single configuration
	 */
	if (fastboot_opts && dfu_opts) {
		printf("Only one of Fastboot and DFU allowed\n");
		return -EINVAL;
	}

	if (fastboot_opts) {
		opts.fastboot_opts.files = file_list_parse(fastboot_opts);
	}

	if (dfu_opts) {
		opts.dfu_opts.files = file_list_parse(dfu_opts);
	}

	if (create_serial) {
		opts.create_acm = acm;
	}

	return usb_multi_register(&opts);
}

BAREBOX_CMD_HELP_START(usbgadget)
BAREBOX_CMD_HELP_TEXT("Enable / disable a USB composite gadget on the USB device interface.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a",   "Create CDC ACM function")
BAREBOX_CMD_HELP_OPT ("-s",   "Create Generic Serial function")
BAREBOX_CMD_HELP_OPT ("-A <desc>",   "Create Android Fastboot function")
BAREBOX_CMD_HELP_OPT ("-D <desc>",   "Create DFU function")
BAREBOX_CMD_HELP_OPT ("-d",   "Disable the currently running gadget")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(usbgadget)
	.cmd		= do_usbgadget,
	BAREBOX_CMD_DESC("Create USB Gadget multifunction device")
	BAREBOX_CMD_OPTS("[-asdAD]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_usbgadget_help)
BAREBOX_CMD_END
