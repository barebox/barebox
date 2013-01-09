/*
 * oftree.c - device tree command support
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on U-Boot code by:
 *
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 * Pantelis Antoniou <pantelis.antoniou@gmail.com> and
 * Matthew McClintock <msm@freescale.com>
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

static int do_oftree(int argc, char *argv[])
{
	struct fdt_header *fdt = NULL;
	void *fdt_free = NULL;
	int size;
	int opt;
	char *file = NULL;
	const char *node = "/";
	int dump = 0;
	int probe = 0;
	int load = 0;
	int free_of = 0;
	int ret;

	while ((opt = getopt(argc, argv, "dpfn:l")) > 0) {
		switch (opt) {
		case 'l':
			load = 1;
			break;
		case 'd':
			dump = 1;
			break;
		case 'p':
			if (IS_ENABLED(CONFIG_CMD_OFTREE_PROBE)) {
				probe = 1;
			} else {
				printf("oftree device probe support disabled\n");
				return COMMAND_ERROR_USAGE;
			}
			break;
		case 'f':
			free_of = 1;
			break;
		case 'n':
			node = optarg;
			break;
		}
	}

	if (free_of) {
		struct device_node *root = of_get_root_node();

		if (root)
			of_free(root);

		return 0;
	}

	if (optind < argc)
		file = argv[optind];

	if (!dump && !probe && !load)
		return COMMAND_ERROR_USAGE;

	if (file) {
		fdt = read_file(file, &size);
		if (!fdt) {
			printf("unable to read %s\n", file);
			return 1;
		}

		fdt_free = fdt;
	}

	if (load) {
		if (!fdt) {
			printf("no fdt given\n");
			ret = -ENOENT;

			goto out;
		}

		ret = of_unflatten_dtb(fdt);
		if (ret) {
			printf("parse oftree: %s\n", strerror(-ret));
			goto out;
		}
	}

	if (dump) {
		if (fdt) {
			ret = fdt_print(fdt, node);
		} else {
			struct device_node *n = of_find_node_by_path(node);

			if (!n) {
				ret = -ENOENT;
				goto out;
			}

			of_print_nodes(n, 0);

			ret = 0;
		}

		goto out;
	}

	if (probe) {
		ret = of_probe();
		if (ret)
			goto out;
	}

	ret = 0;
out:
	free(fdt_free);

	return ret;
}

BAREBOX_CMD_HELP_START(oftree)
BAREBOX_CMD_HELP_USAGE("oftree [OPTIONS] [DTB]\n")
BAREBOX_CMD_HELP_OPT  ("-l",  "Load [DTB] to internal devicetree\n")
BAREBOX_CMD_HELP_OPT  ("-p",  "probe devices from stored devicetree\n")
BAREBOX_CMD_HELP_OPT  ("-d",  "dump oftree from [DTB] or the parsed tree if no dtb is given\n")
BAREBOX_CMD_HELP_OPT  ("-f",  "free stored devicetree\n")
BAREBOX_CMD_HELP_OPT  ("-n <node>",  "specify root devicenode to dump for -d\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(oftree)
	.cmd		= do_oftree,
	.usage		= "handle devicetrees",
	BAREBOX_CMD_HELP(cmd_oftree_help)
BAREBOX_CMD_END
