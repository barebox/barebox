/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_MIPS_CACHE_H
#define _ASM_MIPS_CACHE_H

void flush_cache_all(void);
void r4k_cache_init(void);

#define sync_caches_for_execution flush_cache_all

#endif /* _ASM_MIPS_CACHE_H */
