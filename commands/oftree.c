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
#include <libfile.h>
#include <of.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>
#include <fcntl.h>
#include <complete.h>

static int do_oftree(int argc, char *argv[])
{
	struct fdt_header *fdt = NULL;
	size_t size;
	int opt;
	int probe = 0;
	char *load = NULL;
	char *save = NULL;
	int ret;
	struct device_node *root;

	while ((opt = getopt(argc, argv, "pfl:s:")) > 0) {
		switch (opt) {
		case 'l':
			load = optarg;
			break;
		case 'p':
			if (IS_ENABLED(CONFIG_OFDEVICE)) {
				probe = 1;
			} else {
				printf("oftree device probe support disabled\n");
				return COMMAND_ERROR_USAGE;
			}
			break;
		case 's':
			save = optarg;
			break;
		}
	}

	if (!probe && !load && !save)
		return COMMAND_ERROR_USAGE;

	if (save) {
		fdt = of_get_fixed_tree(NULL);
		if (!fdt) {
			printf("no devicetree available\n");
			ret = -EINVAL;

			goto out;
		}

		ret = write_file(save, fdt, fdt32_to_cpu(fdt->totalsize));

		goto out;
	}

	if (load) {
		fdt = read_file(load, &size);
		if (!fdt) {
			printf("unable to read %s\n", load);
			return 1;
		}

		root = of_unflatten_dtb(fdt);

		free(fdt);

		if (IS_ERR(root))
			return PTR_ERR(root);

		ret = of_set_root_node(root);
		if (ret) {
			printf("setting root node failed: %s\n", strerror(-ret));
			of_delete_node(root);
			goto out;
		}
	}

	if (probe) {
		ret = of_probe();
		if (ret)
			goto out;
	}

	ret = 0;
out:

	return ret;
}

BAREBOX_CMD_HELP_START(oftree)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l <DTB>",  "Load <DTB> to internal devicetree\n")
BAREBOX_CMD_HELP_OPT ("-s <DTB>",  "save internal devicetree to <DTB>\n")
BAREBOX_CMD_HELP_OPT ("-p",  "probe devices from stored device tree")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(oftree)
	.cmd		= do_oftree,
	BAREBOX_CMD_DESC("handle device trees")
	BAREBOX_CMD_OPTS("[-lsp]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_oftree_help)
BAREBOX_CMD_END
