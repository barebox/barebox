#ifndef __ASM_MMU_H
#define __ASM_MMU_H

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <xfuncs.h>

#ifdef CONFIG_CPU_64v8
#include <asm/pgtable64.h>

#define DEV_MEM		(PMD_ATTRINDX(MT_DEVICE_nGnRnE) | PMD_SECT_AF | PMD_TYPE_SECT)
#define CACHED_MEM	(PMD_ATTRINDX(MT_NORMAL) | PMD_SECT_S | PMD_SECT_AF | PMD_TYPE_SECT)
#define UNCACHED_MEM	(PMD_ATTRINDX(MT_NORMAL_NC) | PMD_SECT_S | PMD_SECT_AF | PMD_TYPE_SECT)
#else
#include <asm/pgtable.h>

#define PMD_SECT_DEF_UNCACHED (PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT)
#define PMD_SECT_DEF_CACHED (PMD_SECT_WB | PMD_SECT_DEF_UNCACHED)
#endif



struct arm_memory;

void mmu_enable(void);
void mmu_disable(void);
static inline void arm_create_section(unsigned long virt, unsigned long phys, int size_m,
		unsigned int flags)
{
}

static inline void setup_dma_coherent(unsigned long offset)
{
}

#ifdef CONFIG_MMU
#define ARCH_HAS_REMAP
#define MAP_ARCH_DEFAULT MAP_CACHED
int arch_remap_range(void *_start, size_t size, unsigned flags);
void *map_io_sections(unsigned long physaddr, void *start, size_t size);
#else
#define MAP_ARCH_DEFAULT MAP_UNCACHED
static inline void *map_io_sections(unsigned long phys, void *start, size_t size)
{
	return (void *)phys;
}

#endif

#ifdef CONFIG_CACHE_L2X0
void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask);
#else
static inline void __init l2x0_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
}
#endif

struct outer_cache_fns {
	void (*inv_range)(unsigned long, unsigned long);
	void (*clean_range)(unsigned long, unsigned long);
	void (*flush_range)(unsigned long, unsigned long);
	void (*flush_all)(void);
	void (*disable)(void);
};

extern struct outer_cache_fns outer_cache;

void __dma_clean_range(unsigned long, unsigned long);
void __dma_flush_range(unsigned long, unsigned long);
void __dma_inv_range(unsigned long, unsigned long);

#endif /* __ASM_MMU_H */
