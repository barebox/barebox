/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_PAGEMAP_H
#define _LINUX_PAGEMAP_H

#include <linux/kernel.h>

/*
 * Copyright 1995 Linus Torvalds
 */


#define PAGE_SIZE	4096
#define PAGE_SHIFT	12
#define PAGE_MASK	(PAGE_SIZE - 1)
#define PAGE_ALIGN(s)	ALIGN(s, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(x) ALIGN_DOWN(x, PAGE_SIZE)

#define PAGE_CACHE_SHIFT        PAGE_SHIFT
#define PAGE_CACHE_SIZE         PAGE_SIZE
#define PAGE_CACHE_MASK         PAGE_MASK

#endif /* _LINUX_PAGEMAP_H */
