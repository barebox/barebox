// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Michael Tretter <m.tretter@pengutronix.de>, Pengutronix
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

#include <command.h>
#include <common.h>
#include <environment.h>
#include <fdt.h>
#include <firmware.h>
#include <fs.h>
#include <getopt.h>
#include <libfile.h>
#include <of.h>

static int do_of_overlay(int argc, char *argv[])
{
	int opt, ret;
	struct fdt_header *fdt;
	struct device_node *overlay;
	const char *search_path = NULL;

	while ((opt = getopt(argc, argv, "S:")) > 0) {
		switch (opt) {
		case 'S':
			search_path = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc != optind + 1)
		return COMMAND_ERROR_USAGE;

	fdt = read_file(argv[optind], NULL);
	if (!fdt) {
		printf("cannot read %s\n", argv[optind]);
		return 1;
	}

	overlay = of_unflatten_dtb(fdt);
	free(fdt);
	if (IS_ERR(overlay))
		return PTR_ERR(overlay);

	if (search_path) {
		ret = of_firmware_load_overlay(overlay, search_path);
		if (ret)
			goto err;
	}

	ret = of_register_overlay(overlay);
	if (ret) {
		printf("cannot apply oftree overlay: %s\n", strerror(-ret));
		goto err;
	}

	return 0;

err:
	of_delete_node(overlay);
	return ret;
}

BAREBOX_CMD_HELP_START(of_overlay)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-S path", "load firmware using this search path")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_overlay)
	.cmd = do_of_overlay,
	BAREBOX_CMD_DESC("register device tree overlay as fixup")
	BAREBOX_CMD_OPTS("[-S path] FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_of_overlay_help)
BAREBOX_CMD_END
