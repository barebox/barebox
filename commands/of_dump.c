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
#include <libfile.h>
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

static void of_print_nodenames(struct device_node *node)
{
	struct device_node *n;

	printf("%s\n", node->full_name);

	list_for_each_entry(n, &node->children, parent_list)
		of_print_nodenames(n);
}

static int do_of_dump(int argc, char *argv[])
{
	int opt;
	int ret = 0;
	int fix = 0;
	struct device_node *root = NULL, *node, *of_free = NULL;
	char *dtbfile = NULL;
	size_t size;
	const char *nodename;
	int names_only = 0;

	while ((opt = getopt(argc, argv, "Ff:n")) > 0) {
		switch (opt) {
		case 'f':
			dtbfile = optarg;
			break;
		case 'F':
			fix = 1;
			break;
		case 'n':
			names_only = 1;
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

		if (fix) {
			/* create a copy of internal devicetree */
			void *fdt;
			fdt = of_flatten_dtb(root);
			root = of_unflatten_dtb(fdt);

			free(fdt);

			if (IS_ERR(root)) {
				ret = PTR_ERR(root);
				goto out;
			}

			of_free = root;
		}
	}

	if (fix) {
		ret = of_fix_tree(root);
		if (ret)
			goto out;
	}

	node = of_find_node_by_path_or_alias(root, nodename);
	if (!node) {
		printf("Cannot find nodepath %s\n", nodename);
		ret = -ENOENT;
		goto out;
	}

	if (names_only)
		of_print_nodenames(node);
	else
		of_print_nodes(node, 0);

out:
	if (of_free)
		of_delete_node(of_free);

	return ret;
}

BAREBOX_CMD_HELP_START(of_dump)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT  ("-f dtb",  "work on dtb instead of internal devicetree\n")
BAREBOX_CMD_HELP_OPT  ("-F",  "return fixed devicetree\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_dump)
	.cmd		= do_of_dump,
	BAREBOX_CMD_DESC("dump devicetree nodes")
	BAREBOX_CMD_OPTS("[-fF] [NODE]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_file_complete)
	BAREBOX_CMD_HELP(cmd_of_dump_help)
BAREBOX_CMD_END
