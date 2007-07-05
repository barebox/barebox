/*
 * (C) Copyright 2001
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
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <clock.h>
#include <malloc.h>
#include <readkey.h>

#ifdef CONFIG_CMD_SLEEP
int do_sleep (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uint64_t start;
	ulong delay;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	delay = simple_strtoul(argv[1], NULL, 10);

	start = get_time_ns();
	while (!is_timeout(start, delay * SECOND)) {
		if (ctrlc())
			return 1;
	}

	return 0;
}

U_BOOT_CMD_START(sleep)
	.maxargs	= 2,
	.cmd		= do_sleep,
	.usage		= "delay execution for n seconds",
U_BOOT_CMD_END

#endif

#ifdef CONFIG_CMD_CLEAR
int do_clear (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf(ANSI_CLEAR_SCREEN);

	return 0;
}

U_BOOT_CMD_START(clear)
	.maxargs	= 1,
	.cmd		= do_clear,
	.usage		= "clear screen",
U_BOOT_CMD_END
#endif

#ifdef CONFIG_CMD_MEMINFO
int do_meminfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	malloc_stats();

	return 0;
}

U_BOOT_CMD_START(meminfo)
	.maxargs	= 1,
	.cmd		= do_meminfo,
	.usage		= "print info about memory usage",
U_BOOT_CMD_END
#endif

/* Implemented in $(CPU)/interrupts.c */
#if (CONFIG_COMMANDS & CFG_CMD_IRQ)
int do_irqinfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD_START(irqinfo)
	.maxargs	= 1,
	.cmd		= do_irqinfo,
	.usage		= "print information about IRQs",
U_BOOT_CMD_END
#endif  /* CONFIG_COMMANDS & CFG_CMD_IRQ */

