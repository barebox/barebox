/*
 * of_display_timings.c - list and select display_timings
 *
 * Copyright (c) 2014 Teresa Gámez <t.gamez@phytec.de> PHYTEC Messtechnik GmbH
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
 */

#include <common.h>
#include <filetype.h>
#include <libfile.h>
#include <of.h>
#include <command.h>
#include <malloc.h>
#include <complete.h>
#include <asm/byteorder.h>
#include <getopt.h>
#include <linux/err.h>
#include <string.h>

static int of_display_timing(struct device_node *root, void *timingpath)
{
	int ret = 0;
	struct device_node *display;

	display = of_find_node_by_path_from(root, timingpath);
	if (!display) {
		pr_err("Path to display-timings is not vaild.\n");
		ret = -EINVAL;
		goto out;
	}

	ret = of_set_property_to_child_phandle(display, "native-mode");
	if (ret)
		pr_err("Could not set display\n");
out:
	return ret;
}

static int do_of_display_timings(int argc, char *argv[])
{
	int opt;
	int list = 0;
	int selected = 0;
	struct device_node *root = NULL;
	struct device_node *display = NULL;
	struct device_node *timings = NULL;
	char *timingpath = NULL;
	char *dtbfile = NULL;

	while ((opt = getopt(argc, argv, "sS:lf:")) > 0) {
		switch (opt) {
		case 'l':
			list = 1;
			break;
		case 'f':
			dtbfile = optarg;
			break;
		case 's':
			selected = 1;
			break;
		case 'S':
			timingpath = xzalloc(strlen(optarg) + 1);
			strcpy(timingpath, optarg);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	/* Check if external dtb given */
	if (dtbfile) {
		void *fdt;
		size_t size;

		fdt = read_file(dtbfile, &size);
		if (!fdt) {
			pr_err("unable to read %s: %s\n", dtbfile,
				strerror(errno));
			return -errno;
		}

		if (file_detect_type(fdt, size) != filetype_oftree) {
			pr_err("%s is not a oftree file.\n", dtbfile);
			free(fdt);
			return -EINVAL;
		}

		root = of_unflatten_dtb(fdt);

		free(fdt);

		if (IS_ERR(root))
			return PTR_ERR(root);

	} else {
		root = of_get_root_node();
	}

	if (list) {
		int found = 0;
		const char *node = "display-timings";

		for_each_node_by_name_from(display, root, node) {
			for_each_child_of_node(display, timings) {
				printf("%s\n", timings->full_name);
				found = 1;
			}
		}

		if (!found)
			printf("No display-timings found.\n");
	}

	if (selected) {
		int found = 0;
		const char *node = "display-timings";

		for_each_node_by_name_from(display, root, node) {
			timings = of_parse_phandle_from(display, root,
							"native-mode", 0);
			if (!timings)
				continue;

			printf("%s\n", timings->full_name);
			found = 1;
		}

		if (!found)
			printf("No selected display-timings found.\n");
	}

	if (timingpath)
		of_register_fixup(of_display_timing, timingpath);

	return 0;
}

BAREBOX_CMD_HELP_START(of_display_timings)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-l",  "list path of all available display-timings\n")
BAREBOX_CMD_HELP_OPT("-s",  "list path of all selected display-timings\n")
BAREBOX_CMD_HELP_OPT("-S path",  "select display-timings and register oftree fixup\n")
BAREBOX_CMD_HELP_OPT("-f dtb",  "work on dtb. Has no effect on -s option\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_display_timings)
	.cmd	= do_of_display_timings,
	BAREBOX_CMD_DESC("print and select display-timings")
	BAREBOX_CMD_OPTS("[-ls] [-S path] [-f dtb]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_file_complete)
	BAREBOX_CMD_HELP(cmd_of_display_timings_help)
BAREBOX_CMD_END
