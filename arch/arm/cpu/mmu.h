#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#ifdef CONFIG_MMU
void __mmu_cache_on(void);
void __mmu_cache_off(void);
void __mmu_cache_flush(void);
#else
static inline void __mmu_cache_on(void) {}
static inline void __mmu_cache_off(void) {}
static inline void __mmu_cache_flush(void) {}
#endif

#endif /* __ARM_MMU_H */
