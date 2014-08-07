/*
 * dfu.c - device firmware update command
 *
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
#include <usb/dfu.h>
#include <linux/err.h>

/* dfu /dev/self0(bootloader)sr,/dev/nand0.root.bb(root)
 *
 * s = save mode (download whole image before flashing)
 * r = read back (firmware image can be downloaded back from host)
 */
static int do_dfu(int argc, char *argv[])
{
	struct f_dfu_opts opts;
	char *argstr;
	struct usb_dfu_dev *dfu_alts = NULL;
	int ret;

	if (argc != optind + 1)
		return COMMAND_ERROR_USAGE;

	argstr = argv[optind];

	opts.files = file_list_parse(argstr);
	if (IS_ERR(opts.files)) {
		ret = PTR_ERR(opts.files);
		goto out;
	}

	ret = usb_dfu_register(&opts);

	file_list_free(opts.files);
out:

	free(dfu_alts);

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
