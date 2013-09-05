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

static int do_usbserial(int argc, char *argv[])
{
	int opt;
	struct usb_serial_pdata pdata;
	char *argstr;
	char *manufacturer = "barebox";
	const char *productname = barebox_get_model();
	u16 idVendor = 0, idProduct = 0;
	int mode = 0;

	while ((opt = getopt(argc, argv, "m:p:V:P:asd")) > 0) {
		switch (opt) {
		case 'm':
			manufacturer = optarg;
			break;
		case 'p':
			productname = optarg;
			break;
		case 'V':
			idVendor = simple_strtoul(optarg, NULL, 0);
			break;
		case 'P':
			idProduct = simple_strtoul(optarg, NULL, 0);
			break;
		case 'a':
			mode = 0;
			break;
#ifdef HAVE_OBEX
		case 'o':
			mode = 1;
			break;
#endif
		case 's':
			mode = 2;
			break;
		case 'd':
			usb_serial_unregister();
			return 0;
		}
	}

	argstr = argv[optind];

	pdata.manufacturer = manufacturer;
	pdata.productname = productname;
	pdata.idVendor = idVendor;
	pdata.idProduct = idProduct;
	pdata.mode = mode;

	return usb_serial_register(&pdata);
}

BAREBOX_CMD_HELP_START(usbserial)
BAREBOX_CMD_HELP_USAGE("usbserial [OPTIONS] <description>\n")
BAREBOX_CMD_HELP_SHORT("Enable/disable a serial gadget on the USB device interface.\n")
BAREBOX_CMD_HELP_OPT  ("-m <str>",  "Manufacturer string (barebox)\n")
BAREBOX_CMD_HELP_OPT  ("-p <str>",  "product string\n")
BAREBOX_CMD_HELP_OPT  ("-V <id>",   "vendor id\n")
BAREBOX_CMD_HELP_OPT  ("-P <id>",   "product id\n")
BAREBOX_CMD_HELP_OPT  ("-a",   "CDC ACM (default)\n")
#ifdef HAVE_OBEX
BAREBOX_CMD_HELP_OPT  ("-o",   "CDC OBEX\n")
#endif
BAREBOX_CMD_HELP_OPT  ("-s",   "Generic Serial\n")
BAREBOX_CMD_HELP_OPT  ("-d",   "Disable the serial gadget\n")
BAREBOX_CMD_HELP_END

/**
 * @page usbserial_command
 */

BAREBOX_CMD_START(usbserial)
	.cmd		= do_usbserial,
	.usage		= "Serial gadget enable/disable",
	BAREBOX_CMD_HELP(cmd_usbserial_help)
BAREBOX_CMD_END
