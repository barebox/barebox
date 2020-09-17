/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MM_KASAN_KASAN_H
#define __MM_KASAN_KASAN_H

#include <linux/kasan.h>
#include <linux/linkage.h>

#define KASAN_SHADOW_SCALE_SIZE (1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK       (KASAN_SHADOW_SCALE_SIZE - 1)

#define KASAN_ALLOCA_REDZONE_SIZE	32

/*
 * Stack frame marker (compiler ABI).
 */
#define KASAN_CURRENT_STACK_FRAME_MAGIC 0x41B58AB3

/* Don't break randconfig/all*config builds */
#ifndef KASAN_ABI_VERSION
#define KASAN_ABI_VERSION 1
#endif

struct kasan_access_info {
	const void *access_addr;
	const void *first_bad_addr;
	size_t access_size;
	bool is_write;
	unsigned long ip;
};

/* The layout of struct dictated by compiler */
struct kasan_source_location {
	const char *filename;
	int line_no;
	int column_no;
};

/* The layout of struct dictated by compiler */
struct kasan_global {
	const void *beg;		/* Address of the beginning of the global variable. */
	size_t size;			/* Size of the global variable. */
	size_t size_with_redzone;	/* Size of the variable + size of the red zone. 32 bytes aligned */
	const void *name;
	const void *module_name;	/* Name of the module where the global variable is declared. */
	unsigned long has_dynamic_init;	/* This needed for C++ */
#if KASAN_ABI_VERSION >= 4
	struct kasan_source_location *location;
#endif
#if KASAN_ABI_VERSION >= 5
	char *odr_indicator;
#endif
};

static inline const void *kasan_shadow_to_mem(const void *shadow_addr)
{
	unsigned long sa = (unsigned long)shadow_addr;

	sa -= kasan_shadow_base;
	sa <<= KASAN_SHADOW_SCALE_SHIFT;
	sa += kasan_shadow_start;

	return (void *)sa;
}

static inline bool addr_has_shadow(const void *addr)
{
	return (addr >= (void *)kasan_shadow_start);
}

/**
 * check_memory_region - Check memory region, and report if invalid access.
 * @addr: the accessed address
 * @size: the accessed size
 * @write: true if access is a write access
 * @ret_ip: return address
 * @return: true if access was valid, false if invalid
 */
bool check_memory_region(unsigned long addr, size_t size, bool write,
				unsigned long ret_ip);

void *find_first_bad_addr(void *addr, size_t size);
const char *get_bug_type(struct kasan_access_info *info);

bool kasan_report(unsigned long addr, size_t size,
		bool is_write, unsigned long ip);
void kasan_report_invalid_free(void *object, unsigned long ip);

#ifndef arch_kasan_set_tag
static inline const void *arch_kasan_set_tag(const void *addr, u8 tag)
{
	return addr;
}
#endif
#ifndef arch_kasan_reset_tag
#define arch_kasan_reset_tag(addr)	((void *)(addr))
#endif
#ifndef arch_kasan_get_tag
#define arch_kasan_get_tag(addr)	0
#endif

#define set_tag(addr, tag)	((void *)arch_kasan_set_tag((addr), (tag)))
#define reset_tag(addr)		((void *)arch_kasan_reset_tag(addr))
#define get_tag(addr)		arch_kasan_get_tag(addr)

/*
 * Exported functions for interfaces called from assembly or from generated
 * code. Declarations here to avoid warning about missing declarations.
 */
asmlinkage void kasan_unpoison_task_stack_below(const void *watermark);
void __asan_register_globals(struct kasan_global *globals, size_t size);
void __asan_unregister_globals(struct kasan_global *globals, size_t size);
void __asan_handle_no_return(void);
void __asan_alloca_poison(unsigned long addr, size_t size);
void __asan_allocas_unpoison(const void *stack_top, const void *stack_bottom);

void __asan_load1(unsigned long addr);
void __asan_store1(unsigned long addr);
void __asan_load2(unsigned long addr);
void __asan_store2(unsigned long addr);
void __asan_load4(unsigned long addr);
void __asan_store4(unsigned long addr);
void __asan_load8(unsigned long addr);
void __asan_store8(unsigned long addr);
void __asan_load16(unsigned long addr);
void __asan_store16(unsigned long addr);
void __asan_loadN(unsigned long addr, size_t size);
void __asan_storeN(unsigned long addr, size_t size);

void __asan_load1_noabort(unsigned long addr);
void __asan_store1_noabort(unsigned long addr);
void __asan_load2_noabort(unsigned long addr);
void __asan_store2_noabort(unsigned long addr);
void __asan_load4_noabort(unsigned long addr);
void __asan_store4_noabort(unsigned long addr);
void __asan_load8_noabort(unsigned long addr);
void __asan_store8_noabort(unsigned long addr);
void __asan_load16_noabort(unsigned long addr);
void __asan_store16_noabort(unsigned long addr);
void __asan_loadN_noabort(unsigned long addr, size_t size);
void __asan_storeN_noabort(unsigned long addr, size_t size);

void __asan_report_load1_noabort(unsigned long addr);
void __asan_report_store1_noabort(unsigned long addr);
void __asan_report_load2_noabort(unsigned long addr);
void __asan_report_store2_noabort(unsigned long addr);
void __asan_report_load4_noabort(unsigned long addr);
void __asan_report_store4_noabort(unsigned long addr);
void __asan_report_load8_noabort(unsigned long addr);
void __asan_report_store8_noabort(unsigned long addr);
void __asan_report_load16_noabort(unsigned long addr);
void __asan_report_store16_noabort(unsigned long addr);
void __asan_report_load_n_noabort(unsigned long addr, size_t size);
void __asan_report_store_n_noabort(unsigned long addr, size_t size);

void __asan_set_shadow_00(const void *addr, size_t size);
void __asan_set_shadow_f1(const void *addr, size_t size);
void __asan_set_shadow_f2(const void *addr, size_t size);
void __asan_set_shadow_f3(const void *addr, size_t size);
void __asan_set_shadow_f5(const void *addr, size_t size);
void __asan_set_shadow_f8(const void *addr, size_t size);

extern int kasan_depth;

#endif
