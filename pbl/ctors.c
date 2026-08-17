// SPDX-License-Identifier: GPL-2.0-only

#include <asm-generic/sections.h>

typedef void (*ctor_fn_t)(void);

void pbl_do_ctors(void)
{
	ctor_fn_t *fn = (ctor_fn_t *)__ctors_start;

	for (; fn < (ctor_fn_t *)__ctors_end; fn++)
		(*fn)();
}
