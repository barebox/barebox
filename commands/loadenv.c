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

static int do_loadenv(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	char *filename, *dirname;

	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = "/dev/env0";
	else
		filename = argv[1];
	printf("loading environment from %s\n", filename);
	return envfs_load(filename, dirname);
}

static const __maybe_unused char cmd_loadenv_help[] =
"Usage: loadenv [ENVFS] [DIRECTORY]\n"
"Load the persistent storage contained in <envfs> to the directory\n"
"<directory>.\n"
"If ommitted <directory> defaults to /env and <envfs> defaults to /dev/env0.\n"
"Note that envfs can only handle files. Directories are skipped silently.\n";

BAREBOX_CMD_START(loadenv)
	.cmd		= do_loadenv,
	.usage		= "load environment from persistent storage",
	BAREBOX_CMD_HELP(cmd_loadenv_help)
BAREBOX_CMD_END

/**
 * @page loadenv_command loadenv
 *
 * Usage: loadenv [\<directory>] [\<envfs>]
 *
 * Load the persistent storage contained in \<envfs> to the directory \<directory>.
 *
 * If ommitted \<directory> defaults to \c /env and \<envfs> defaults to
 * \c /dev/env0.
 *
 * @note envfs can only handle files. Directories are skipped silently.
 */
