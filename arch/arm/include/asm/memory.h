/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_MEMORY_H
#define __ASM_ARM_MEMORY_H

#include <linux/sizes.h>

#ifndef __ASSEMBLY__
#include <memory.h>

#include <linux/const.h>

static inline int arm_add_mem_device(const char* name, resource_size_t start,
				     resource_size_t size)
{
	return barebox_add_memory_bank(name, start, size);
}

#endif


/*
 * Alignment of barebox PBL segments (e.g. .text, .data).
 *
 *  4  B granule:  Same flat rwx mapping for everything
 *  4 KB granule:  16 level 3 entries, with contiguous bit
 * 16 KB granule:   4 level 3 entries, without contiguous bit
 * 64 KB granule:   1 level 3 entry
 */
#ifdef CONFIG_EFI_PAYLOAD
#define PBL_SEGMENT_ALIGN		SZ_64K
#else
#define PBL_SEGMENT_ALIGN		4
#endif

#endif	/* __ASM_ARM_MEMORY_H */
