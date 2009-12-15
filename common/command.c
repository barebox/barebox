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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 *  Command Processor Table
 */

#include <common.h>
#include <command.h>
#include <xfuncs.h>
#include <malloc.h>
#include <environment.h>
#include <linux/list.h>
#include <init.h>
#include <complete.h>
#include <linux/utsrelease.h>
#include <getopt.h>

const char version_string[] =
	"barebox " UTS_RELEASE " (" __DATE__ " - " __TIME__ ")";

LIST_HEAD(command_list);
EXPORT_SYMBOL(command_list);

#ifdef CONFIG_SHELL_HUSH

static int do_exit (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int r;

	r = 0;
	if (argc > 1)
		r = simple_strtoul(argv[1], NULL, 0);

	return -r - 2;
}

BAREBOX_CMD_START(exit)
	.cmd		= do_exit,
	.usage		= "exit script",
BAREBOX_CMD_END

#endif

void barebox_cmd_usage(cmd_tbl_t *cmdtp)
{
#ifdef	CONFIG_LONGHELP
		/* found - print (long) help info */
		if (cmdtp->help) {
			puts (cmdtp->help);
		} else {
			puts (cmdtp->name);
			putchar (' ');
			puts ("- No help available.\n");
		}
		putchar ('\n');
#else	/* no long help available */
		if (cmdtp->usage) {
			puts (cmdtp->usage);
			puts("\n");
		}
#endif	/* CONFIG_LONGHELP */
}
EXPORT_SYMBOL(barebox_cmd_usage);

static int compare(struct list_head *a, struct list_head *b)
{
	char *na = (char*)list_entry(a, cmd_tbl_t, list)->name;
	char *nb = (char*)list_entry(b, cmd_tbl_t, list)->name;

	return strcmp(na, nb);
}

int execute_command(int argc, char **argv)
{
	cmd_tbl_t *cmdtp;
	int ret;

	getopt_reset();

	/* Look up command in command table */
	if ((cmdtp = find_cmd(argv[0]))) {
		/* OK - call function to do the command */
		ret = cmdtp->cmd(cmdtp, argc, argv);
		if (ret == COMMAND_ERROR_USAGE) {
			barebox_cmd_usage(cmdtp);
			return COMMAND_ERROR;
		}
		return ret;
	} else {
#ifdef CONFIG_CMD_HELP
		printf ("Unknown command '%s' - try 'help'\n", argv[0]);
#else
		printf ("Unknown command '%s'\n", argv[0]);
#endif
		return -1;	/* give up after bad command */
	}
}

int register_command(cmd_tbl_t *cmd)
{
	/*
	 * We do not check if the command already exists.
	 * This allows us to overwrite a builtin command
	 * with a module.
	 */

	debug("register command %s\n", cmd->name);

	list_add_sort(&cmd->list, &command_list, compare);

	if (cmd->aliases) {
		char **aliases = (char**)cmd->aliases;
		while(*aliases) {
			char *usage = "alias for ";
			cmd_tbl_t *c = xzalloc(sizeof(cmd_tbl_t));

			memcpy(c, cmd, sizeof(cmd_tbl_t));

			c->name = *aliases;
			c->usage = xmalloc(strlen(usage) + strlen(cmd->name) + 1);
			sprintf((char*)c->usage, "%s%s", usage, cmd->name);

			c->aliases = NULL;

			register_command(c);

			aliases++;
		}
	}

	return 0;
}
EXPORT_SYMBOL(register_command);

/*
 * find command table entry for a command
 */
cmd_tbl_t *find_cmd (const char *cmd)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = &__barebox_cmd_start;	/*Init value */
	int len;
	int n_found = 0;
	len = strlen (cmd);

	cmdtp = list_entry(&command_list, cmd_tbl_t, list);

	for_each_command(cmdtp) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp;		/* full match */

			cmdtp_temp = cmdtp;		/* abbreviated command ? */
			n_found++;
		}
	}

	if (n_found == 1) {			/* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}

/*
 * Put all commands into a linked list. Without module support we could use
 * the raw command array, but with module support a list is easier to handle.
 * It does not create significant overhead so do it also without module
 * support.
 */
static int init_command_list(void)
{
	cmd_tbl_t *cmdtp;

	for (cmdtp = &__barebox_cmd_start;
			cmdtp != &__barebox_cmd_end;
			cmdtp++)
		register_command(cmdtp);

	return 0;
}

late_initcall(init_command_list);

