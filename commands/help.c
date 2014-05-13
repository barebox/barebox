/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <complete.h>


static void list_group(int verbose, const char *grpname, uint32_t group)
{
	struct command *cmdtp;
	bool first = true;
	int pos = 0;
	int len;

	for_each_command(cmdtp) {
		if (cmdtp->group != group)
			continue;
		if (first) {
			first = false;
			printf("%s commands:\n", grpname);
			if (!verbose) {
				printf("  ");
				pos = 2;
			}
		}
		if (verbose) {
			printf("  %-21s %s\n", cmdtp->name, cmdtp->usage);
			continue;
		}
		len = strlen(cmdtp->name);
		if (pos + len + 2 > 77) {
			printf("\n  %s", cmdtp->name);
			pos = len + 2;
		} else {
			if (pos != 2) {
				printf(", ");
				pos += 2;
			}
			printf(cmdtp->name);
			pos += len;
		}
	}
	if (!first)
		printf("\n");
}

static void list_commands(int verbose)
{
	putchar('\n');
	list_group(verbose, "Information",           CMD_GRP_INFO);
	list_group(verbose, "Boot",                  CMD_GRP_BOOT);
	list_group(verbose, "Environment",           CMD_GRP_ENV);
	list_group(verbose, "Partition",             CMD_GRP_PART);
	list_group(verbose, "File",                  CMD_GRP_FILE);
	list_group(verbose, "Scripting",             CMD_GRP_SCRIPT);
	list_group(verbose, "Network",               CMD_GRP_NET);
	list_group(verbose, "Console",               CMD_GRP_CONSOLE);
	list_group(verbose, "Memory",                CMD_GRP_MEM);
	list_group(verbose, "Hardware manipulation", CMD_GRP_HWMANIP);
	list_group(verbose, "Miscellaneous",         CMD_GRP_MISC);
	list_group(verbose, "Ungrouped",             0);
	printf("Use 'help COMMAND' for more details.\n\n");
}


/*
 * Use puts() instead of printf() to avoid printf buffer overflow
 * for long help messages
 */
static int do_help(int argc, char *argv[])
{
	struct command *cmdtp;
	int opt, verbose = 0;

	while ((opt = getopt(argc, argv, "v")) > 0) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		}
	}

	if (optind == argc) {	/* show list of commands */
		list_commands(verbose);
		return 0;
	}

	/* command help (long version) */
	if ((cmdtp = find_cmd(argv[optind])) != NULL) {
		barebox_cmd_usage(cmdtp);
		return 0;
	} else {
		printf ("Unknown command '%s' - try 'help'"
			" without arguments for list of all"
			" known commands\n\n", argv[1]
				);
		return 1;
	}
}

static const __maybe_unused char cmd_help_help[] =
"Show help information (for 'command')\n"
"'help' prints online help for the monitor commands.\n\n"
"Without arguments, it lists all all commands.\n\n"
"To get detailed help information for specific commands you can type\n"
"'help' with a command names as argument.\n";

static const char *help_aliases[] = { "?", NULL};

BAREBOX_CMD_START(help)
	.cmd		= do_help,
	.aliases	= help_aliases,
	.usage		= "print online help",
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_help_help)
	BAREBOX_CMD_COMPLETE(command_complete)
BAREBOX_CMD_END

