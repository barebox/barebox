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
 * @brief setenv: Set an environment variables
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <environment.h>

static int do_setenv ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	setenv(argv[1], argv[2]);

	return 0;
}

static const __maybe_unused char cmd_setenv_help[] =
"name value ...\n"
"    - set environment variable 'name' to 'value ...'\n"
"setenv name\n"
"    - delete environment variable 'name'\n";


BAREBOX_CMD_START(setenv)
	.cmd		= do_setenv,
	.usage		= "set environment variables",
	BAREBOX_CMD_HELP(cmd_setenv_help)
BAREBOX_CMD_END

/**
 * @page setenv_command setenv: set an environment variable
 *
 * Usage: setenv \<name> [\<value>]
 *
 * Set environment variable \<name> to \<value>. Without a given value, the
 * environment variable will be deleted.
 *
 * @note This command is only available if the simple command line parser is
 * in use. Within the hush shell \c setenv is not required.
 */
