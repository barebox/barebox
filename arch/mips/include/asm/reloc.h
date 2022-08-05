/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _ASM_RELOC_H_
#define _ASM_RELOC_H_

static inline unsigned long get_runtime_offset(void)
{
	/* On MIPS, we always relocate before jumping into C */
	return 0;
}

#include <asm-generic/reloc.h>

#endif
