/*
 * barebox - board.c First C file to be called contains init routines
 *
 * Copyright (c) 2005 blackfin.uclinux.org
 *
 * (C) Copyright 2000-2004
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

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mem_malloc.h>
#include <asm/cpu.h>
#include <asm-generic/memory_layout.h>

int blackfin_mem_malloc_init(void)
{
	mem_malloc_init((void *)(MALLOC_BASE),
			(void *)(MALLOC_BASE + MALLOC_SIZE));
	return 0;
}

core_initcall(blackfin_mem_malloc_init);

void arch_shutdown(void)
{
	icache_disable();
}
