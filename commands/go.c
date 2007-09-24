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
 * Misc boot support
 */

#include <common.h>
#include <command.h>

int do_go (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

#ifdef ARCH_HAS_EXECUTE
	rc = arch_execute(addr, argc, &argv[1]);
#else
	rc = ((ulong (*)(int, char *[]))addr) (--argc, &argv[1]);
#endif

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD_START(go)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_go,
	.usage		= "start application at address 'addr'",
	U_BOOT_CMD_HELP(
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments\n")
U_BOOT_CMD_END
