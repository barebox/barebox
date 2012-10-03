/*
 * (C) Copyright 2000
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

/*
 *  Definitions for Command Processor
 */
#ifndef __COMMAND_H
#define __COMMAND_H

#include <linux/list.h>
#include <linux/stringify.h>

#ifndef NULL
#define NULL	0
#endif

#ifndef	__ASSEMBLY__

extern struct list_head command_list;

#define for_each_command(cmd)	list_for_each_entry(cmd, &command_list, list)

struct string_list;

/*
 * Monitor Command Table
 */
struct command {
	const char	*name;		/* Command Name			*/
	const char	**aliases;
					/* Implementation function	*/
	int		(*cmd)(int, char *[]);
	int		(*complete)(struct string_list *sl, char *instr);
	const char	*usage;		/* Usage message	(short)	*/

	struct list_head list;		/* List of commands		*/
#ifdef	CONFIG_LONGHELP
	const char	*help;		/* Help  message	(long)	*/
#endif
}
#ifdef __x86_64__
/* This is required because the linker will put symbols on a 64 bit alignment */
__attribute__((aligned(64)))
#endif
;

extern struct command __barebox_cmd_start;
extern struct command __barebox_cmd_end;


/* common/command.c */
struct command *find_cmd(const char *cmd);
int execute_command(int argc, char **argv);
void barebox_cmd_usage(struct command *cmdtp);

#define COMMAND_SUCCESS		0
#define COMMAND_ERROR		1
#define COMMAND_ERROR_USAGE	2

#endif	/* __ASSEMBLY__ */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define Struct_Section  __attribute__ ((unused,section (".barebox_cmd")))

#define BAREBOX_CMD_START(_name)							\
extern const struct command __barebox_cmd_##_name;					\
const struct command __barebox_cmd_##_name						\
	__attribute__ ((unused,section (".barebox_cmd_" __stringify(_name)))) = {	\
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

#define BAREBOX_CMD_HELP_USAGE(_name) "Usage: " _name
#define BAREBOX_CMD_HELP_SHORT(_text) _text
#define BAREBOX_CMD_HELP_OPT(_opt, _desc) _opt "\t" _desc
#define BAREBOX_CMD_HELP_TEXT(_text) _text
#define BAREBOX_CMD_HELP_END ;

#ifdef CONFIG_LONGHELP
#define BAREBOX_CMD_HELP(text)	.help = text,
#else
#define BAREBOX_CMD_HELP(text)
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int register_command(struct command *);

#endif	/* __COMMAND_H */
