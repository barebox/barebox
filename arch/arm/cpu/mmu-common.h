#ifndef __ARM_MMU_COMMON_H
#define __ARM_MMU_COMMON_H

void dma_inv_range(void *ptr, size_t size);
void *dma_alloc_map(size_t size, dma_addr_t *dma_handle, unsigned flags);
void __mmu_init(bool mmu_on);

#endif