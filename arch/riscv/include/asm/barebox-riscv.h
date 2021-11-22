/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 */

#ifndef _BAREBOX_RISCV_H_
#define _BAREBOX_RISCV_H_

#include <linux/sizes.h>
#include <asm-generic/memory_layout.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <asm/sections.h>
#include <asm/barebox-riscv-head.h>
#include <asm/system.h>
#include <asm/cache.h>

unsigned long get_runtime_offset(void);

void setup_c(void);
void relocate_to_current_adr(void);
void relocate_to_adr(unsigned long target);

void __noreturn __naked barebox_riscv_entry(unsigned long membase, unsigned long memsize,
					    void *boarddata, unsigned int flags);

#define barebox_riscv_machine_entry(membase, memsize, boarddata) \
	barebox_riscv_entry(membase, memsize, boarddata, RISCV_M_MODE)

#define barebox_riscv_supervisor_entry(membase, memsize, hartid, boarddata) do { \
	__asm__ volatile("mv tp, %0\n" : : "r"(hartid)); \
	barebox_riscv_entry(membase, memsize, boarddata, RISCV_S_MODE); \
} while (0)

unsigned long riscv_mem_ramoops_get(void);
unsigned long riscv_mem_endmem_get(void);

void *barebox_riscv_boot_dtb(void);

static inline unsigned long riscv_mem_stack_top(unsigned long membase,
						unsigned long endmem)
{
	return endmem;
}

static inline unsigned long riscv_mem_stack(unsigned long membase,
					  unsigned long endmem)
{
	return riscv_mem_stack_top(membase, endmem) - STACK_SIZE;
}

static inline unsigned long riscv_mem_early_malloc(unsigned long membase,
						   unsigned long endmem)
{
	return riscv_mem_stack(membase, endmem) - SZ_128K;
}

static inline unsigned long riscv_mem_early_malloc_end(unsigned long membase,
						       unsigned long endmem)
{
	return riscv_mem_stack(membase, endmem);
}

static inline unsigned long riscv_mem_ramoops(unsigned long membase,
					      unsigned long endmem)
{
	endmem = riscv_mem_stack(membase, endmem);
#ifdef CONFIG_FS_PSTORE_RAMOOPS
	endmem -= CONFIG_FS_PSTORE_RAMOOPS_SIZE;
	endmem = ALIGN_DOWN(endmem, SZ_4K);
#endif

	return endmem;
}

static inline unsigned long riscv_mem_barebox_image(unsigned long membase,
						    unsigned long endmem,
						    unsigned long size)
{
	endmem = riscv_mem_ramoops(membase, endmem);

	return ALIGN_DOWN(endmem - size, SZ_1M);
}

#define ENTRY_FUNCTION(name, arg0, arg1, arg2)                          \
	void name (ulong r0, ulong r1, ulong r2);                       \
	static void __##name(ulong, ulong, ulong);                      \
	void __naked __noreturn __section(.text_head_entry_##name) name \
		(ulong a0, ulong a1, ulong a2)                          \
	{                                                               \
		__barebox_riscv_head();                                 \
		__##name(a0, a1, a2);                                   \
	}                                                               \
	static void __naked __noreturn noinline __##name                \
		(ulong arg0, ulong arg1, ulong arg2)


/*
 * When using compressed images in conjunction with relocatable images
 * the PBL code must pick a suitable place where to uncompress the barebox
 * image. For doing this the PBL code must know the size of the final
 * image including the BSS segment. The BSS size is unknown to the PBL
 * code, so define a maximum BSS size here.
 */
#define MAX_BSS_SIZE SZ_1M

#define barebox_image_size (__image_end - __image_start)

#endif	/* _BAREBOX_RISCV_H_ */
