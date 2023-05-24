/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __ZERO_PAGE_H
#define __ZERO_PAGE_H

#include <common.h>

#if defined CONFIG_ARCH_HAS_ZERO_PAGE && defined CONFIG_MMU && !defined __PBL__

/*
 * zero_page_faulting - fault when accessing the zero page
 */
void zero_page_faulting(void);

/*
 * zero_page_access - allow accesses to the zero page
 *
 * Disable the null pointer trap on the zero page if access to the zero page
 * is actually required. Disable the trap with care and re-enable it
 * immediately after the access to properly trap null pointers.
 */
void zero_page_access(void);

void zero_page_access(void);

static inline bool zero_page_remappable(void)
{
	return true;
}

#else

static inline void zero_page_faulting(void)
{
}

static inline void zero_page_access(void)
{
}

static inline bool zero_page_remappable(void)
{
	return false;
}

#endif

static inline bool zero_page_contains(unsigned long addr)
{
	return addr < PAGE_SIZE;
}

/*
 * zero_page_memcpy - copy to or from an address located in the zero page
 */
static inline void *zero_page_memcpy(void *dest, const void *src, size_t count)
{
	void *ret;

	OPTIMIZER_HIDE_VAR(dest);

	zero_page_access();
	ret = memcpy(dest, src, count);
	zero_page_faulting();

	return ret;
}

#endif /* __ZERO_PAGE_H */
