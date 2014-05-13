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

#define PARSE_DEVICE	0
#define PARSE_NAME	1
#define PARSE_FLAGS	2

static int dfu_do_parse_one(char *partstr, char **endstr, struct usb_dfu_dev *dfu)
{
	int i = 0, state = PARSE_DEVICE;
	char device[PATH_MAX];
	char name[PATH_MAX];

	memset(device, 0, sizeof(device));
	memset(name, 0, sizeof(name));
	dfu->flags = 0;

	while (*partstr && *partstr != ',') {
		switch (state) {
		case PARSE_DEVICE:
			if (*partstr == '(') {
				state = PARSE_NAME;
				i = 0;
			} else {
				device[i++] = *partstr;
			}
			break;
		case PARSE_NAME:
			if (*partstr == ')') {
				state = PARSE_FLAGS;
				i = 0;
			} else {
				name[i++] = *partstr;
			}
			break;
		case PARSE_FLAGS:
			switch (*partstr) {
			case 's':
				dfu->flags |= DFU_FLAG_SAFE;
				break;
			case 'r':
				dfu->flags |= DFU_FLAG_READBACK;
				break;
			case 'c':
				dfu->flags |= DFU_FLAG_CREATE;
				break;
			default:
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
		partstr++;
	}

	if (state != PARSE_FLAGS)
		return -EINVAL;

	dfu->name = xstrdup(name);
	dfu->dev = xstrdup(device);
	if (*partstr == ',')
		partstr++;
	*endstr = partstr;

	return 0;
}

/* dfu /dev/self0(bootloader)sr,/dev/nand0.root.bb(root)
 *
 * s = save mode (download whole image before flashing)
 * r = read back (firmware image can be downloaded back from host)
 */
static int do_dfu(int argc, char *argv[])
{
	int opt, n = 0;
	struct usb_dfu_pdata pdata;
	char *endptr, *argstr;
	struct usb_dfu_dev *dfu_alts = NULL;
	char *manufacturer = "barebox";
	const char *productname = barebox_get_model();
	u16 idVendor = 0, idProduct = 0;
	int ret;

	while((opt = getopt(argc, argv, "m:p:V:P:")) > 0) {
		switch(opt) {
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
		}
	}

	if (argc != optind + 1)
		return COMMAND_ERROR_USAGE;

	argstr = argv[optind];

	if (!idProduct && !idVendor) {
		idVendor = 0x1d50; /* Openmoko, Inc */
		idProduct = 0x60a2; /* barebox bootloader USB DFU Mode */
	}

	if ((idProduct && !idVendor) || (!idProduct && idVendor)) {
		printf("Only one of vendor id or product id given\n");
		return -EINVAL;
	}

	for (n = 0; *argstr; n++) {
		dfu_alts = xrealloc(dfu_alts, sizeof(*dfu_alts) * (n + 1));
		if (dfu_do_parse_one(argstr, &endptr, &dfu_alts[n])) {
			printf("parse error\n");
			ret = -EINVAL;
			goto out;
		}
		argstr = endptr;
	}

	pdata.alts = dfu_alts;
	pdata.num_alts = n;

	pdata.manufacturer = manufacturer;
	pdata.productname = productname;
	pdata.idVendor = idVendor;
	pdata.idProduct = idProduct;

	ret = usb_dfu_register(&pdata);

out:
	while (n) {
		n--;
		free(dfu_alts[n].name);
		free(dfu_alts[n].dev);
	};

	free(dfu_alts);

	return ret;
}

BAREBOX_CMD_HELP_START(dfu)
BAREBOX_CMD_HELP_TEXT("Turn's the USB host into DFU mode (Device Firmware Mode) and accepts")
BAREBOX_CMD_HELP_TEXT("a new firmware. The destination is described by DESC in the this format:")
BAREBOX_CMD_HELP_TEXT("  DEVICE(NAME)[src]...")
BAREBOX_CMD_HELP_TEXT("Specify the '(') and ')' literal, the [] however denote this optional modes:")
BAREBOX_CMD_HELP_TEXT("- 's' safe mode (download the complete image before flashing)")
BAREBOX_CMD_HELP_TEXT("- 'r' readback of the firmware is allowed")
BAREBOX_CMD_HELP_TEXT("- 'c' the file will be created (for use with regular files)")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-m STR", "Manufacturer string (barebox)")
BAREBOX_CMD_HELP_OPT ("-p STR", "product string")
BAREBOX_CMD_HELP_OPT ("-V ID",  "vendor id")
BAREBOX_CMD_HELP_OPT ("-P ID",  "product id")
BAREBOX_CMD_HELP_END

/**
 * @page dfu_command
\<description> has the following form:
device1(name1)[sr],device2(name2)[sr]
's' means 'safe mode' (download the complete image before flashing) and
'r' that readback of the firmware is allowed.

\todo Add example, how to use dfu from a Linux or Windows host.
 */

BAREBOX_CMD_START(dfu)
	.cmd		= do_dfu,
	BAREBOX_CMD_DESC("device firmware update")
	BAREBOX_CMD_OPTS("[-mpVP] DESC")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_dfu_help)
BAREBOX_CMD_END
