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
#include <list.h>
#include <init.h>
#include <complete.h>
#include <linux/utsrelease.h>

const char version_string[] =
	"U-Boot " UTS_RELEASE " (" __DATE__ " - " __TIME__ ")";

LIST_HEAD(command_list);
EXPORT_SYMBOL(command_list);

static int do_version (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	printf ("\n%s\n", version_string);
	return 0;
}

U_BOOT_CMD_START(version)
	.maxargs	= 1,
	.cmd		= do_version,
	.usage		= "print monitor version",
U_BOOT_CMD_END

static int do_true (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return 0;
}

U_BOOT_CMD_START(true)
	.maxargs	= 1,
	.cmd		= do_true,
	.usage		= "do nothing, successfully",
U_BOOT_CMD_END

static int do_false (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	return 1;
}

U_BOOT_CMD_START(false)
	.maxargs	= 1,
	.cmd		= do_false,
	.usage		= "do nothing, unsuccessfully",
U_BOOT_CMD_END

#ifdef CONFIG_SHELL_HUSH

static int do_exit (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int r;

	r = 0;
	if (argc > 1)
		r = simple_strtoul(argv[1], NULL, 0);

	return -r - 2;
}

U_BOOT_CMD_START(exit)
	.maxargs	= 2,
	.cmd		= do_exit,
	.usage		= "exit script",
U_BOOT_CMD_END

#endif

void u_boot_cmd_usage(cmd_tbl_t *cmdtp)
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
EXPORT_SYMBOL(u_boot_cmd_usage);

/*
 * Use puts() instead of printf() to avoid printf buffer overflow
 * for long help messages
 */
static int do_help (cmd_tbl_t * cmdtp, int argc, char *argv[])
{
	if (argc == 1) {	/* show list of commands */
		for_each_command(cmdtp) {
			if (!cmdtp->usage)
				continue;
			printf("%10s - %s\n", cmdtp->name, cmdtp->usage);
		}
		return 0;
	}

	/* command help (long version) */
	if ((cmdtp = find_cmd(argv[1])) != NULL) {
		u_boot_cmd_usage(cmdtp);
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
"Without arguments, it prints a short usage message for all commands.\n\n"
"To get detailed help information for specific commands you can type\n"
"'help' with one or more command names as arguments.\n";

static const char *help_aliases[] = { "?", NULL};

U_BOOT_CMD_START(help)
	.maxargs	= 2,
	.cmd		= do_help,
	.aliases	= help_aliases,
	.usage		= "print online help",
	U_BOOT_CMD_HELP(cmd_help_help)
U_BOOT_CMD_END

static int compare(struct list_head *a, struct list_head *b)
{
	char *na = (char*)list_entry(a, cmd_tbl_t, list)->name;
	char *nb = (char*)list_entry(b, cmd_tbl_t, list)->name;

	return strcmp(na, nb);
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
	cmd_tbl_t *cmdtp_temp = &__u_boot_cmd_start;	/*Init value */
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

	for (cmdtp = &__u_boot_cmd_start;
			cmdtp != &__u_boot_cmd_end;
			cmdtp++)
		register_command(cmdtp);

	return 0;
}

late_initcall(init_command_list);

