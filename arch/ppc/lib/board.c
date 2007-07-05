/*
 * (C) Copyright 2000-2006
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

#include <debug_ll.h>
#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <malloc.h>
#include <mem_malloc.h>
#include <init.h>
#include <net.h>
#include <serial.h>

char *strmhz (char *buf, long hz)
{
	long l, n;
	long m;

	n = hz / 1000000L;
	l = sprintf (buf, "%ld", n);
	m = (hz % 1000000L) / 1000L;
	if (m != 0)
		sprintf (buf + l, ".%03ld", m);
	return (buf);
}

/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r (ulong end_of_ram)
{
	extern void malloc_bin_reloc (void);

	asm ("sync ; isync");

	mem_malloc_init((void *)(end_of_ram - 4096 - CFG_MALLOC_LEN), (void *)(end_of_ram - 4096));

	/*
	 * Setup trap handlers
	 */
	trap_init (0);

	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r ();

	/*
	 * Enable Interrupts
	 */
#ifdef CONFIG_INTERRUPTS
	interrupt_init ();
#endif

	/* Initialization complete - start the monitor */

	start_uboot();
}

