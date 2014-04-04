#ifndef __ASM_ARM_MEMORY_H
#define __ASM_ARM_MEMORY_H

#include <memory.h>

#include <linux/const.h>
/*
 * Allow for constants defined here to be used from assembly code
 * by prepending the UL suffix only with actual C code compilation.
 */
#define UL(x) _AC(x, UL)

static inline void arm_add_mem_device(const char* name, resource_size_t start,
				    resource_size_t size)
{
	barebox_add_memory_bank(name, start, size);
}

#endif	/* __ASM_ARM_MEMORY_H */
