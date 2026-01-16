/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_MMU_H
#define __ASM_MMU_H

#include <linux/types.h>

/*
 * RISC-V supports memory protection through two mechanisms:
 * - S-mode: Virtual memory with page tables (MMU)
 * - M-mode: Physical Memory Protection (PMP) regions
 */

#ifdef CONFIG_MMU
#define ARCH_HAS_REMAP
#define MAP_ARCH_DEFAULT MAP_CACHED

/* Architecture-specific memory type flags */
#define ARCH_MAP_CACHED_RWX		MAP_ARCH(2)	/* Cached, RWX (early boot) */
#define ARCH_MAP_FLAG_PAGEWISE		(1 << 16)	/* Force page-wise mapping */

/*
 * Remap a virtual address range with specified memory type (barebox proper).
 * Used by the generic remap infrastructure after barebox is fully relocated.
 * Implementation is in arch/riscv/cpu/mmu.c (S-mode) or pmp.c (M-mode).
 */
int arch_remap_range(void *virt_addr, phys_addr_t phys_addr, size_t size,
		     maptype_t map_type);

#else
#define MAP_ARCH_DEFAULT MAP_UNCACHED
#endif

/*
 * Early MMU/PMP setup - called before decompression for performance.
 * S-mode: Sets up basic page tables and enables MMU via SATP CSR.
 * M-mode: Configures initial PMP regions.
 */
void mmu_early_enable(unsigned long membase, unsigned long memsize,
		      unsigned long barebox_base);

#include <mmu.h>

#endif /* __ASM_MMU_H */
