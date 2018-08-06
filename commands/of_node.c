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
#include <complete.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>
#include <libgen.h>

static int do_of_node_create_now(struct device_node *root, const char *path);
static int do_of_node_delete_now(struct device_node *root, const char *path);

static int of_fixup_node_create(struct device_node *root, void *context)
{
	return do_of_node_create_now(root, (const char *)context);
}

static int of_fixup_node_delete(struct device_node *root, void *context)
{
	return do_of_node_delete_now(root, (const char *)context);
}

static int do_of_node_create_fixup(const char *path)
{
	char *data = xstrdup(path);

	return of_register_fixup(of_fixup_node_create, (void *)data);
}

static int do_of_node_delete_fixup(const char *path)
{
	char *data = xstrdup(path);

	return of_register_fixup(of_fixup_node_delete, (void *)data);
}

static int do_of_node_create_now(struct device_node *root, const char *path)
{
	struct device_node *node = of_create_node(root, path);

	if (!node)
		return -EINVAL;

	return 0;
}

static int do_of_node_delete_now(struct device_node *root, const char *path)
{
	struct device_node *node = of_find_node_by_path(path);

	if (!node) {
		printf("Cannot find nodepath %s\n", path);
		return -ENOENT;
	}

	of_delete_node(node);

	return 0;
}

static int do_of_node(int argc, char *argv[])
{
	int opt;
	int delete = 0;
	int create = 0;
	int fixup = 0;
	char *path = NULL;

	while ((opt = getopt(argc, argv, "cdf")) > 0) {
		switch (opt) {
		case 'c':
			create = 1;
			break;
		case 'd':
			delete = 1;
			break;
		case 'f':
			fixup = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind + 1 != argc)
		return COMMAND_ERROR_USAGE;

	path = argv[optind];

	if (!path)
		return COMMAND_ERROR_USAGE;

	if (fixup) {
		if (create)
			return do_of_node_create_fixup(path);
		if (delete)
			return do_of_node_delete_fixup(path);
	} else {
		struct device_node *root = of_get_root_node();
		if (!root) {
			printf("root node not set\n");
			return -ENOENT;
		}

		if (create)
			return do_of_node_create_now(root, path);
		if (delete)
			return do_of_node_delete_now(root, path);
	}

	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_HELP_START(of_node)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c",  "create a new node")
BAREBOX_CMD_HELP_OPT ("-d",  "delete a node")
BAREBOX_CMD_HELP_OPT ("-f",  "create/delete as a fixup (defer the action until booting)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_node)
	.cmd		= do_of_node,
	BAREBOX_CMD_DESC("create/delete nodes in the device tree")
	BAREBOX_CMD_OPTS("[-cd] [-f] NODEPATH")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_complete)
	BAREBOX_CMD_HELP(cmd_of_node_help)
BAREBOX_CMD_END
