/*
 * barebox-update.c - update barebox
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 */
#include <common.h>
#include <command.h>
#include <getopt.h>
#include <malloc.h>
#include <errno.h>
#include <bbu.h>
#include <fs.h>

static int do_barebox_update(int argc, char *argv[])
{
	int opt, ret;
	struct bbu_data data = {};

	while ((opt = getopt(argc, argv, "t:yf:ld:")) > 0) {
		switch (opt) {
		case 'd':
			data.devicefile = optarg;
			break;
		case 'f':
			data.force = simple_strtoul(optarg, NULL, 0);
			data.flags |= BBU_FLAG_FORCE;
			break;
		case 't':
			data.handler_name = optarg;
			break;
		case 'y':
			data.flags |= BBU_FLAG_YES;
			break;
		case 'l':
			printf("registered update handlers:\n");
			bbu_handlers_list();
			return 0;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!(argc - optind))
		return COMMAND_ERROR_USAGE;

	data.imagefile = argv[optind];

	data.image = read_file(data.imagefile, &data.len);
	if (!data.image)
		return -errno;

	ret = barebox_update(&data);

	free(data.image);

	return ret;
}

BAREBOX_CMD_HELP_START(barebox_update)
BAREBOX_CMD_HELP_USAGE("barebox_update [OPTIONS] <image>\n")
BAREBOX_CMD_HELP_SHORT("Update barebox to persistent media\n")
BAREBOX_CMD_HELP_OPT("-t <target>", "\n")
BAREBOX_CMD_HELP_OPT("-d <device>", "write image to <device> instead of handler default\n")
BAREBOX_CMD_HELP_OPT("           ", "Can be used for debugging purposes (-d /tmpfile)\n")
BAREBOX_CMD_HELP_OPT("-y\t", "yes. Do not ask for confirmation\n")
BAREBOX_CMD_HELP_OPT("-f <level>", "Set force level\n")
BAREBOX_CMD_HELP_OPT("-l\t", "list registered targets\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(barebox_update)
	.cmd		= do_barebox_update,
	.usage		= "update barebox",
	BAREBOX_CMD_HELP(cmd_barebox_update_help)
BAREBOX_CMD_END
