// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* of_dump.c - dump devicetrees to the console */

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

	list_for_each_entry(n, &node->children, parent_list) {
		if (ctrlc())
			return;
		of_print_nodenames(n);
	}
}

static int do_of_dump(int argc, char *argv[])
{
	int opt;
	int ret = 0;
	int fix = 0;
	struct device_node *root = NULL, *node, *of_free = NULL;
	char *dtbfile = NULL;
	const char *nodename;
	unsigned maxpropsize = ~0;
	int names_only = 0, properties_only = 0;

	while ((opt = getopt(argc, argv, "Ff:npP:")) > 0) {
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
		case 'p':
			properties_only = 1;
			break;
		case 'P':
			ret = kstrtouint(optarg, 0, &maxpropsize);
			if (ret)
				return ret;
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

	if (fix)
		of_fix_tree(root);

	node = of_find_node_by_path_or_alias(root, nodename);
	if (!node) {
		printf("Cannot find nodepath %s\n", nodename);
		ret = -ENOENT;
		goto out;
	}

	if (names_only && !properties_only)
		of_print_nodenames(node);
	else if (properties_only && !names_only)
		of_print_properties(node, maxpropsize);
	else
		of_print_nodes(node, 0, maxpropsize);

out:
	of_delete_node(of_free);

	return ret;
}

BAREBOX_CMD_HELP_START(of_dump)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT  ("-f dtb",  "work on dtb instead of internal devicetree")
BAREBOX_CMD_HELP_OPT  ("-F",  "return fixed devicetree")
BAREBOX_CMD_HELP_OPT  ("-n",  "Print node names only, no properties")
BAREBOX_CMD_HELP_OPT  ("-p",  "Print properties only, no child nodes")
BAREBOX_CMD_HELP_OPT  ("-P len",  "print only len property bytes")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_dump)
	.cmd		= do_of_dump,
	BAREBOX_CMD_DESC("dump devicetree nodes")
	BAREBOX_CMD_OPTS("[-fFnpP] [NODE]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_file_complete)
	BAREBOX_CMD_HELP(cmd_of_dump_help)
BAREBOX_CMD_END
