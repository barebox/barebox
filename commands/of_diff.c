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
		node = of_dup(root) ?: ERR_PTR(-ENOENT);
	} else if (!strcmp(filename, "+")) {
		return NULL;
	} else {
		node = of_read_file(filename);
	}

	if (IS_ERR(node))
		printf("Cannot read %s: %pe\n", filename, node);

	return node;
}

static struct device_node *get_tree_fixed(const struct device_node *root)
{
	struct device_node *node;

	node = of_dup(root);
	if (!IS_ERR(node))
		of_fix_tree(node);

	return node;
}

static int do_of_diff(int argc, char *argv[])
{
	int ret = COMMAND_ERROR;
	struct device_node *a, *b, *root;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	root = of_get_root_node();
	a = get_tree(argv[1], root);
	b = get_tree(argv[2], root);

	if (!a && !b)
		return COMMAND_ERROR_USAGE;

	if (!a)
		a = get_tree_fixed(b);
	if (!b)
		b = get_tree_fixed(a);

	if (!IS_ERR(a) && !IS_ERR(b))
		ret = of_diff(a, b, 0) ? COMMAND_ERROR : COMMAND_SUCCESS;

	if (!IS_ERR(a) && a != root)
		of_delete_node(a);
	if (!IS_ERR(b) && b != root)
		of_delete_node(b);

	return ret;
}

BAREBOX_CMD_HELP_START(of_diff)
BAREBOX_CMD_HELP_TEXT("This command prints a diff between two given device trees.")
BAREBOX_CMD_HELP_TEXT("The device trees are given as dtb files or:")
BAREBOX_CMD_HELP_TEXT("'-' to compare against the barebox live tree, or")
BAREBOX_CMD_HELP_TEXT("'+' to compare against the other device tree after fixups")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_diff)
	.cmd		= do_of_diff,
	BAREBOX_CMD_DESC("diff device trees")
	BAREBOX_CMD_OPTS("<a> <b>")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_of_diff_help)
BAREBOX_CMD_END
