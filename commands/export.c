/*
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file
 * @brief export: Export an environment variable
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <environment.h>

static int do_export ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i = 1;
	char *ptr;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	while (i < argc) {
		if ((ptr = strchr(argv[i], '='))) {
			*ptr++ = 0;
			setenv(argv[i], ptr);
		}
		if (export(argv[i])) {
			printf("could not export: %s\n", argv[i]);
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_export_help[] =
"Usage: export <var>[=value]...\n"
"export an environment variable to subsequently executed scripts\n";

U_BOOT_CMD_START(export)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_export,
	.usage		= "export environment variables",
	U_BOOT_CMD_HELP(cmd_export_help)
U_BOOT_CMD_END

/**
 * @page export_command export: Export an environment variable
 *
 * Usage: export \<var>[=value]...
 *
 * Export an environment variable to subsequently executed scripts.
 */
