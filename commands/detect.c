// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* detect.c - detect devices command */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <driver.h>
#include <getopt.h>
#include <errno.h>

static int do_detect(int argc, char *argv[])
{
	struct device_d *dev;
	int opt, i, ret, err;
	int option_list = 0;
	int option_all = 0;

	while ((opt = getopt(argc, argv, "ela")) > 0) {
		switch (opt) {
		case 'l':
			option_list = 1;
			break;
		case 'e':
			break;
		case 'a':
			option_all = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (option_list) {
		for_each_device(dev) {
			if (dev->detect)
				printf("%s\n", dev_name(dev));
		}
		return 0;
	}

	if (option_all) {
		device_detect_all();
		return 0;
	}

	if (argc == optind)
		return COMMAND_ERROR_USAGE;

	err = 0;

	for (i = optind; i < argc; i++) {
		ret = device_detect_by_name(argv[i]);
		if (!err && ret)
			err = ret;
	}

	return err;
}

BAREBOX_CMD_HELP_START(detect)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l",  "list detectable devices")
BAREBOX_CMD_HELP_OPT ("-a",  "detect all devices")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(detect)
	.cmd		= do_detect,
	BAREBOX_CMD_DESC("detect devices")
	BAREBOX_CMD_OPTS("[-la] [devices]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_COMPLETE(device_complete)
	BAREBOX_CMD_HELP(cmd_detect_help)
BAREBOX_CMD_END
