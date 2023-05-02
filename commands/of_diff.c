// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2019 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* of_diff.c - compare device tree files */

#include <common.h>
#include <fs.h>
#include <libfile.h>
#include <of.h>
#include <command.h>
#include <malloc.h>
#include <complete.h>
#include <errno.h>
#include <linux/err.h>

static struct device_node *get_tree(const char *filename, struct device_node *root)
{
	struct device_node *node;

	if (!strcmp(filename, "-")) {
		node = of_get_root_node();
		if (!node)
			return ERR_PTR(-ENOENT);

		return of_dup(node);
	}

	if (!strcmp(filename, "+")) {
		node = of_get_root_node();
		if (!node)
			return ERR_PTR(-ENOENT);

		node = of_dup(root);

		of_fix_tree(node);

		return node;
	}

	return of_read_file(filename);
}

static int do_of_diff(int argc, char *argv[])
{
	int ret = 0;
	struct device_node *a, *b, *root;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	root = of_get_root_node();
	a = get_tree(argv[1], root);
	b = get_tree(argv[2], root);

	if (IS_ERR(a)) {
		printf("Cannot read %s: %pe\n", argv[1], a);
		ret = COMMAND_ERROR;
		a = NULL;
		goto out;
	}

	if (IS_ERR(b)) {
		printf("Cannot read %s: %pe\n", argv[2], b);
		ret = COMMAND_ERROR;
		b = NULL;
		goto out;
	}

	ret = of_diff(a, b, 0) ? COMMAND_ERROR : COMMAND_SUCCESS;
out:
	if (a && a != root)
		of_delete_node(a);
	if (b && b != root)
		of_delete_node(b);

	return ret;
}

BAREBOX_CMD_HELP_START(of_diff)
BAREBOX_CMD_HELP_TEXT("This command prints a diff between two given device trees.")
BAREBOX_CMD_HELP_TEXT("The device trees are given as dtb files or:")
BAREBOX_CMD_HELP_TEXT("'-' to compare against the barebox live tree, or")
BAREBOX_CMD_HELP_TEXT("'+' to compare against the fixed barebox live tree")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_diff)
	.cmd		= do_of_diff,
	BAREBOX_CMD_DESC("diff device trees")
	BAREBOX_CMD_OPTS("<a> <b>")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_of_diff_help)
BAREBOX_CMD_END
