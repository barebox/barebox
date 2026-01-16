// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2026 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "mmu: " fmt

#include <common.h>
#include <init.h>
#include <mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <linux/bitops.h>
#include <asm/sections.h>
#include <asm/csr.h>

#include "mmu.h"

/*
 * Page table storage for early MMU setup in PBL.
 * Static allocation before BSS is available.
 */
static char early_pt_storage[RISCV_EARLY_PAGETABLE_SIZE] __aligned(RISCV_PGSIZE);
static unsigned int early_pt_idx;

/*
 * Allocate a page table from the early PBL storage
 */
static pte_t *alloc_pte(void)
{
	pte_t *pt;

	if ((early_pt_idx + 1) * RISCV_PGSIZE >= RISCV_EARLY_PAGETABLE_SIZE) {
		pr_err("Out of early page table memory (need more than %d KB)\n",
		       RISCV_EARLY_PAGETABLE_SIZE / 1024);
		hang();
	}

	pt = (pte_t *)(early_pt_storage + early_pt_idx * RISCV_PGSIZE);
	early_pt_idx++;

	/* Clear the page table */
	memset(pt, 0, RISCV_PGSIZE);

	return pt;
}

/*
 * split_pte - Split a megapage/gigapage PTE into a page table
 * @pte: Pointer to the PTE to split
 * @level: Current page table level (0-2 for Sv39)
 *
 * This function takes a leaf PTE (megapage/gigapage) and converts it into
 * a page table pointer with 512 entries, each covering 1/512th of the
 * original range with identical permissions.
 *
 * Example: A 2MB megapage at Level 1 becomes a Level 2 page table with
 * 512 × 4KB pages, all with the same R/W/X attributes.
 */
static void split_pte(pte_t *pte, int level)
{
	pte_t old_pte = *pte;
	pte_t *new_table;
	pte_t phys_base;
	pte_t attrs;
	unsigned long granularity;
	int i;

	/* If already a table pointer (no RWX bits), nothing to do */
	if (!(*pte & (PTE_R | PTE_W | PTE_X)))
		return;

	/* Allocate new page table (512 entries × 8 bytes = 4KB) */
	new_table = alloc_pte();

	/* Extract physical base address from old PTE */
	phys_base = (old_pte >> PTE_PPN_SHIFT) << RISCV_PGSHIFT;

	/* Extract permission attributes to replicate */
	attrs = old_pte & (PTE_R | PTE_W | PTE_X | PTE_A | PTE_D | PTE_U | PTE_G);

	/*
	 * Calculate granularity of child level.
	 * Level 0 (1GB) → Level 1 (2MB): granularity = 2MB = 1 << 21
	 * Level 1 (2MB) → Level 2 (4KB): granularity = 4KB = 1 << 12
	 *
	 * Formula: granularity = 1 << (12 + 9 * (Levels - 2 - level))
	 * For Sv39 (3 levels):
	 *   level=0: 1 << (12 + 9*1) = 2MB
	 *   level=1: 1 << (12 + 9*0) = 4KB
	 */
	granularity = 1UL << (RISCV_PGSHIFT + RISCV_PGLEVEL_BITS *
			      (RISCV_PGTABLE_LEVELS - 2 - level));

	/* Populate new table: replicate old mapping across 512 entries */
	for (i = 0; i < RISCV_PTE_ENTRIES; i++) {
		unsigned long new_phys = phys_base + (i * granularity);
		pte_t new_pte = ((new_phys >> RISCV_PGSHIFT) << PTE_PPN_SHIFT) |
				attrs | PTE_V;
		new_table[i] = new_pte;
	}

	/*
	 * Replace old leaf PTE with table pointer.
	 * No RWX bits = pointer to next level.
	 */
	*pte = (((unsigned long)new_table >> RISCV_PGSHIFT) << PTE_PPN_SHIFT) | PTE_V;

	pr_debug("Split level %d PTE at phys=0x%llx (granularity=%lu KB)\n",
		 level, (unsigned long long)phys_base, granularity / 1024);
}

/*
 * Get the root page table base
 */
static pte_t *get_ttb(void)
{
	return (pte_t *)early_pt_storage;
}

/*
 * Convert maptype flags to PTE permission bits
 */
