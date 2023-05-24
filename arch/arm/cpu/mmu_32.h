/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#include <asm/pgtable.h>
#include <linux/sizes.h>
#include <asm/system_info.h>

#include "mmu-common.h"

#define PGDIR_SHIFT	20
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)

#define pgd_index(addr)		((addr) >> PGDIR_SHIFT)

#ifdef CONFIG_MMU
void __mmu_cache_on(void);
void __mmu_cache_off(void);
void __mmu_cache_flush(void);
#else
static inline void __mmu_cache_on(void) {}
static inline void __mmu_cache_off(void) {}
static inline void __mmu_cache_flush(void) {}
#endif

static inline unsigned long get_ttbr(void)
{
	unsigned long ttb;

	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r"(ttb));

	return ttb;
}

static inline void set_ttbr(void *ttb)
{
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);
}

#define DOMAIN_CLIENT	1
#define DOMAIN_MANAGER	3

static inline unsigned long get_domain(void)
{
	unsigned long dacr;

	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r"(dacr));

	return dacr;
}

static inline void set_domain(unsigned val)
{
	/* Set the Domain Access Control Register */
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(val) /*:*/);
}

#define PMD_SECT_DEF_UNCACHED (PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT)
#define PMD_SECT_DEF_CACHED (PMD_SECT_WB | PMD_SECT_DEF_UNCACHED)

static inline unsigned long attrs_uncached_mem(void)
{
	unsigned int flags = PMD_SECT_DEF_UNCACHED;

	if (cpu_architecture() >= CPU_ARCH_ARMv7)
		flags |= PMD_SECT_XN;

	return flags;
}

#endif /* __ARM_MMU_H */
