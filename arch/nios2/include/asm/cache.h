/*
 * Copyright (C) 2003 Microtronix Datacom Ltd.
 * Copyright (C) 2000-2002 Greg Ungerer <gerg@snapgear.com>
 *
 * Ported from m68knommu.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_NIOS2_CACHEFLUSH_H
#define _ASM_NIOS2_CACHEFLUSH_H

void flush_cache_all(void);
void flush_dcache_all(void);
void flush_icache_all(void);
void flush_icache_range(unsigned long start, unsigned long end);
void flush_dcache_range(unsigned long start, unsigned long end);

#endif /* _ASM_NIOS2_CACHEFLUSH_H */
