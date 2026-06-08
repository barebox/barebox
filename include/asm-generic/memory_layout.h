/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_GENERIC_MEMORY_LAYOUT_H
#define __ASM_GENERIC_MEMORY_LAYOUT_H

#ifdef CONFIG_MEMORY_LAYOUT_DEFAULT
#define STACK_BASE (TEXT_BASE - CONFIG_MALLOC_SIZE - CONFIG_STACK_SIZE)
#endif

#ifdef CONFIG_MEMORY_LAYOUT_FIXED
#define STACK_BASE CONFIG_STACK_BASE
#endif

#ifdef CONFIG_OPTEE_SIZE
#define OPTEE_SIZE CONFIG_OPTEE_SIZE
#else
#define OPTEE_SIZE 0
#endif

#ifdef CONFIG_OPTEE_SHM_SIZE
#define OPTEE_SHM_SIZE CONFIG_OPTEE_SHM_SIZE
#else
#define OPTEE_SHM_SIZE 0
#endif

#ifdef CONFIG_PBL_OPTEE_DTB_MAX_SIZE
#define PBL_OPTEE_DTB_MAX_SIZE CONFIG_PBL_OPTEE_DTB_MAX_SIZE
#else
#define PBL_OPTEE_DTB_MAX_SIZE 0
#endif

#define MALLOC_SIZE CONFIG_MALLOC_SIZE
#define STACK_SIZE  CONFIG_STACK_SIZE
#define SCRATCH_SIZE	CONFIG_SCRATCH_SIZE

#ifndef __ASSEMBLY__

#include <linux/pagemap.h>
#include <linux/minmax.h>
#include <linux/sizes.h>

/*
 * This generates a useless load from the specified symbol
 * to ensure linker garbage collection doesn't delete it
 */
#define __keep_symbolref(sym)	\
	__asm__ __volatile__("": :"r"(&sym) :)

#ifdef CONFIG_BAREBOX_MEMORY_OFFSET
static inline unsigned long barebox_malloc_base(unsigned long membase,
						unsigned long memsize)
 {
	 unsigned long offset = CONFIG_BAREBOX_MEMORY_OFFSET;

	 if (offset >= memsize)
		 offset = 0;

	 if (!offset)
		 offset = memsize - min_t(unsigned long, memsize / 2, SZ_1G);

	 return PAGE_ALIGN(membase + offset);
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __ASM_GENERIC_MEMORY_LAYOUT_H */
