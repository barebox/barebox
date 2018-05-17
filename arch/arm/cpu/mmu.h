#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#include <asm/pgtable.h>
#include <linux/sizes.h>

#ifdef CONFIG_MMU
void __mmu_cache_on(void);
void __mmu_cache_off(void);
void __mmu_cache_flush(void);
#else
static inline void __mmu_cache_on(void) {}
static inline void __mmu_cache_off(void) {}
static inline void __mmu_cache_flush(void) {}
#endif

static inline void set_ttbr(void *ttb)
{
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);
}

#define DOMAIN_MANAGER	3

static inline void set_domain(unsigned val)
{
	/* Set the Domain Access Control Register */
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(val) /*:*/);
}

static inline void
create_sections(uint32_t *ttb, unsigned long first,
		unsigned long last, unsigned int flags)
{
	unsigned long ttb_start = first >> 20;
	unsigned long ttb_end = (last >> 20) + 1;
	unsigned int i, addr = first;

	for (i = ttb_start; i < ttb_end; i++) {
		ttb[i] = addr | flags;
		addr += SZ_1M;
	}
}

#define PMD_SECT_DEF_UNCACHED (PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT)
#define PMD_SECT_DEF_CACHED (PMD_SECT_WB | PMD_SECT_DEF_UNCACHED)

static inline void create_flat_mapping(uint32_t *ttb)
{
	/* create a flat mapping using 1MiB sections */
	create_sections(ttb, 0, 0xffffffff, PMD_SECT_DEF_UNCACHED);
}

#endif /* __ARM_MMU_H */
