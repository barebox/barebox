#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#ifdef CONFIG_CPU_64v8

#define TCR_FLAGS		(TCR_TG0_4K | \
		TCR_SHARED_OUTER | \
		TCR_SHARED_INNER | \
		TCR_IRGN_WBWA | \
		TCR_ORGN_WBWA | \
		TCR_T0SZ(BITS_PER_VA))

#ifndef __ASSEMBLY__

static inline void set_ttbr_tcr_mair(int el, uint64_t table, uint64_t tcr, uint64_t attr)
{
	asm volatile("dsb sy");
	if (el == 1) {
		asm volatile("msr ttbr0_el1, %0" : : "r" (table) : "memory");
		asm volatile("msr tcr_el1, %0" : : "r" (tcr) : "memory");
		asm volatile("msr mair_el1, %0" : : "r" (attr) : "memory");
	} else if (el == 2) {
		asm volatile("msr ttbr0_el2, %0" : : "r" (table) : "memory");
		asm volatile("msr tcr_el2, %0" : : "r" (tcr) : "memory");
		asm volatile("msr mair_el2, %0" : : "r" (attr) : "memory");
	} else if (el == 3) {
		asm volatile("msr ttbr0_el3, %0" : : "r" (table) : "memory");
		asm volatile("msr tcr_el3, %0" : : "r" (tcr) : "memory");
		asm volatile("msr mair_el3, %0" : : "r" (attr) : "memory");
	} else {
		hang();
	}
	asm volatile("isb");
}

static inline uint64_t get_ttbr(int el)
{
	uint64_t val;
	if (el == 1) {
		asm volatile("mrs %0, ttbr0_el1" : "=r" (val));
	} else if (el == 2) {
		asm volatile("mrs %0, ttbr0_el2" : "=r" (val));
	} else if (el == 3) {
		asm volatile("mrs %0, ttbr0_el3" : "=r" (val));
	} else {
		hang();
	}

	return val;
}

void mmu_early_enable(uint64_t membase, uint64_t memsize, uint64_t _ttb);

#endif

#endif /* CONFIG_CPU_64v8 */

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
