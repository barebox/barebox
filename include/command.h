/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 *  Definitions for Command Processor
 */
#ifndef __COMMAND_H
#define __COMMAND_H

#include <linux/list.h>
#include <linux/stringify.h>
#include <linux/compiler_types.h>
#include <linux/stddef.h>
#include <string.h>

#ifndef	__ASSEMBLY__

extern struct list_head command_list;

#define for_each_command(cmd)	list_for_each_entry(cmd, &command_list, list)

struct string_list;

/*
 * Monitor Command Table
 */
struct command {
	const char	*name;		/* Command Name			*/
	const char	* const *aliases;
					/* Implementation function	*/
	int		(*cmd)(int, char *[]);
	int		(*complete)(struct string_list *sl, char *instr);
	const char	*desc;		/* Short command description, start with lowercase */
	const char	*opts;		/* command options */

	struct list_head list;		/* List of commands		*/
	uint32_t	group;
#ifdef	CONFIG_LONGHELP
	const char	*help;		/* Help  message	(long)	*/
	void		(*usage)(void);
#endif
};

extern struct command * const __barebox_cmd_start[];
extern struct command * const __barebox_cmd_end[];


/* common/command.c */
struct command *find_cmd(const char *cmd);
int execute_command(int argc, char **argv);
void barebox_cmd_usage(struct command *cmdtp);
int run_command(const char *cmd);

#define COMMAND_SUCCESS		0
#define COMMAND_ERROR		1
#define COMMAND_ERROR_USAGE	2

/* Note: keep this list in sync with commands/help.c */
#define CMD_GRP_INFO		1
#define CMD_GRP_BOOT		2
#define CMD_GRP_ENV		3
#define CMD_GRP_FILE		4
#define CMD_GRP_PART		5
#define CMD_GRP_SCRIPT		6
#define CMD_GRP_NET		7
#define CMD_GRP_CONSOLE		8
#define CMD_GRP_MEM		9
#define CMD_GRP_HWMANIP		10
#define CMD_GRP_MISC		11

#endif	/* __ASSEMBLY__ */

#define BAREBOX_CMD_START(_name)						\
static struct command __barebox_cmd_##_name;					\
const struct command *barebox_cmd_##_name 					\
	__ll_elem(.barebox_cmd_##_name)	= &__barebox_cmd_##_name;		\
static struct command __barebox_cmd_##_name = {					\
	.name		= #_name,

#define BAREBOX_CMD_END					\
};
#ifdef CONFIG_AUTO_COMPLETE
#define BAREBOX_CMD_COMPLETE(_cpt) .complete = _cpt,
#else
#define BAREBOX_CMD_COMPLETE(_cpt)
#endif

#define BAREBOX_CMD_HELP_START(_name) \
static const __maybe_unused char cmd_##_name##_help[] =

#define BAREBOX_CMD_HELP_OPT(_opt, _desc) "\t" _opt "\t" _desc "\n"
#define BAREBOX_CMD_HELP_TEXT(_text) _text "\n"
#define BAREBOX_CMD_HELP_END ;

#ifdef CONFIG_LONGHELP
#define BAREBOX_CMD_HELP(text)	.help = text,
#define BAREBOX_CMD_USAGE(fn)	.usage = fn,
#else
#define BAREBOX_CMD_HELP(text)
#define BAREBOX_CMD_USAGE(fn)
#endif

#define BAREBOX_CMD_GROUP(grp)	.group = grp,

#define BAREBOX_CMD_DESC(text)	.desc = text,

#define BAREBOX_CMD_OPTS(text)	.opts = text,

int register_command(struct command *);

#endif	/* __COMMAND_H */
