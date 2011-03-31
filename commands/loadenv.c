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
 * @brief loadenv: Restoring a environment
 */

#include <common.h>
#include <command.h>
#include <environment.h>

static int do_loadenv(struct command *cmdtp, int argc, char *argv[])
{
	char *filename, *dirname;

	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = default_environment_path;
	else
		filename = argv[1];
	printf("loading environment from %s\n", filename);
	return envfs_load(filename, dirname);
}

BAREBOX_CMD_HELP_START(loadenv)
BAREBOX_CMD_HELP_USAGE("loadenv [ENVFS] [DIRECTORY]\n")
BAREBOX_CMD_HELP_SHORT("Load environment from ENVFS into DIRECTORY (default: /dev/env0 -> /env).\n")
BAREBOX_CMD_HELP_END

/**
 * @page loadenv_command

ENVFS can only handle files, directories are skipped silently.

\todo This needs proper documentation. What is ENVFS, why is it FS etc. Explain the concepts.

 */

BAREBOX_CMD_START(loadenv)
	.cmd		= do_loadenv,
	.usage		= "Load environment from ENVFS into DIRECTORY (default: /dev/env0 -> /env).",
	BAREBOX_CMD_HELP(cmd_loadenv_help)
BAREBOX_CMD_END
