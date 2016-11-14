/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
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

#include <common.h>
#include <string.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/cpu-features.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>

extern void handle_reserved(void);

void main_entry(void);

unsigned long exception_handlers[32];

static void set_except_vector(int n, void *addr)
{
	unsigned handler = (unsigned long) addr;

	exception_handlers[n] = handler;
}

static void trap_init(void)
{
	extern char except_vec3_generic;
	int i;

	unsigned long ebase;

	ebase = CKSEG1;

	/*
	 * Copy the generic exception handlers to their final destination.
	 * This will be overriden later as suitable for a particular
	 * configuration.
	 */
	memcpy((void *)(ebase + 0x180), &except_vec3_generic, 0x80);

	/*
	 * Setup default vectors
	 */
	for (i = 0; i <= 31; i++) {
		set_except_vector(i, &handle_reserved);
	}

	if (!cpu_has_4kex)
		memcpy((void *)(ebase + 0x080), &except_vec3_generic, 0x80);

	/* FIXME: handle tlb */
	memcpy((void *)(ebase), &except_vec3_generic, 0x80);

	/* unset BOOT EXCEPTION VECTOR bit */
	write_c0_status(read_c0_status() & ~ST0_BEV);
}

/**
 * Called plainly from assembler code
 *
 * @note The C environment isn't initialized yet
 */
void main_entry(void)
{
	/* clear the BSS first */
	memset(__bss_start, 0x00, __bss_stop - __bss_start);

	cpu_probe();

	if (cpu_has_4k_cache) {
		extern void r4k_cache_init(void);

		r4k_cache_init();
	}

	trap_init();

	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));

	start_barebox();
}
