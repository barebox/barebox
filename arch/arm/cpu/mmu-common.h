#ifndef __ARM_MMU_COMMON_H
#define __ARM_MMU_COMMON_H

void dma_inv_range(void *ptr, size_t size);
void dma_flush_range(void *ptr, size_t size);
void *dma_alloc_map(size_t size, dma_addr_t *dma_handle, unsigned flags);
void __mmu_init(bool mmu_on);

static inline void arm_mmu_not_initialized_error(void)
{
	/*
	 * This means:
	 * - one of the MMU functions like dma_alloc_coherent
	 *   or remap_range is called too early, before the MMU is initialized
	 * - Or the MMU initialization has failed earlier
	 */
	panic("MMU not initialized\n");
}

#endif