/*
 * (C) Copyright 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 Only
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <filetype.h>
#include <environment.h>
#include <magicvar.h>
#include <getopt.h>
#include <linux/stat.h>
#include <fs.h>

static int do_filetype(int argc, char *argv[])
{
	int opt;
	enum filetype type;
	char *filename = NULL;
	int verbose = -1, list = 0;
	const char *varname = NULL;
	struct stat s;
	int ret;

	while ((opt = getopt(argc, argv, "vls:")) > 0) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 'l':
			list = 1;
			break;
		case 's':
			varname = optarg;
			/* in scripting mode default to nonverbose */
			if (verbose < 0)
				verbose = 0;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (verbose < 0)
		verbose = 1;

	if (list) {
		int i;

		printf("known filetypes:\n");

		for (i = 1; i < filetype_max; i++)
			printf("%-16s: %s\n", file_type_to_short_string(i),
					file_type_to_string(i));
		return 0;
	}

	if (argc - optind < 1)
		return COMMAND_ERROR_USAGE;

	filename = argv[optind];

	ret = stat(filename, &s);
	if (ret)
		return ret;

	if (S_ISDIR(s.st_mode))
		return -EISDIR;

	type = file_name_detect_type(filename);

	if (verbose)
		printf("%s: %s (%s)\n", filename,
				file_type_to_string(type),
				file_type_to_short_string(type));

	if (varname)
		setenv(varname, file_type_to_short_string(type));

	return 0;
}

BAREBOX_CMD_HELP_START(filetype)
BAREBOX_CMD_HELP_USAGE("filetype [OPTIONS] <file>\n")
BAREBOX_CMD_HELP_SHORT("detect type of a file and export result to a variable\n")
BAREBOX_CMD_HELP_OPT("-v", "verbose\n")
BAREBOX_CMD_HELP_OPT("-s <v>", "set <v> to shortname\n")
BAREBOX_CMD_HELP_OPT("-l", "list known filetypes\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(filetype)
	.cmd		= do_filetype,
	.usage		= "detect file type",
	BAREBOX_CMD_HELP(cmd_filetype_help)
BAREBOX_CMD_END
