/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KASAN_H
#define _LINUX_KASAN_H

#include <linux/types.h>

/*
 * On 64bit architectures tlsf aligns all allocations to a 64bit
 * boundary, otherwise they are only 32bit aligned.
 */
#ifdef CONFIG_64BIT
#define KASAN_SHADOW_SCALE_SHIFT	3
#else
#define KASAN_SHADOW_SCALE_SHIFT	2
#endif

#define KASAN_TAG_KERNEL	0xFF /* native kernel pointers tag */
#define KASAN_TAG_INVALID	0xFE /* inaccessible memory tag */
#define KASAN_TAG_MAX		0xFD /* maximum value for random tags */

#define KASAN_FREE_PAGE         0xFF  /* page was freed */
#define KASAN_PAGE_REDZONE      0xFE  /* redzone for kmalloc_large allocations */
#define KASAN_KMALLOC_REDZONE   0xFC  /* redzone inside slub object */
#define KASAN_KMALLOC_FREE      0xFB  /* object was freed (kmem_cache_free/kfree) */
#define KASAN_KMALLOC_FREETRACK 0xFA  /* object was freed and has free track set */

#define KASAN_GLOBAL_REDZONE    0xF9  /* redzone for global variable */
#define KASAN_DMA_DEV_MAPPED    0xF8  /* DMA-mapped buffer currently owned by device */

/*
 * Stack redzone shadow values
 * (Those are compiler's ABI, don't change them)
 */
#define KASAN_STACK_LEFT        0xF1
#define KASAN_STACK_MID         0xF2
#define KASAN_STACK_RIGHT       0xF3
#define KASAN_STACK_PARTIAL     0xF4

/*
 * alloca redzone shadow values
 */
#define KASAN_ALLOCA_LEFT	0xCA
#define KASAN_ALLOCA_RIGHT	0xCB

extern unsigned long kasan_shadow_start;
extern unsigned long kasan_shadow_base;

#if defined(CONFIG_KASAN) && !defined(__PBL__)

static inline void *kasan_mem_to_shadow(const void *addr)
{
	unsigned long a = (unsigned long)addr;

	a -= kasan_shadow_start;
	a >>= KASAN_SHADOW_SCALE_SHIFT;
	a += kasan_shadow_base;

	return (void *)a;
}

void kasan_init(unsigned long membase, unsigned long memsize,
		unsigned long shadow_base);

/* Enable reporting bugs after kasan_disable_current() */
extern void kasan_enable_current(void);

/* Disable reporting bugs for current task */
extern void kasan_disable_current(void);

void kasan_poison_shadow(const void *address, size_t size, u8 value);
void kasan_unpoison_shadow(const void *address, size_t size);
int kasan_is_poisoned_shadow(const void *address, size_t size);

bool kasan_save_enable_multi_shot(void);
void kasan_restore_multi_shot(bool enabled);

#else /* CONFIG_KASAN */

static inline void kasan_poison_shadow(const void *address, size_t size, u8 value) {}
static inline void kasan_unpoison_shadow(const void *address, size_t size) {}
static inline int kasan_is_poisoned_shadow(const void *address, size_t size) { return -1; }

static inline void kasan_enable_current(void) {}
static inline void kasan_disable_current(void) {}

static inline void kasan_init(unsigned long membase, unsigned long memsize,
		unsigned long shadow_base) {}

#endif /* CONFIG_KASAN */

#endif /* LINUX_KASAN_H */