static unsigned long flags_to_pte(maptype_t flags)
{
	unsigned long pte = PTE_V;  /* Valid bit always set */

	/*
	 * Map barebox memory types to RISC-V PTE flags:
	 * - ARCH_MAP_CACHED_RWX: read + write + execute (early boot, full RAM access)
	 * - MAP_CODE: read + execute (text sections)
	 * - MAP_CACHED_RO: read only (rodata sections)
	 * - MAP_CACHED: read + write (data/bss sections)
	 * - MAP_UNCACHED: read + write, uncached (device memory)
	 */
	switch (flags & MAP_TYPE_MASK) {
	case ARCH_MAP_CACHED_RWX:
		/* Full access for early boot: R + W + X */
		pte |= PTE_R | PTE_W | PTE_X;
		break;
	case MAP_CACHED_RO:
		/* Read-only data: R, no W, no X */
		pte |= PTE_R;
		break;
	case MAP_CODE:
		/* Code: R + X, no W */
		pte |= PTE_R | PTE_X;
		break;
	case MAP_CACHED: /* TODO: implement */
	case MAP_UNCACHED:
	default:
		/* Data or uncached: R + W, no X */
		pte |= PTE_R | PTE_W;
		break;
	}

	/* Set accessed and dirty bits to avoid hardware updates */
	pte |= PTE_A | PTE_D;

	return pte;
}

/*
 * Walk page tables and get/create PTE for given address at specified level
 */
static pte_t *walk_pgtable(unsigned long addr, int target_level)
{
	pte_t *table = get_ttb();
	int level;

	for (level = 0; level < target_level; level++) {
		unsigned int index = VPN(addr, RISCV_PGTABLE_LEVELS - 1 - level);
		pte_t *pte = &table[index];

		if (!(*pte & PTE_V)) {
			/* Entry not valid - allocate new page table */
			pte_t *new_table = alloc_pte();
			pte_t new_pte = ((unsigned long)new_table >> RISCV_PGSHIFT) << PTE_PPN_SHIFT;
			new_pte |= PTE_V;
			*pte = new_pte;
			table = new_table;
		} else if (*pte & (PTE_R | PTE_W | PTE_X)) {
			/* This is a leaf PTE - split it before descending */
			split_pte(pte, level);
			/* After split, PTE is now a table pointer - follow it */
			table = (pte_t *)(((*pte >> PTE_PPN_SHIFT) << RISCV_PGSHIFT));
		} else {
			/* Valid non-leaf PTE - follow to next level */
			table = (pte_t *)(((*pte >> PTE_PPN_SHIFT) << RISCV_PGSHIFT));
		}
	}

	return table;
}

/*
 * Create a page table entry mapping virt -> phys with given permissions
 */
static void create_pte(unsigned long virt, phys_addr_t phys, maptype_t flags)
{
	pte_t *table;
	unsigned int index;
	pte_t pte;

	/* Walk to leaf level page table */
	table = walk_pgtable(virt, RISCV_PGTABLE_LEVELS - 1);

	/* Get index for this address at leaf level */
	index = VPN(virt, 0);

	/* Build PTE: PPN + flags */
	pte = (phys >> RISCV_PGSHIFT) << PTE_PPN_SHIFT;
	pte |= flags_to_pte(flags);

	/* Write PTE */
	table[index] = pte;
}

/*
 * create_megapage - Create a 2MB megapage mapping
 * @virt: Virtual address (should be 2MB-aligned)
 * @phys: Physical address (should be 2MB-aligned)
 * @flags: Mapping flags (MAP_CACHED, etc.)
 *
 * Creates a leaf PTE at Level 1 covering 2MB. This is identical to a 4KB
 * PTE except it's placed at Level 1 instead of Level 2, saving page tables.
 */
static void create_megapage(unsigned long virt, phys_addr_t phys, maptype_t flags)
{
	pte_t *table;
	unsigned int index;
	pte_t pte;

	/* Walk to Level 1 (one level above 4KB leaf) */
	table = walk_pgtable(virt, RISCV_PGTABLE_LEVELS - 2);

	/* Get VPN[1] index for this address at Level 1 */
	index = VPN(virt, 1);

	/* Build leaf PTE at Level 1: PPN + RWX flags make it a megapage */
	pte = (phys >> RISCV_PGSHIFT) << PTE_PPN_SHIFT;
	pte |= flags_to_pte(flags);

	/* Write megapage PTE */
	table[index] = pte;
}

