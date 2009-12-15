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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 *  Definitions for Command Processor
 */
#ifndef __COMMAND_H
#define __COMMAND_H

#include <linux/list.h>

#ifndef NULL
#define NULL	0
#endif

#ifndef	__ASSEMBLY__

extern struct list_head command_list;

#define for_each_command(cmd)	list_for_each_entry(cmd, &command_list, list)

/*
 * Monitor Command Table
 */

struct cmd_tbl_s {
	const char	*name;		/* Command Name			*/
	const char	**aliases;
					/* Implementation function	*/
	int		(*cmd)(struct cmd_tbl_s *, int, char *[]);
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


typedef struct cmd_tbl_s	cmd_tbl_t;

extern cmd_tbl_t  __barebox_cmd_start;
extern cmd_tbl_t  __barebox_cmd_end;


/* common/command.c */
cmd_tbl_t *find_cmd(const char *cmd);
int execute_command(int argc, char **argv);
void barebox_cmd_usage(cmd_tbl_t *cmdtp);

#define COMMAND_SUCCESS		0
#define COMMAND_ERROR		1
#define COMMAND_ERROR_USAGE	2

#endif	/* __ASSEMBLY__ */

/*
 * Configurable monitor commands definitions have been moved
 * to include/cmd_confdefs.h
 */

#define __stringify_1(x)	#x
#define __stringify(x)		__stringify_1(x)


#define Struct_Section  __attribute__ ((unused,section (".barebox_cmd")))

#define BAREBOX_CMD_START(_name)				\
const cmd_tbl_t __barebox_cmd_##_name	\
	__attribute__ ((unused,section (".barebox_cmd_" __stringify(_name)))) = {				\
	.name		= #_name,

#define BAREBOX_CMD_END					\
};

#ifdef CONFIG_LONGHELP
#define BAREBOX_CMD_HELP(text)	.help = text,
#else
#define BAREBOX_CMD_HELP(text)
#endif

int register_command(cmd_tbl_t *);

#endif	/* __COMMAND_H */
