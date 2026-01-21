/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2026 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix */

#ifndef __RISCV_CPU_MMU_H
#define __RISCV_CPU_MMU_H

#include <linux/types.h>

/*
 * RISC-V MMU constants for Sv39 (RV64) and Sv32 (RV32) page tables
 */

/* Page table configuration */
#define RISCV_PGSHIFT		12
#define RISCV_PGSIZE		(1UL << RISCV_PGSHIFT)  /* 4KB */

#ifdef CONFIG_64BIT
/* Sv39: 9-bit VPN fields, 512 entries per table */
#define RISCV_PGLEVEL_BITS	9
#define RISCV_PGTABLE_ENTRIES	512
#else
/* Sv32: 10-bit VPN fields, 1024 entries per table */
#define RISCV_PGLEVEL_BITS	10
#define RISCV_PGTABLE_ENTRIES	1024
#endif

/* Page table entry (PTE) bit definitions */
#define PTE_V		(1UL << 0)  /* Valid */
#define PTE_R		(1UL << 1)  /* Read */
#define PTE_W		(1UL << 2)  /* Write */
#define PTE_X		(1UL << 3)  /* Execute */
#define PTE_U		(1UL << 4)  /* User accessible */
#define PTE_G		(1UL << 5)  /* Global mapping */
#define PTE_A		(1UL << 6)  /* Accessed */
#define PTE_D		(1UL << 7)  /* Dirty */
#define PTE_RSW_MASK	(3UL << 8)  /* Reserved for software */

/* PTE physical page number (PPN) field position */
#define PTE_PPN_SHIFT	10

#ifdef CONFIG_64BIT
/*
 * Sv39: 39-bit virtual addressing, 3-level page tables
 * Virtual address format: [38:30] VPN[2], [29:21] VPN[1], [20:12] VPN[0], [11:0] offset
 */
#define RISCV_PGTABLE_LEVELS	3
#define VA_BITS			39
#else
/*
 * Sv32: 32-bit virtual addressing, 2-level page tables
 * Virtual address format: [31:22] VPN[1], [21:12] VPN[0], [11:0] offset
 */
#define RISCV_PGTABLE_LEVELS	2
#define VA_BITS			32
#endif

/* SATP register fields */
#ifdef CONFIG_64BIT
#define SATP_PPN_MASK		((1ULL << 44) - 1)  /* Physical page number (Sv39) */
#else
#define SATP_PPN_MASK		((1UL << 22) - 1)   /* Physical page number (Sv32) */
#endif

/* Extract VPN (Virtual Page Number) from virtual address */
#define VPN_MASK		((1UL << RISCV_PGLEVEL_BITS) - 1)
#define VPN(addr, level)	(((addr) >> (RISCV_PGSHIFT + (level) * RISCV_PGLEVEL_BITS)) & VPN_MASK)

/* RISC-V page sizes by level */
#ifdef CONFIG_64BIT
/* Sv39: 3-level page tables */
#define RISCV_L2_SHIFT		30	/* 1GB gigapages (Level 0 in Sv39) */
#define RISCV_L1_SHIFT		21	/* 2MB megapages (Level 1 in Sv39) */
#define RISCV_L0_SHIFT		12	/* 4KB pages (Level 2 in Sv39) */
#else
/* Sv32: 2-level page tables */
#define RISCV_L1_SHIFT		22	/* 4MB superpages (Level 0 in Sv32) */
#define RISCV_L0_SHIFT		12	/* 4KB pages (Level 1 in Sv32) */
#endif

#ifdef CONFIG_64BIT
#define RISCV_L2_SIZE		(1UL << RISCV_L2_SHIFT)	/* 1GB (RV64 only) */
#endif
#define RISCV_L1_SIZE		(1UL << RISCV_L1_SHIFT)	/* 2MB (RV64) or 4MB (RV32) */
#define RISCV_L0_SIZE		(1UL << RISCV_L0_SHIFT)	/* 4KB */

/* Number of entries per page table (use RISCV_PGTABLE_ENTRIES instead) */
#define RISCV_PTE_ENTRIES	RISCV_PGTABLE_ENTRIES

/* PTE type - 64-bit on RV64, 32-bit on RV32 */
#ifdef CONFIG_64BIT
typedef uint64_t pte_t;
#else
typedef uint32_t pte_t;
#endif

/* Early page table allocation size (PBL) */
#ifdef CONFIG_64BIT
/* Sv39: 3 levels, allocate space for root + worst case intermediate tables */
#define RISCV_EARLY_PAGETABLE_SIZE	(64 * 1024)  /* 64KB */
#else
/* Sv32: 2 levels, smaller allocation */
#define RISCV_EARLY_PAGETABLE_SIZE	(32 * 1024)  /* 32KB */
#endif

#ifndef __ASSEMBLY__

/* SFENCE.VMA - Synchronize updates to page tables */
static inline void sfence_vma(void)
{
	__asm__ __volatile__ ("sfence.vma" : : : "memory");
}

static inline void sfence_vma_addr(unsigned long addr)
{
	__asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

#endif /* __ASSEMBLY__ */

#endif /* __RISCV_CPU_MMU_H */
