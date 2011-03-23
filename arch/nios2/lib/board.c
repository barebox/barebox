/*
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
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
#include <malloc.h>
#include <init.h>
#include <mem_malloc.h>
#include <asm-generic/memory_layout.h>
#include <cache.h>

int altera_mem_malloc_init(void)
{

	mem_malloc_init((void *)(NIOS_SOPC_TEXT_BASE - MALLOC_SIZE),
			(void *)(NIOS_SOPC_TEXT_BASE));

	return 0;
}

core_initcall(altera_mem_malloc_init);

void arch_shutdown(void)
{
#ifdef CONFIG_USE_IRQ
	disable_interrupts();
#endif
}

