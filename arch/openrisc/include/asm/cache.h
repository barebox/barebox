/*
 * (C) Copyright 2011, Stefan Kristiansson <stefan.kristiansson@saunalahti.fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_OPENRISC_CACHE_H_
#define __ASM_OPENRISC_CACHE_H_

#include <asm/dma.h>

void flush_dcache_range(unsigned long addr, unsigned long stop);
void invalidate_dcache_range(unsigned long addr, unsigned long stop);
void flush_cache(unsigned long addr, unsigned long size);
int icache_status(void);
int checkicache(void);
int dcache_status(void);
int checkdcache(void);
void dcache_enable(void);
void dcache_disable(void);
void icache_enable(void);
void icache_disable(void);

#endif /* __ASM_OPENRISC_CACHE_H_ */
