/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PBL_MMU_H
#define __PBL_MMU_H

#include <linux/types.h>

struct elf_image;

/**
 * pbl_mmu_setup_from_elf() - Configure MMU using ELF segment information
 * @elf: ELF image structure from elf_open_binary_into()
 * @membase: Base address of RAM
 * @memsize: Size of RAM
 *
 * This function sets up the MMU with proper permissions based on ELF
 * segment flags. It should be called after elf_load_inplace() has
 * relocated the barebox image.
 *
 * Segment permissions are mapped as follows:
 *   PF_R | PF_X  -> Read-only + executable (text)
 *   PF_R | PF_W  -> Read-write (data, bss)
 *   PF_R         -> Read-only (rodata)
 *
 * Return: 0 on success, negative error code on failure
 */
#if IS_ENABLED(CONFIG_MMU)
int pbl_mmu_setup_from_elf(struct elf_image *elf, unsigned long membase,
			    unsigned long memsize);
#else
static inline int pbl_mmu_setup_from_elf(struct elf_image *elf,
					 unsigned long membase,
					 unsigned long memsize)
{
	return 0;
}
#endif

#endif /* __PBL_MMU_H */
