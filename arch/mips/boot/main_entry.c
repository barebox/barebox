// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <init.h>
#include <common.h>
#include <string.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/cpu-features.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>

extern void handle_reserved(void);

void main_entry(void *fdt, u32 fdt_size);

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

extern void *glob_fdt;
extern u32 glob_fdt_size;

/**
 * Called plainly from assembler code
 *
 * @note The C environment isn't initialized yet
 */
void __bare_init main_entry(void *fdt, u32 fdt_size)
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

	glob_fdt = fdt;
	glob_fdt_size = fdt_size;

	start_barebox();
}
