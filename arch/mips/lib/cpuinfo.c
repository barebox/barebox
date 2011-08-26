/*
 * cpuinfo - show information about MIPS CPU
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>

static int do_cpuinfo(struct command *cmdtp, int argc, char *argv[])
{
	printf("CP0_PRID = 0x%08x\n", read_c0_prid());
	printf("CP0_CONFIG = 0x%08x\n", read_c0_config());

	return 0;
}

BAREBOX_CMD_START(cpuinfo)
	.cmd            = do_cpuinfo,
	.usage          = "Show info about CPU",
BAREBOX_CMD_END
