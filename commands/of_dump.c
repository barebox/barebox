/*
 * of_dump.c - dump devicetrees to the console
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <complete.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <linux/err.h>

static int do_of_dump(int argc, char *argv[])
{
	int opt;
	int ret;
	struct device_node *root = NULL, *node, *of_free = NULL;
	char *dtbfile = NULL;
	size_t size;
	const char *nodename;

	while ((opt = getopt(argc, argv, "f:")) > 0) {
		switch (opt) {
		case 'f':
			dtbfile = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc)
		nodename = "/";
	else
		nodename = argv[optind];

	if (dtbfile) {
		void *fdt;

		fdt = read_file(dtbfile, &size);
		if (!fdt) {
			printf("unable to read %s: %s\n", dtbfile, strerror(errno));
			return -errno;
		}

		root = of_unflatten_dtb(fdt);

		free(fdt);

		if (IS_ERR(root)) {
			ret = PTR_ERR(root);
			goto out;
		}

		of_free = root;
	} else {
		root = of_get_root_node();
	}

	node = of_find_node_by_path_or_alias(root, nodename);
	if (!node) {
		printf("Cannot find nodepath %s\n", nodename);
		ret = -ENOENT;
		goto out;
	}

	of_print_nodes(node, 0);

out:
	if (of_free)
		of_delete_node(of_free);

	return 0;
}

BAREBOX_CMD_HELP_START(of_dump)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT  ("-f dtb",  "work on dtb instead of internal devicetree\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_dump)
	.cmd		= do_of_dump,
	BAREBOX_CMD_DESC("dump devicetree nodes")
	BAREBOX_CMD_OPTS("[-f]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_file_complete)
	BAREBOX_CMD_HELP(cmd_of_dump_help)
BAREBOX_CMD_END
