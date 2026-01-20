// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* barebox-update.c - update barebox */

#include <common.h>
#include <command.h>
#include <libfile.h>
#include <globalvar.h>
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
	char *pathbuf = NULL;
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
	} else if (!repair) {
		pathbuf = xasprintf("%s/%s-barebox-%s", globalvar_get("net.fetchdir"),
				    globalvar_get("user"), globalvar_get("hostname"));
		data.imagefile = pathbuf;
	}

	if (data.imagefile) {
		image = read_file(data.imagefile, &data.len);
		if (!image) {
			ret = -errno;
			goto out;
		}
		data.image = image;
	}

	ret = barebox_update(&data, handler);

	free(image);
out:
	free(pathbuf);

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
