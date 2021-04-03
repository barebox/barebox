/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __ASM_SECTIONS_H
#define __ASM_SECTIONS_H

#ifndef __ASSEMBLY__
#include <asm-generic/sections.h>
#include <linux/types.h>
#include <asm/unaligned.h>

extern char __rel_dyn_start[];
extern char __rel_dyn_end[];
extern char __dynsym_start[];
extern char __dynsym_end[];

extern char input_data[];
extern char input_data_end[];

unsigned long get_runtime_offset(void);

static inline unsigned int input_data_len(void)
{
	return get_unaligned((const u32 *)(input_data_end + get_runtime_offset() - 4));
}

#endif

#endif /* __ASM_SECTIONS_H */
