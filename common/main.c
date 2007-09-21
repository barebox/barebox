/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Add to readline cmdline-editing by
 * (C) Copyright 2005
 * JinHua Luo, GuangDong Linux Center, <luo.jinhua@gd-linux.com>
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

/* #define	DEBUG	*/

#include <common.h>
#include <environment.h>
#include <watchdog.h>
#include <command.h>

#ifdef CONFIG_HUSH_PARSER
#include <hush.h>
#endif

extern int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);


#define MAX_DELAY_STOP_STR 32

#undef DEBUG_PARSER

char        console_buffer[CONFIG_CBSIZE];		/* console I/O buffer	*/

/****************************************************************************/

void main_loop (void)
{
#ifndef CONFIG_HUSH_PARSER
	static char lastcommand[CONFIG_CBSIZE] = { 0, };
	int len;
	int rc = 1;
	int flag;
#endif

#ifdef CONFIG_AUTO_COMPLETE
	install_auto_complete();
#endif

	/*
	 * Main Loop for Monitor Command Processing
	 */
#ifdef CONFIG_HUSH_PARSER
	parse_file_outer();
	/* This point is never reached */
	for (;;);
#else
	for (;;) {
		len = readline (CONFIG_PROMPT, console_buffer, CONFIG_CBSIZE);

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strcpy (lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;

		if (len == -1)
			puts ("<INTERRUPT>\n");
		else
			rc = run_command (lastcommand, flag);

		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
#endif /*CONFIG_HUSH_PARSER*/
}
