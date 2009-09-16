#ifndef __ASM_MMU_H
#define __ASM_MMU_H

#include <asm/pgtable.h>
#include <malloc.h>

#define PMD_SECT_DEF_UNCACHED (PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | PMD_TYPE_SECT)
#define PMD_SECT_DEF_CACHED (PMD_SECT_WB | PMD_SECT_DEF_UNCACHED)

void mmu_init(void);
void mmu_enable(void);
void mmu_disable(void);
void arm_create_section(unsigned long virt, unsigned long phys, int size_m,
		unsigned int flags);

void setup_dma_coherent(unsigned long offset);

#ifdef CONFIG_MMU
void *dma_alloc_coherent(size_t size);
void dma_free_coherent(void *mem);

void dma_clean_range(const void *, const void *);
void dma_flush_range(const void *, const void *);
void dma_inv_range(const void *, const void *);
unsigned long virt_to_phys(void *virt);
void *phys_to_virt(unsigned long phys);

#else
static inline void *dma_alloc_coherent(size_t size)
{
	return malloc(size);
}

static inline void dma_free_coherent(void *mem)
{
	free(mem);
}

static inline void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}

static inline unsigned long virt_to_phys(void *mem)
{
	return (unsigned long)mem;
}

static inline void dma_clean_range(const void *s, const void *e)
{
}

static inline void dma_flush_range(const void *s, const void *e)
{
}

static inline void dma_inv_range(const void *s, const void *e)
{
}

#endif

#endif /* __ASM_MMU_H */

