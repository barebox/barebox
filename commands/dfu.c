// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2009 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* dfu.c - device firmware update command */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <fs.h>
#include <xfuncs.h>
#include <linux/usb/dfu.h>
#include <linux/usb/gadget-multi.h>
#include <linux/err.h>

/* dfu /dev/self0(bootloader)sr,/dev/nand0.root.bb(root)
 *
 * s = save mode (download whole image before flashing)
 * r = read back (firmware image can be downloaded back from host)
 */
static int do_dfu(int argc, char *argv[])
{
	struct usbgadget_funcs funcs = {};
	int ret;

	if (argc != optind + 1)
		return COMMAND_ERROR_USAGE;

	funcs.flags |= USBGADGET_DFU;
	funcs.dfu_opts = argv[optind];
	ret = usbgadget_prepare_register(&funcs);
	if (ret)
		return COMMAND_ERROR_USAGE;

	command_slice_release();
	while (!usb_dfu_detached()) {
		if (ctrlc()) {
			ret = -EINTR;
			break;
		}
	}
	command_slice_acquire();

	usb_multi_unregister();

	return ret;
}

BAREBOX_CMD_HELP_START(dfu)
BAREBOX_CMD_HELP_TEXT("Turn's the USB host into DFU mode (Device Firmware Mode) and accepts")
BAREBOX_CMD_HELP_TEXT("a new firmware. The destination is described by DESC in the format")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("\tDEVICE(NAME)[src]...")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Here '(' and ')' are literal characters. The '[' and ']' however denote")
BAREBOX_CMD_HELP_TEXT("one of the following optional modes:")
BAREBOX_CMD_HELP_TEXT("'s': safe mode (download the complete image before flashing); ")
BAREBOX_CMD_HELP_TEXT("'r': readback of the firmware is allowed; ")
BAREBOX_CMD_HELP_TEXT("'c': the file will be created (for use with regular files).")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(dfu)
	.cmd		= do_dfu,
	BAREBOX_CMD_DESC("device firmware update")
	BAREBOX_CMD_OPTS("DESC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_dfu_help)
BAREBOX_CMD_END
