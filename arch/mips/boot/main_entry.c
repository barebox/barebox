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
#include <linux/sizes.h>

extern void exception_vec(void);
extern void exception_vec_end(void);

void main_entry(void *fdt, u32 fdt_size);

static void trap_init(void)
{
	const unsigned long vec_size = exception_vec_end - exception_vec;
	const unsigned long ebase = CKSEG1;

	 /* copy the generic exception handlers to their final destination */
	memcpy((void *)(ebase + 0x180), &exception_vec, vec_size);

	/* FIXME: handle tlb */
	memcpy((void *)(ebase), &exception_vec, vec_size);
	memcpy((void *)(ebase + 0x80), &exception_vec, vec_size);

	/* unset BOOT EXCEPTION VECTOR bit */
	write_c0_status(read_c0_status() & ~ST0_BEV);
}

extern void *glob_fdt;
extern u32 glob_fdt_size;
extern unsigned long mips_stack_top;

/**
 * Called plainly from assembler code
 *
 * @note The C environment isn't initialized yet
 */
void __bare_init main_entry(void *fdt, u32 fdt_size)
{
	unsigned long malloc_start, malloc_end;
	/* clear the BSS first */
	memset(__bss_start, 0x00, __bss_stop - __bss_start);

	cpu_probe();

	trap_init();

	malloc_end = (unsigned long)_stext;

	if (MALLOC_SIZE > 0)
		malloc_start = malloc_end - MALLOC_SIZE;
	else
		malloc_start = malloc_end - SZ_8M;

	pr_debug("initializing malloc pool at 0x%08lx (size 0x%08lx)\n",
			malloc_start, malloc_end - malloc_start);

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);
	mips_stack_top = malloc_start;

	glob_fdt = fdt;
	glob_fdt_size = fdt_size;

	start_barebox();
}
