// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <getopt.h>

LIST_HEAD(command_list);
EXPORT_SYMBOL(command_list);

void barebox_cmd_usage(struct command *cmdtp)
{
	putchar('\n');
	if (cmdtp->desc)
		printf("%s - %s\n\n", cmdtp->name, cmdtp->desc);
	if (cmdtp->opts)
		printf("Usage: %s %s\n\n", cmdtp->name, cmdtp->opts);
#ifdef CONFIG_LONGHELP
	/* found - print (long) help info */
	if (cmdtp->help) {
		puts(cmdtp->help);
		putchar('\n');
	}
	if (cmdtp->usage)
		cmdtp->usage();
#endif
}
EXPORT_SYMBOL(barebox_cmd_usage);

static int compare(struct list_head *a, struct list_head *b)
{
	char *na = (char*)list_entry(a, struct command, list)->name;
	char *nb = (char*)list_entry(b, struct command, list)->name;

	return strcmp(na, nb);
}

int execute_command(int argc, char **argv)
{
	struct command *cmdtp;
	int ret;
	struct getopt_context gc;

	getopt_context_store(&gc);

	/* Look up command in command table */
	if ((cmdtp = find_cmd(argv[0]))) {
		/* OK - call function to do the command */
		ret = cmdtp->cmd(argc, argv);
		if (ret == COMMAND_ERROR_USAGE) {
			barebox_cmd_usage(cmdtp);
			ret = COMMAND_ERROR;
		}
	} else {
#ifdef CONFIG_CMD_HELP
		printf ("Unknown command '%s' - try 'help'\n", argv[0]);
#else
		printf ("Unknown command '%s'\n", argv[0]);
#endif
		ret = COMMAND_ERROR;	/* give up after bad command */
	}

	getopt_context_restore(&gc);

	return ret;
}

int register_command(struct command *cmd)
{
	/*
	 * We do not check if the command already exists.
	 * This allows us to overwrite a builtin command
	 * with a module.
	 */

	debug("register command %s\n", cmd->name);

	list_add_sort(&cmd->list, &command_list, compare);

	if (cmd->aliases) {
		const char * const *aliases = cmd->aliases;
		while(*aliases) {
			struct command *c = xzalloc(sizeof(struct command));

			memcpy(c, cmd, sizeof(struct command));

			c->name = *aliases;
			c->desc = cmd->desc;
			c->opts = cmd->opts;

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
struct command *find_cmd (const char *cmd)
{
	struct command *cmdtp;

	for_each_command(cmdtp)
		if (!strcmp(cmd, cmdtp->name))
			return cmdtp;

	return NULL;	/* not found or ambiguous command */
}
EXPORT_SYMBOL(find_cmd);

int cmd_export_val(const char *variable, const char *val)
{
	if (!val)
		return -EINVAL;

	if (variable)
		return setenv(variable, val);

	printf("%s\n", val);
	return 0;
}
EXPORT_SYMBOL(cmd_export_val);

/*
 * Put all commands into a linked list. Without module support we could use
 * the raw command array, but with module support a list is easier to handle.
 * It does not create significant overhead so do it also without module
 * support.
 */
static int init_command_list(void)
{
	struct command * const *cmdtp;

	for (cmdtp = __barebox_cmd_start;
			cmdtp != __barebox_cmd_end;
			cmdtp++)
		register_command(*cmdtp);

	return 0;
}

late_initcall(init_command_list);