/*
 * mmu_early_enable - Set up initial MMU with identity mapping
 *
 * Called before barebox decompression to enable caching for faster decompression.
 * Creates a simple identity map of all RAM with RWX permissions.
 */
void mmu_early_enable(unsigned long membase, unsigned long memsize,
		      unsigned long barebox_base)
{
	unsigned long addr;
	unsigned long end = membase + memsize;
	unsigned long satp;

	pr_debug("Enabling MMU: mem=0x%08lx-0x%08lx barebox=0x%08lx\n",
		 membase, end, barebox_base);

	/* Reset page table allocator */
	early_pt_idx = 0;

	/* Allocate root page table */
	(void)alloc_pte();

	pr_debug("Creating flat identity mapping...\n");

	/*
	 * Create a flat identity mapping of the lower address space as uncached.
	 * This ensures I/O devices (UART, etc.) are accessible after MMU is enabled.
	 * RV64: Map lower 4GB using 2MB megapages (2048 entries).
	 * RV32: Map entire 4GB using 4MB superpages (1024 entries in root table).
	 */
	addr = 0;
	do {
		create_megapage(addr, addr, MAP_UNCACHED);
		addr += RISCV_L1_SIZE;
	} while (lower_32_bits(addr) != 0);  /* Wraps around to 0 after 0xFFFFFFFF */

	/*
	 * Remap RAM as cached with RWX permissions using superpages.
	 * This overwrites the uncached mappings for RAM regions, providing
	 * better performance. Later, pbl_mmu_setup_from_elf() will split
	 * superpages as needed to set fine-grained permissions based on ELF segments.
	 */
	pr_debug("Remapping RAM 0x%08lx-0x%08lx as cached RWX...\n", membase, end);
	for (addr = membase; addr < end; addr += RISCV_L1_SIZE)
		create_megapage(addr, addr, ARCH_MAP_CACHED_RWX);

	pr_debug("Page table setup complete, used %lu KB\n",
		 (early_pt_idx * RISCV_PGSIZE) / 1024);

	/*
	 * Enable MMU by setting SATP CSR:
	 * - MODE field: Sv39 (RV64) or Sv32 (RV32)
	 * - ASID: 0 (no address space ID)
	 * - PPN: physical address of root page table
	 */
	satp = SATP_MODE | (((unsigned long)get_ttb() >> RISCV_PGSHIFT) & SATP_PPN_MASK);

	pr_debug("Enabling MMU: SATP=0x%08lx\n", satp);

	/* Synchronize before enabling MMU */
	sfence_vma();

	/* Enable MMU */
	csr_write(satp, satp);

	/* Synchronize after enabling MMU */
	sfence_vma();

	pr_debug("MMU enabled with %lu %spages for RAM\n",
		 (memsize / RISCV_L1_SIZE),
		 IS_ENABLED(CONFIG_64BIT) ? "2MB mega" : "4MB super");
}

/*
 * arch_remap_range - Remap a virtual address range (barebox proper)
 *
 * This is the non-PBL version used in barebox proper after full relocation.
 * Currently provides basic remapping support. For full MMU management in
 * barebox proper, this would need to be extended with:
 * - Dynamic page table allocation
 * - Cache flushing for non-cached mappings
 * - TLB management
 * - Support for MAP_FAULT (guard pages)
 */
int arch_remap_range(void *virt, phys_addr_t phys, size_t size,
		     maptype_t map_type)
{
	unsigned long addr = (unsigned long)virt;
	unsigned long end = addr + size;

	pr_debug("Remapping 0x%p-0x%08lx -> 0x%pap (flags=0x%x)\n",
		 virt, end, &phys, map_type);

	/* Align to page boundaries */
	addr &= ~(RISCV_PGSIZE - 1);
	end = ALIGN(end, RISCV_PGSIZE);

	/* Create page table entries for each page in the range */
	while (addr < end) {
		create_pte(addr, phys, map_type);
		addr += RISCV_PGSIZE;
		phys += RISCV_PGSIZE;
	}

	/* Flush TLB for the remapped range */
	sfence_vma();

	return 0;
}
