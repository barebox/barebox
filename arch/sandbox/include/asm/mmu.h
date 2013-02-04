#ifndef __ASM_MMU_H
#define __ASM_MMU_H

static inline void remap_range(void *_start, size_t size, uint32_t flags)
{
}

static inline uint32_t mmu_get_pte_cached_flags(void)
{
	return 0;
}

static inline uint32_t mmu_get_pte_uncached_flags(void)
{
	return 0;
}

#endif /* __ASM_MMU_H */
