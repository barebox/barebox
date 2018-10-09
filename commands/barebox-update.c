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
#include <libfile.h>
#include <getopt.h>
#include <malloc.h>
#include <errno.h>
#include <bbu.h>
#include <fs.h>

static void print_handlers_list(void)
{
	printf("registered update handlers:\n");
	bbu_handlers_list();
}

static int do_barebox_update(int argc, char *argv[])
{
	int opt, ret, repair = 0;
	struct bbu_data data = {};
	struct bbu_handler *handler;
	void *image = NULL;
	const char *name;
	const char *fmt;

	while ((opt = getopt(argc, argv, "t:yf:ld:r")) > 0) {
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
			print_handlers_list();
			return 0;
		case 'r':
			repair = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (data.handler_name && data.devicefile) {
		printf("Both TARGET and DEVICE are provided. "
		       "Ignoring the latter\n");

		data.devicefile = NULL;
	}

	if (data.handler_name) {
		handler = bbu_find_handler_by_name(data.handler_name);
		fmt = "handler '%s' does not exist\n";
		name = data.handler_name;
	} else if (data.devicefile) {
		handler = bbu_find_handler_by_device(data.devicefile);
		fmt = "handler for '%s' does not exist\n";
		name = data.devicefile;
	} else {
		handler = bbu_find_handler_by_name(NULL);
		fmt = "default handler does not exist\n";
		name = NULL;
	}

	if (!handler) {
		printf(fmt, name);
		print_handlers_list();
		return COMMAND_ERROR;
	}

	if (argc - optind > 0) {
		data.imagefile = argv[optind];

		image = read_file(data.imagefile, &data.len);
		if (!image)
			return -errno;
		data.image = image;
	} else {
		if (!repair)
			return COMMAND_ERROR_USAGE;
	}

	ret = barebox_update(&data, handler);

	free(image);

	return ret;
}

BAREBOX_CMD_HELP_START(barebox_update)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-l\t", "list registered targets")
BAREBOX_CMD_HELP_OPT("-t TARGET", "specify data target handler name")
BAREBOX_CMD_HELP_OPT("-d DEVICE", "write image to DEVICE")
BAREBOX_CMD_HELP_OPT("-r\t", "refresh or repair. Do not update, but repair an existing image")
BAREBOX_CMD_HELP_OPT("-y\t", "autom. use 'yes' when asking confirmations")
BAREBOX_CMD_HELP_OPT("-f LEVEL", "set force level")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(barebox_update)
	.cmd		= do_barebox_update,
	BAREBOX_CMD_DESC("update barebox to persistent media")
	BAREBOX_CMD_OPTS("[-ltdyfr] [IMAGE]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_barebox_update_help)
BAREBOX_CMD_END
