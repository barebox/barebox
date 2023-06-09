/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_MEMORY_H
#define __ASM_ARM_MEMORY_H

#include <memory.h>

#include <linux/const.h>

static inline int arm_add_mem_device(const char* name, resource_size_t start,
				     resource_size_t size)
{
	return barebox_add_memory_bank(name, start, size);
}

#endif	/* __ASM_ARM_MEMORY_H */
