#include <common.h>
#include <asm/mmu.h>
#include <errno.h>
#include <linux/sizes.h>
#include <asm/memory.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <asm/pgtable.h>

#include "mmu.h"

static uint32_t *ttb;

static void create_sections(unsigned long addr, int size_m, unsigned int flags)
{
	int i;

	addr >>= 20;

	for (i = size_m; i > 0; i--, addr++)
		ttb[addr] = (addr << 20) | flags;
}

static void map_cachable(unsigned long start, unsigned long size)
{
	start &= ~(SZ_1M - 1);
	size = (size + (SZ_1M - 1)) & ~(SZ_1M - 1);

	create_sections(start, size >> 20, PMD_SECT_AP_WRITE |
			PMD_SECT_AP_READ | PMD_TYPE_SECT | PMD_SECT_WB);
}

void mmu_early_enable(unsigned long membase, unsigned long memsize,
		      unsigned long _ttb)
{
	int i;

	ttb = (uint32_t *)_ttb;

	arm_set_cache_functions();

	/* Set the ttb register */
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb) /*:*/);

	/* Set the Domain Access Control Register */
	i = 0x3;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	create_sections(0, 4096, PMD_SECT_AP_WRITE |
			PMD_SECT_AP_READ | PMD_TYPE_SECT);

	map_cachable(membase, memsize);

	__mmu_cache_on();
}
