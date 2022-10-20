/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __ASM_SECTIONS_H
#define __ASM_SECTIONS_H

#ifndef __ASSEMBLY__
#include <asm-generic/sections.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include <asm/reloc.h>

extern char __rel_dyn_start[];
extern char __rel_dyn_end[];
extern char __dynsym_start[];
extern char __dynsym_end[];

extern char input_data[];
extern char input_data_end[];

unsigned long get_runtime_offset(void);

static inline unsigned int input_data_len(void)
{
	return get_unaligned((const u32 *)runtime_address(input_data_end) - 1);
}

#endif

#endif /* __ASM_SECTIONS_H */
