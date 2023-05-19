// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2023 Ahmad Fatoum <a.fatoum@pengutronix.de>

#include <common.h>
#include <libfile.h>
#include <fdt.h>
#include <of.h>
#include <command.h>
#include <complete.h>
#include <errno.h>
#include <getopt.h>

static int do_of_compatible(int argc, char *argv[])
{
	int opt;
	int ret = 0;
	bool fix = false, kernel_compat = false;
	struct device_node *root = NULL, *node, *of_free = NULL;
	char **compats, **compat, *dtbfile = NULL;
	const char *nodename = "/";

	while ((opt = getopt(argc, argv, "f:n:Fk")) > 0) {
		switch (opt) {
		case 'f':
			dtbfile = optarg;
			break;
		case 'n':
			nodename = optarg;
			break;
		case 'F':
			fix = true;
			break;
		case 'k':
			kernel_compat = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc - optind < 1)
		return COMMAND_ERROR_USAGE;

	compats = &argv[optind];

	if (dtbfile) {
		root = of_read_file(dtbfile);
		if (IS_ERR(root))
			return PTR_ERR(root);

		of_free = root;
	} else {
		root = of_get_root_node();

		/* copy internal device tree to apply fixups onto it */
		if (fix)
			root = of_free = of_dup(root);
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

	ret = COMMAND_ERROR;

	if (kernel_compat) {
		const char *compat_override;

		if (node->parent) {
			printf("-k only valid for root node\n");
			ret = COMMAND_ERROR_USAGE;
			goto out;
		}

		compat_override = barebox_get_of_machine_compatible() ?: "";
		for (compat = compats; *compat; compat++) {
			if (strcmp(*compat, compat_override) == 0) {
				ret = COMMAND_SUCCESS;
				goto out;
			}
		}
	}

	for (compat = compats; *compat; compat++) {
		int score;

		score = of_device_is_compatible(node, *compat);
		if (score > 0) {
			ret = COMMAND_SUCCESS;
			break;
		}
	}

out:
	of_delete_node(of_free);

	return ret;
}

BAREBOX_CMD_HELP_START(of_compatible)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT  ("-f dtb",  "work on dtb instead of internal devicetree")
BAREBOX_CMD_HELP_OPT  ("-F",      "apply fixups on devicetree before compare")
BAREBOX_CMD_HELP_OPT  ("-n node", "node path or alias to compare its compatible (default is /)")
BAREBOX_CMD_HELP_OPT  ("-k",      "compare $global.of.kernel.add_machine_compatible as well")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_compatible)
	.cmd		= do_of_compatible,
	BAREBOX_CMD_DESC("Check DT node's compatible")
	BAREBOX_CMD_OPTS("[-fFnk] [COMPATS..]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(empty_complete)
	BAREBOX_CMD_HELP(cmd_of_compatible_help)
BAREBOX_CMD_END
