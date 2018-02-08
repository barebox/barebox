/*
 * of_property.c - device tree property handling support
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
#include <complete.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <getopt.h>
#include <init.h>

static int of_parse_prop_cells(char * const *newval, int count, char *data, int *len)
{
	char *cp;
	unsigned long tmp;	/* holds converted values */
	int stridx = 0;
	char *newp = newval[0];

	newp++;

	while (1) {
		if (*newp == '>')
			return 0;

		if (*newp == '\0') {
			newp = newval[++stridx];

			if (stridx == count) {
				printf("missing '>'\n");
				return -EINVAL;
			}

			continue;
		}

		cp = newp;

		if (isdigit(*cp)) {
			tmp = simple_strtoul(cp, &newp, 0);
		} else {
			struct device_node *n;
			char *str;
			int len = 0;

			str = cp;
			while (*str && *str != '>' && *str != ' ') {
				str++;
				len++;
			}

			str = xzalloc(len + 1);
			strncpy(str, cp, len);

			n = of_find_node_by_path_or_alias(NULL, str);
			if (!n)
				printf("Cannot find node '%s'\n", str);

			free(str);

			if (!n)
				return -EINVAL;

			tmp = of_node_create_phandle(n);
			newp += len;
		}

		*(__be32 *)data = __cpu_to_be32(tmp);
		data  += 4;
		*len += 4;

		/* If the ptr didn't advance, something went wrong */
		if ((newp - cp) <= 0) {
			printf("cannot convert \"%s\"\n", cp);
			return -EINVAL;
		}

		while (*newp == ' ')
			newp++;
	}
}

static int of_parse_prop_stream(char * const *newval, int count, char *data, int *len)
{
	char *cp;
	unsigned long tmp;	/* holds converted values */
	int stridx = 0;
	char *newp = newval[0];

	newp++;

	while (1) {
		if (*newp == ']')
			return 0;

		while (*newp == ' ')
			newp++;

		if (*newp == '\0') {
			newp = newval[++stridx];

			if (stridx == count) {
				printf("missing ']'\n");
				return -EINVAL;
			}

			continue;
		}

		cp = newp;
		tmp = simple_strtoul(newp, &newp, 16);
		*data++ = tmp & 0xff;
		*len    = *len + 1;

		/* If the ptr didn't advance, something went wrong */
		if ((newp - cp) <= 0) {
			printf("cannot convert \"%s\"\n", cp);
			return -EINVAL;
		}
	}
}

static int of_parse_prop_string(char * const *newval, int count, char *data, int *len)
{
	int stridx = 0;
	char *newp = newval[0];

	/*
	 * Assume it is one or more strings.  Copy it into our
	 * data area for convenience (including the
	 * terminating '\0's).
	 */
	while (stridx < count) {
		size_t length = strlen(newp) + 1;

		strcpy(data, newp);
		data += length;
		*len += length;
		newp = newval[++stridx];
	}

	return 0;
}

/*
 * Parse the user's input, partially heuristic.  Valid formats:
 * <0x00112233 4 05>	- an array of cells.  Numbers follow standard
 *			C conventions.
 * [00 11 22 .. nn] - byte stream
 * "string"	- If the the value doesn't start with "<" or "[", it is
 *			treated as a string.  Note that the quotes are
 *			stripped by the parser before we get the string.
 * newval: An array of strings containing the new property as specified
 *	on the command line
 * count: The number of strings in the array
 * data: A bytestream to be placed in the property
 * len: The length of the resulting bytestream
 */
static int of_parse_prop(char * const *newval, int count, char *data, int *len)
{
	char *newp;		/* temporary newval char pointer */

	*len = 0;

	if (!count)
		return 0;

	newp = newval[0];

	switch (*newp) {
	case '<':
		return of_parse_prop_cells(newval, count, data, len);
	case '[':
		return of_parse_prop_stream(newval, count, data, len);
	default:
		return of_parse_prop_string(newval, count, data, len);
	}
}

static int do_of_property(int argc, char *argv[])
{
	int opt;
	int delete = 0;
	int set = 0;
	int ret;
	char *path = NULL, *propname = NULL;
	struct device_node *node = NULL;
	struct property *pp = NULL;

	while ((opt = getopt(argc, argv, "ds")) > 0) {
		switch (opt) {
		case 'd':
			delete = 1;
			break;
		case 's':
			set = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	if (optind < argc) {
		path = argv[optind];
		node = of_find_node_by_path_or_alias(NULL, path);
		if (!node) {
			printf("Cannot find nodepath %s\n", path);
			return -ENOENT;
		}
	}

	if (optind + 1 < argc) {
		propname = argv[optind + 1];

		pp = of_find_property(node, propname, NULL);
		if (!set && !pp) {
			printf("Cannot find property %s\n", propname);
			return -ENOENT;
		}
	}

	debug("path: %s propname: %s\n", path, propname);

	if (delete) {
		if (!node || !pp)
			return COMMAND_ERROR_USAGE;

		of_delete_property(pp);

		return 0;
	}

	if (set) {
		int num_args = argc - optind - 2;
		int len;
		void *data;

		if (!node)
			return COMMAND_ERROR_USAGE;

		/*
		 * standard console buffer size. The result won't be bigger than the
		 * string input.
		 */
		data = malloc(1024);
		if (!data)
			return -ENOMEM;

		ret = of_parse_prop(&argv[optind + 2], num_args, data, &len);
		if (ret) {
			free(data);
			return ret;
		}

		if (pp) {
			free(pp->value);
			pp->value_const = NULL;

			/* limit property data to the actual size */
			if (len) {
				pp->value = xrealloc(data, len);
			} else {
				pp->value = NULL;
				free(data);
			}

			pp->length = len;
		} else {
			pp = of_new_property(node, propname, data, len);
			if (!pp) {
				printf("Cannot create property %s\n", propname);
				free(data);
				return 1;
			}
		}
	}

	return 0;
}

BAREBOX_CMD_HELP_START(of_property)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-s",  "set property to value")
BAREBOX_CMD_HELP_OPT ("-d",  "delete property")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Valid formats for values:")
BAREBOX_CMD_HELP_TEXT("<0x00112233 4 05> - an array of cells. cells not beginning with a digit are")
BAREBOX_CMD_HELP_TEXT("                    interpreted as node paths and converted to phandles")
BAREBOX_CMD_HELP_TEXT("[00 11 22 .. nn]  - byte stream")
BAREBOX_CMD_HELP_TEXT("If the value does not start with '<' or '[' it is interpreted as string")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(of_property)
	.cmd		= do_of_property,
	BAREBOX_CMD_DESC("handle device tree properties")
	BAREBOX_CMD_OPTS("[-sd] NODE [PROPERTY] [VALUES]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_COMPLETE(devicetree_complete)
	BAREBOX_CMD_HELP(cmd_of_property_help)
BAREBOX_CMD_END
