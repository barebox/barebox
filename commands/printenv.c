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
 * @brief printenv: Print out environment variables
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <environment.h>

static int do_printenv(struct command *cmdtp, int argc, char *argv[])
{
	struct variable_d *var;
	struct env_context *c, *current_c;

	if (argc == 2) {
		const char *val = getenv(argv[1]);
		if (val) {
			printf("%s=%s\n", argv[1], val);
			return 0;
		}
		printf("## Error: \"%s\" not defined\n", argv[1]);
		return 1;
	}

	current_c = get_current_context();
	var = current_c->local->next;
	printf("locals:\n");
	while (var) {
		printf("%s=%s\n", var_name(var), var_val(var));
		var = var->next;
	}

	printf("globals:\n");
	c = get_current_context();
	while(c) {
		var = c->global->next;
		while (var) {
			printf("%s=%s\n", var_name(var), var_val(var));
			var = var->next;
		}
		c = c->parent;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(printenv)
BAREBOX_CMD_HELP_USAGE("printenv [variable]\n")
BAREBOX_CMD_HELP_SHORT("Print value of one or all environment variables.\n")
BAREBOX_CMD_HELP_END

/**
 * @page printenv_command

<p>If an argument is given, printenv prints the content of an environment
variable to the terminal. If no argument is specified, all variables are
printed.</p>

 */

BAREBOX_CMD_START(printenv)
	.cmd		= do_printenv,
	.usage		= "Print value of one or all environment variables.",
	BAREBOX_CMD_HELP(cmd_printenv_help)
BAREBOX_CMD_END
