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

#ifndef _BAREBOX_ARM_H_
#define _BAREBOX_ARM_H_

#include <linux/sizes.h>
#include <asm-generic/memory_layout.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <asm/barebox-arm-head.h>
#include <asm/common.h>
#include <asm/sections.h>
#include <asm/reloc.h>
#include <linux/stringify.h>
#include <boarddata.h>

#define ARM_EARLY_PAGETABLE_SIZE	SZ_64K

void __noreturn barebox_arm_entry(unsigned long membase, unsigned long memsize, void *boarddata);

#define barebox_arm_boarddata		barebox_boarddata
#define BAREBOX_ARM_BOARDDATA_MAGIC	BAREBOX_BOARDDATA_MAGIC

u32 barebox_arm_machine(void);

unsigned long arm_mem_ramoops_get(void);
unsigned long arm_mem_membase_get(void);
unsigned long arm_mem_endmem_get(void);

struct barebox_arm_boarddata *barebox_arm_get_boarddata(void);

#define barebox_arm_get_boarddata barebox_get_boarddata

#if defined(CONFIG_RELOCATABLE) && defined(CONFIG_ARM_EXCEPTIONS)
void arm_fixup_vectors(void);
#else
static inline void arm_fixup_vectors(void)
{
}
#endif

void *barebox_arm_boot_dtb(void);

static inline unsigned long arm_mem_optee(unsigned long endmem)
{
	return endmem - OPTEE_SIZE;
}

static inline unsigned long arm_mem_scratch(unsigned long endmem)
{
	return arm_mem_optee(endmem) - SZ_32K;
}

static inline unsigned long arm_mem_stack(unsigned long endmem)
{
	return arm_mem_scratch(endmem) - STACK_SIZE;
}

static inline unsigned long arm_mem_guard_page(unsigned long endmem)
{
	endmem = arm_mem_stack(endmem);

	if (!IS_ENABLED(CONFIG_STACK_GUARD_PAGE))
		return endmem;

	return ALIGN_DOWN(endmem, PAGE_SIZE) - PAGE_SIZE;
}

static inline unsigned long arm_mem_ttb(unsigned long endmem)
{
	endmem = arm_mem_guard_page(endmem);
	endmem = ALIGN_DOWN(endmem, ARM_EARLY_PAGETABLE_SIZE) - ARM_EARLY_PAGETABLE_SIZE;

	return endmem;
}

static inline unsigned long arm_mem_early_malloc(unsigned long endmem)
{
	return arm_mem_ttb(endmem) - SZ_128K;
}

static inline unsigned long arm_mem_early_malloc_end(unsigned long endmem)
{
	return arm_mem_ttb(endmem);
}

static inline unsigned long arm_mem_ramoops(unsigned long endmem)
{
	endmem = arm_mem_ttb(endmem);
#ifdef CONFIG_FS_PSTORE_RAMOOPS
	endmem -= CONFIG_FS_PSTORE_RAMOOPS_SIZE;
	endmem = ALIGN_DOWN(endmem, SZ_4K);
#endif

	return endmem;
}

static inline unsigned long arm_mem_stack_top(unsigned long endmem)
{
	return arm_mem_stack(endmem) + STACK_SIZE;
}

static inline const void *arm_mem_scratch_get(void)
{
	return (const void *)arm_mem_scratch(arm_mem_endmem_get());
}

static inline unsigned long arm_mem_guard_page_get(void)
{
	return arm_mem_guard_page(arm_mem_endmem_get());
}

static inline unsigned long arm_mem_barebox_image(unsigned long membase,
						  unsigned long endmem,
						  unsigned long size)
{
	endmem = arm_mem_ramoops(endmem);

	if (IS_ENABLED(CONFIG_RELOCATABLE)) {
		return ALIGN_DOWN(endmem - size, SZ_1M);
	} else {
		if (TEXT_BASE >= membase && TEXT_BASE < endmem)
			return TEXT_BASE;
		else
			return endmem;
	}
}

/*
 * Unlike ENTRY_FUNCTION, this can be used to setup stack for a C entry
 * point on both ARM32 and ARM64. ENTRY_FUNCTION on ARM64 can only be used
 * if preceding boot stage has initialized the stack pointer.
 *
 * Stack top of 0 means stack is already set up. In that case, the follow-up
 * code block will not be inlined and may spill to stack right away.
 */
#ifdef CONFIG_CPU_64

void __barebox_arm64_head(ulong x0, ulong x1, ulong x2);

#define ENTRY_FUNCTION_WITHSTACK(name, stack_top, arg0, arg1, arg2)	\
	void name(ulong r0, ulong r1, ulong r2);			\
									\
	static void __##name(ulong, ulong, ulong);			\
									\
	void __section(.text_head_entry_##name)	name			\
				(ulong r0, ulong r1, ulong r2)		\
		{							\
			static __section(.pbl_board_stack_top_##name)	\
				const ulong __stack_top = (stack_top);	\
			__keep_symbolref(__barebox_arm64_head);		\
			__keep_symbolref(__stack_top);			\
			__##name(r0, r1, r2);				\
		}							\
		static void noinline __##name				\
			(ulong arg0, ulong arg1, ulong arg2)

#define ENTRY_FUNCTION(name, arg0, arg1, arg2)				\
	ENTRY_FUNCTION_WITHSTACK(name, 0, arg0, arg1, arg2)

#else
#define ENTRY_FUNCTION_WITHSTACK(name, stack_top, arg0, arg1, arg2)	\
	static void ____##name(ulong, ulong, ulong);			\
	ENTRY_FUNCTION(name, arg0, arg1, arg2)				\
	{								\
		if (stack_top)						\
			arm_setup_stack(stack_top);			\
		____##name(arg0, arg1, arg2);				\
	}								\
	static void noinline ____##name					\
		(ulong arg0, ulong arg1, ulong arg2)

#define ENTRY_FUNCTION_HEAD(name, head, arg0, arg1, arg2)		\
	void name(ulong r0, ulong r1, ulong r2);			\
									\
	static void __##name(ulong, ulong, ulong);			\
									\
	void __naked __section(.text_head_entry_##name)	name		\
				(ulong r0, ulong r1, ulong r2)		\
		{							\
			head();				\
			__##name(r0, r1, r2);				\
		}							\
	static void __naked noinline __##name				\
		(ulong arg0, ulong arg1, ulong arg2)

#define ENTRY_FUNCTION(name, arg0, arg1, arg2)		\
		ENTRY_FUNCTION_HEAD(name, __barebox_arm_head, arg0, arg1, arg2)
#endif

/*
 * When using compressed images in conjunction with relocatable images
 * the PBL code must pick a suitable place where to uncompress the barebox
 * image. For doing this the PBL code must know the size of the final
 * image including the BSS segment. The BSS size is unknown to the PBL
 * code, so define a maximum BSS size here.
 */
#define MAX_BSS_SIZE SZ_1M

#define barebox_image_size (__image_end - __image_start)

#ifdef CONFIG_CPU_32
#define NAKED __naked
#else
/*
 * There is no naked support for aarch64, so do not rely on it.
 * This basically means we must have a stack configured when a
 * function with the naked attribute is entered. On nowadays hardware
 * the ROM should have some basic stack already. If not, set one
 * up before jumping into the barebox entry functions.
 */
#define NAKED
#endif

#endif	/* _BAREBOX_ARM_H_ */
