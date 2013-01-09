/*
 * of_node.c - device tree node handling support
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <environment.h>
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <libfdt.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>
#include <libgen.h>

static int do_of_node(int argc, char *argv[])
{
	int opt;
	int delete = 0;
	int create = 0;
	char *path = NULL;
	struct device_node *node = NULL;

	while ((opt = getopt(argc, argv, "cd")) > 0) {
		switch (opt) {
		case 'c':
			create = 1;
			break;
		case 'd':
			delete = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind < argc) {
		path = argv[optind];
	}

	if (create) {
		char *name;

		if (!path)
			return COMMAND_ERROR_USAGE;

		name = xstrdup(basename(path));
		path = dirname(path);

		node = of_find_node_by_path(path);
		if (!node) {
			printf("Cannot find nodepath %s\n", path);
			free(name);
			return -ENOENT;
		}

		debug("create node \"%s\" \"%s\"\n", path, name);

		of_new_node(node, name);

		free(name);

		return 0;
	}

	if (delete) {
		if (!path)
			return COMMAND_ERROR_USAGE;

		node = of_find_node_by_path(path);
		if (!node) {
			printf("Cannot find nodepath %s\n", path);
			return -ENOENT;
		}

		of_free(node);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(of_node)
BAREBOX_CMD_HELP_USAGE("of_node [OPTIONS] [NODE] [NAME]\n")
BAREBOX_CMD_HELP_OPT  ("-c",  "create a new node\n")
BAREBOX_CMD_HELP_OPT  ("-d",  "delete a node\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_node)
	.cmd		= do_of_node,
	.usage		= "handle of nodes",
	BAREBOX_CMD_HELP(cmd_of_node_help)
BAREBOX_CMD_END
