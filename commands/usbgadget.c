// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: © 2011 Eric Bénard <eric@eukrea.com>, Eukréa Electromatique

/* usbserial.c - usb serial gadget command */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fs.h>
#include <xfuncs.h>
#include <linux/usb/usbserial.h>
#include <linux/usb/dfu.h>
#include <linux/usb/gadget-multi.h>

static int do_usbgadget(int argc, char *argv[])
{
	struct usbgadget_funcs funcs = {};
	int opt;
	int ret;

	while ((opt = getopt(argc, argv, "asdA::D::S::b")) > 0) {
		switch (opt) {
		case 'a':
		case 's':
			funcs.flags |= USBGADGET_ACM;
			break;
		case 'D':
			funcs.flags |= USBGADGET_DFU;
			funcs.dfu_opts = optarg;
			break;
		case 'A':
			funcs.flags |= USBGADGET_FASTBOOT;
			funcs.fastboot_opts = optarg;
			break;
		case 'S':
			funcs.flags |= USBGADGET_MASS_STORAGE;
			funcs.ums_opts = optarg;
			break;
		case 'b':
			funcs.flags |= USBGADGET_EXPORT_BBU;
			break;
		case 'd':
			usb_multi_unregister();
			return 0;
		default:
			return -EINVAL;
		}
	}

	ret = usbgadget_prepare_register(&funcs);
	return ret ? COMMAND_ERROR_USAGE : 0;
}

BAREBOX_CMD_HELP_START(usbgadget)
BAREBOX_CMD_HELP_TEXT("Enable / disable a USB composite gadget on the USB device interface.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a\t", "Create CDC ACM function")
BAREBOX_CMD_HELP_OPT ("-A <desc>", "Create Android Fastboot function. If 'desc' is not provided, "
				   "try to use 'global.fastboot.partitions' variable.")
BAREBOX_CMD_HELP_OPT ("-b\t", "include registered barebox update handlers (fastboot specific,"
			      "exported as 'bbu-<update_handler_name>' partitions)")
BAREBOX_CMD_HELP_OPT ("-D <desc>", "Create DFU function. If 'desc' is not provided, "
				   "try to use 'global.usbgadget.dfu_function' variable.")
BAREBOX_CMD_HELP_OPT ("-S <desc>", "Create USB Mass Storage function. If 'desc' is not provided,"
				   "fallback directly to 'global.system.partitions' variable.")
BAREBOX_CMD_HELP_OPT ("-d\t", "Disable the currently running gadget")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(usbgadget)
	.cmd		= do_usbgadget,
	BAREBOX_CMD_DESC("Create USB Gadget multifunction device")
	BAREBOX_CMD_OPTS("[-adADS]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_usbgadget_help)
BAREBOX_CMD_END
